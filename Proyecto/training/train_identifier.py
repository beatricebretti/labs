#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import random
import re
import shutil
from collections import Counter
from dataclasses import dataclass
from pathlib import Path

import numpy as np
import tensorflow as tf


CLASS_NAMES = ["no_identifier", "identifier"]
ZONE_NAMES = ["left", "center", "right"]
IMAGE_SIZE = 96
ZONE_COUNT = 3
SEED = 4200


@dataclass(frozen=True)
class LabeledImage:
    path: Path
    present: int
    quadrant: int


@dataclass(frozen=True)
class CropExample:
    path: Path
    label: int
    zone: int


def parse_label_line(line: str) -> tuple[str, int, int] | None:
    line = line.strip()
    if not line:
        return None
    parts = [part.strip() for part in re.split(r"\s*[,|;]\s*|\s+", line) if part.strip()]
    if len(parts) != 3:
        raise ValueError(f"linea invalida: {line!r}")
    filename, present, quadrant = parts
    return filename, int(present), int(quadrant)


def load_labeled_images(dataset: Path, sources: list[str]) -> list[LabeledImage]:
    images: list[LabeledImage] = []
    for source in sources:
        folder = dataset / source
        labels_path = folder / "etiquetas.txt"
        if not labels_path.exists():
            raise FileNotFoundError(f"No existe {labels_path}")

        for line_no, line in enumerate(labels_path.read_text(encoding="utf-8").splitlines(), 1):
            try:
                row = parse_label_line(line)
            except Exception as exc:
                raise ValueError(f"{labels_path}:{line_no}: {exc}") from exc
            if row is None:
                continue

            filename, present, quadrant = row
            image_path = folder / filename
            if not image_path.exists():
                raise FileNotFoundError(f"Etiqueta apunta a imagen inexistente: {image_path}")
            if present not in (0, 1):
                raise ValueError(f"{labels_path}:{line_no}: present debe ser 0 o 1")
            if quadrant not in (1, 2, 3):
                raise ValueError(f"{labels_path}:{line_no}: cuadrante debe ser 1, 2 o 3")
            images.append(LabeledImage(path=image_path, present=present, quadrant=quadrant))
    return images


def split_images(
    images: list[LabeledImage], val_fraction: float
) -> tuple[list[LabeledImage], list[LabeledImage]]:
    # Split by image, not by crop, so validation does not see sibling crops from
    # the same original frame during training.
    by_key: dict[int, list[LabeledImage]] = {0: [], 1: [], 2: [], 3: []}
    for image in images:
        key = image.quadrant if image.present else 0
        by_key[key].append(image)

    rng = random.Random(SEED)
    train: list[LabeledImage] = []
    val: list[LabeledImage] = []
    for key, key_images in by_key.items():
        if not key_images:
            continue
        rng.shuffle(key_images)
        if len(key_images) == 1:
            train.extend(key_images)
            continue
        val_count = max(1, int(round(len(key_images) * val_fraction)))
        val.extend(key_images[:val_count])
        train.extend(key_images[val_count:])

    rng.shuffle(train)
    rng.shuffle(val)
    return train, val


def expand_quadrant_examples(images: list[LabeledImage]) -> list[CropExample]:
    examples: list[CropExample] = []
    for image in images:
        positive_zone = image.quadrant - 1
        for zone in range(ZONE_COUNT):
            label = 1 if image.present and zone == positive_zone else 0
            examples.append(CropExample(path=image.path, label=label, zone=zone))
    return examples


def decode_crop(path: tf.Tensor, label: tf.Tensor, zone: tf.Tensor) -> tuple[tf.Tensor, tf.Tensor]:
    data = tf.io.read_file(path)
    image = tf.io.decode_jpeg(data, channels=1)
    image = tf.cast(image, tf.float32)
    image = tf.image.resize(image, [IMAGE_SIZE, IMAGE_SIZE], method="bilinear")
    width = tf.shape(image)[1]
    x_start = (zone * width) // ZONE_COUNT
    x_end = ((zone + 1) * width) // ZONE_COUNT
    crop = image[:, x_start:x_end, :]
    crop = tf.image.resize(crop, [IMAGE_SIZE, IMAGE_SIZE], method="nearest")
    crop = crop / 255.0
    return crop, label


def augment_crop(image: tf.Tensor, label: tf.Tensor) -> tuple[tf.Tensor, tf.Tensor]:
    image = tf.image.random_brightness(image, max_delta=0.12, seed=SEED)
    image = tf.image.random_contrast(image, lower=0.85, upper=1.15, seed=SEED)
    image = tf.clip_by_value(image, 0.0, 1.0)
    return image, label


def make_dataset(examples: list[CropExample], batch_size: int, shuffle: bool, augment: bool) -> tf.data.Dataset:
    paths = [str(example.path) for example in examples]
    labels = [example.label for example in examples]
    zones = [example.zone for example in examples]
    dataset = tf.data.Dataset.from_tensor_slices((paths, labels, zones))
    if shuffle:
        dataset = dataset.shuffle(len(examples), seed=SEED, reshuffle_each_iteration=True)
    dataset = dataset.map(decode_crop, num_parallel_calls=tf.data.AUTOTUNE)
    if augment:
        dataset = dataset.map(augment_crop, num_parallel_calls=tf.data.AUTOTUNE)
    return dataset.batch(batch_size).prefetch(tf.data.AUTOTUNE)


def make_model() -> tf.keras.Model:
    inputs = tf.keras.Input(shape=(IMAGE_SIZE, IMAGE_SIZE, 1), name="image")
    x = tf.keras.layers.Conv2D(8, 3, strides=2, padding="same", activation="relu")(inputs)
    x = tf.keras.layers.AveragePooling2D(pool_size=2)(x)
    x = tf.keras.layers.Conv2D(16, 3, padding="same", activation="relu")(x)
    x = tf.keras.layers.AveragePooling2D(pool_size=2)(x)
    x = tf.keras.layers.Conv2D(24, 3, padding="same", activation="relu")(x)
    x = tf.keras.layers.AveragePooling2D(pool_size=2)(x)
    x = tf.keras.layers.Conv2D(len(CLASS_NAMES), 1, padding="same")(x)
    x = tf.keras.layers.GlobalAveragePooling2D()(x)
    outputs = tf.keras.layers.Activation("softmax", name="output_0")(x)
    return tf.keras.Model(inputs=inputs, outputs=outputs, name="identifier_zone_detector")


def representative_dataset(examples: list[CropExample]):
    sample = examples[: min(100, len(examples))]
    for example in sample:
        data = tf.io.read_file(str(example.path))
        image = tf.io.decode_jpeg(data, channels=1)
        image = tf.cast(image, tf.float32)
        image = tf.image.resize(image, [IMAGE_SIZE, IMAGE_SIZE], method="bilinear")
        width = tf.shape(image)[1]
        x_start = (example.zone * width) // ZONE_COUNT
        x_end = ((example.zone + 1) * width) // ZONE_COUNT
        crop = image[:, x_start:x_end, :]
        crop = tf.image.resize(crop, [IMAGE_SIZE, IMAGE_SIZE], method="nearest")
        crop = crop / 255.0
        yield [tf.expand_dims(crop, axis=0)]


def convert_to_tflite(model: tf.keras.Model, calibration_examples: list[CropExample]) -> bytes:
    converter = tf.lite.TFLiteConverter.from_keras_model(model)
    converter.optimizations = [tf.lite.Optimize.DEFAULT]
    converter.representative_dataset = lambda: representative_dataset(calibration_examples)
    converter.target_spec.supported_ops = [tf.lite.OpsSet.TFLITE_BUILTINS_INT8]
    converter.inference_input_type = tf.int8
    converter.inference_output_type = tf.int8
    return converter.convert()


def write_c_array(tflite_data: bytes, out_cc: Path, out_h: Path) -> None:
    header_guard = "TENSORFLOW_LITE_MICRO_EXAMPLES_IDENTIFIER_DETECTION_IDENTIFIER_DETECT_MODEL_DATA_H_"
    out_h.write_text(
        "\n".join(
            [
                "#ifndef " + header_guard,
                "#define " + header_guard,
                "",
                "extern const unsigned char g_identifier_detect_model_data[];",
                "extern const int g_identifier_detect_model_data_len;",
                "",
                "#endif  // " + header_guard,
                "",
            ]
        ),
        encoding="utf-8",
    )

    lines = [
        '#include "identifier_detect_model_data.h"',
        "",
        "alignas(8) const unsigned char g_identifier_detect_model_data[] = {",
    ]
    for index in range(0, len(tflite_data), 12):
        chunk = tflite_data[index : index + 12]
        lines.append("  " + ", ".join(f"0x{byte:02x}" for byte in chunk) + ",")
    lines.append("};")
    lines.append(f"const int g_identifier_detect_model_data_len = {len(tflite_data)};")
    lines.append("")
    out_cc.write_text("\n".join(lines), encoding="utf-8")


def evaluate_model(model: tf.keras.Model, val_ds: tf.data.Dataset) -> dict[str, object]:
    y_true: list[int] = []
    y_pred: list[int] = []
    for images, labels in val_ds:
        probs = model.predict(images, verbose=0)
        y_true.extend(int(value) for value in labels.numpy())
        y_pred.extend(int(value) for value in np.argmax(probs, axis=1))

    matrix = np.zeros((2, 2), dtype=int)
    for true_label, pred_label in zip(y_true, y_pred):
        matrix[true_label, pred_label] += 1

    accuracy = float(np.mean(np.array(y_true) == np.array(y_pred))) if y_true else 0.0
    return {
        "class_names": CLASS_NAMES,
        "accuracy": accuracy,
        "confusion_matrix": matrix.tolist(),
    }


def main() -> int:
    parser = argparse.ArgumentParser(description="Entrena detector binario por cuadrante.")
    parser.add_argument("--dataset", default="../../dataset_embebidos_grupo3", type=Path)
    parser.add_argument("--sources", nargs="+", default=["esp"])
    parser.add_argument("--out-dir", default=Path("training/out"), type=Path)
    parser.add_argument("--epochs", default=60, type=int)
    parser.add_argument("--batch-size", default=16, type=int)
    parser.add_argument("--val-fraction", default=0.2, type=float)
    parser.add_argument("--min-per-class", default=20, type=int)
    parser.add_argument("--no-augment", action="store_true")
    parser.add_argument("--emit-firmware", action="store_true")
    args = parser.parse_args()

    random.seed(SEED)
    np.random.seed(SEED)
    tf.random.set_seed(SEED)

    dataset = args.dataset.resolve()
    images = load_labeled_images(dataset, args.sources)
    train_images, val_images = split_images(images, args.val_fraction)
    train_examples = expand_quadrant_examples(train_images)
    val_examples = expand_quadrant_examples(val_images)
    all_examples = train_examples + val_examples

    counts = Counter(example.label for example in all_examples)
    zone_positive_counts = Counter(example.zone for example in all_examples if example.label == 1)
    print("Ejemplos por clase despues de expandir cuadrantes:")
    for label, name in enumerate(CLASS_NAMES):
        print(f"  {name}: {counts.get(label, 0)}")
    print("Positivos por zona:")
    for zone, name in enumerate(ZONE_NAMES):
        print(f"  {name}: {zone_positive_counts.get(zone, 0)}")

    if min(counts.get(0, 0), counts.get(1, 0)) < args.min_per_class:
        raise SystemExit(
            "Dataset insuficiente despues de expandir cuadrantes. "
            f"Se requieren al menos {args.min_per_class} ejemplos por clase."
        )

    train_ds = make_dataset(train_examples, args.batch_size, shuffle=True, augment=not args.no_augment)
    val_ds = make_dataset(val_examples, args.batch_size, shuffle=False, augment=False)

    train_counts = Counter(example.label for example in train_examples)
    class_weight = {
        label: len(train_examples) / (len(CLASS_NAMES) * max(1, train_counts.get(label, 0)))
        for label in range(len(CLASS_NAMES))
    }

    model = make_model()
    model.compile(
        optimizer=tf.keras.optimizers.Adam(learning_rate=1e-3),
        loss="sparse_categorical_crossentropy",
        metrics=["accuracy"],
    )
    model.summary()

    callbacks = [
        tf.keras.callbacks.EarlyStopping(monitor="val_accuracy", patience=12, restore_best_weights=True),
    ]
    model.fit(
        train_ds,
        validation_data=val_ds,
        epochs=args.epochs,
        class_weight=class_weight,
        callbacks=callbacks,
    )

    args.out_dir.mkdir(parents=True, exist_ok=True)
    metrics = evaluate_model(model, val_ds)
    metrics.update(
        {
            "dataset": str(dataset),
            "sources": args.sources,
            "train_images": len(train_images),
            "val_images": len(val_images),
            "train_crops": len(train_examples),
            "val_crops": len(val_examples),
            "class_counts": {CLASS_NAMES[label]: counts.get(label, 0) for label in range(len(CLASS_NAMES))},
            "positive_zone_counts": {ZONE_NAMES[zone]: zone_positive_counts.get(zone, 0) for zone in range(ZONE_COUNT)},
        }
    )
    (args.out_dir / "metrics.json").write_text(json.dumps(metrics, indent=2), encoding="utf-8")
    model.save(args.out_dir / "identifier_model.keras")

    tflite_data = convert_to_tflite(model, train_examples)
    tflite_path = args.out_dir / "identifier_detect_model.tflite"
    tflite_path.write_bytes(tflite_data)
    generated_cc = args.out_dir / "identifier_detect_model_data.cc"
    generated_h = args.out_dir / "identifier_detect_model_data.h"
    write_c_array(tflite_data, generated_cc, generated_h)

    print(f"TFLite generado: {tflite_path}")
    print(f"Metricas: {args.out_dir / 'metrics.json'}")

    if args.emit_firmware:
        firmware_dir = Path(__file__).resolve().parents[1] / "cam_identificador" / "code" / "main"
        shutil.copy2(generated_cc, firmware_dir / "identifier_detect_model_data.cc")
        shutil.copy2(generated_h, firmware_dir / "identifier_detect_model_data.h")
        print(f"Modelo copiado a firmware: {firmware_dir}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
