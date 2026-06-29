#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import random
import shutil
from collections import Counter
from dataclasses import dataclass
from pathlib import Path

import numpy as np
import tensorflow as tf


CLASS_NAMES = ["no_can", "can"]
IMAGE_SIZE = 96
SEED = 4200


@dataclass(frozen=True)
class ImageExample:
    path: Path
    label: int


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Entrena un detector binario de latas para ESP32-CAM."
    )
    parser.add_argument(
        "--dataset",
        type=Path,
        default=Path("dataset"),
        help="Carpeta con subcarpetas latas/ y no_latas/.",
    )
    parser.add_argument(
        "--can-dir",
        type=Path,
        default=None,
        help="Carpeta positiva. Sobrescribe --dataset/latas.",
    )
    parser.add_argument(
        "--not-can-dir",
        type=Path,
        default=None,
        help="Carpeta negativa. Sobrescribe --dataset/no_latas.",
    )
    parser.add_argument("--epochs", type=int, default=80)
    parser.add_argument("--batch-size", type=int, default=16)
    parser.add_argument("--val-fraction", type=float, default=0.2)
    parser.add_argument("--out-dir", type=Path, default=Path("training/out"))
    parser.add_argument(
        "--emit-firmware",
        action="store_true",
        help="Copia can_detect_model_data.cc/.h al firmware cam_latas.",
    )
    return parser.parse_args()


def find_class_dir(root: Path, names: list[str]) -> Path:
    for name in names:
        candidate = root / name
        if candidate.is_dir():
            return candidate
    raise FileNotFoundError(
        f"No encontre ninguna de estas carpetas dentro de {root}: {', '.join(names)}"
    )


def iter_images(folder: Path) -> list[Path]:
    extensions = {".jpg", ".jpeg", ".png", ".bmp"}
    paths = [
        path
        for path in folder.rglob("*")
        if path.is_file() and path.suffix.lower() in extensions
    ]
    return sorted(paths)


def load_examples(args: argparse.Namespace) -> list[ImageExample]:
    can_dir = args.can_dir or find_class_dir(
        args.dataset, ["latas", "can", "cans", "positivas", "positive", "1"]
    )
    not_can_dir = args.not_can_dir or find_class_dir(
        args.dataset, ["no_latas", "otras", "not_can", "no_can", "negativas", "negative", "0"]
    )

    can_images = iter_images(can_dir)
    not_can_images = iter_images(not_can_dir)
    if not can_images:
        raise ValueError(f"No hay imagenes positivas en {can_dir}")
    if not not_can_images:
        raise ValueError(f"No hay imagenes negativas en {not_can_dir}")

    examples = [ImageExample(path=path, label=1) for path in can_images]
    examples.extend(ImageExample(path=path, label=0) for path in not_can_images)
    return examples


def split_examples(
    examples: list[ImageExample], val_fraction: float
) -> tuple[list[ImageExample], list[ImageExample]]:
    rng = random.Random(SEED)
    by_label = {0: [], 1: []}
    for example in examples:
        by_label[example.label].append(example)

    train: list[ImageExample] = []
    val: list[ImageExample] = []
    for label_examples in by_label.values():
        rng.shuffle(label_examples)
        val_count = max(1, int(round(len(label_examples) * val_fraction)))
        val.extend(label_examples[:val_count])
        train.extend(label_examples[val_count:])

    rng.shuffle(train)
    rng.shuffle(val)
    return train, val


def decode_image(path: tf.Tensor, label: tf.Tensor) -> tuple[tf.Tensor, tf.Tensor]:
    data = tf.io.read_file(path)
    image = tf.io.decode_image(data, channels=1, expand_animations=False)
    image.set_shape([None, None, 1])
    image = tf.cast(image, tf.float32)
    image = tf.image.resize(image, [IMAGE_SIZE, IMAGE_SIZE], method="bilinear")
    image = image / 255.0
    return image, label


def augment_image(image: tf.Tensor, label: tf.Tensor) -> tuple[tf.Tensor, tf.Tensor]:
    image = tf.image.random_flip_left_right(image, seed=SEED)
    image = tf.image.random_brightness(image, max_delta=0.12, seed=SEED)
    image = tf.image.random_contrast(image, lower=0.85, upper=1.15, seed=SEED)
    image = tf.clip_by_value(image, 0.0, 1.0)
    return image, label


def make_dataset(
    examples: list[ImageExample], batch_size: int, shuffle: bool, augment: bool
) -> tf.data.Dataset:
    paths = [str(example.path) for example in examples]
    labels = [example.label for example in examples]
    dataset = tf.data.Dataset.from_tensor_slices((paths, labels))
    if shuffle:
        dataset = dataset.shuffle(len(examples), seed=SEED, reshuffle_each_iteration=True)
    dataset = dataset.map(decode_image, num_parallel_calls=tf.data.AUTOTUNE)
    if augment:
        dataset = dataset.map(augment_image, num_parallel_calls=tf.data.AUTOTUNE)
    return dataset.batch(batch_size).prefetch(tf.data.AUTOTUNE)


def make_model() -> tf.keras.Model:
    inputs = tf.keras.Input(shape=(IMAGE_SIZE, IMAGE_SIZE, 1), name="image")
    x = tf.keras.layers.Conv2D(8, 3, strides=2, padding="same", activation="relu")(inputs)
    x = tf.keras.layers.AveragePooling2D(pool_size=2)(x)
    x = tf.keras.layers.Conv2D(16, 3, padding="same", activation="relu")(x)
    x = tf.keras.layers.AveragePooling2D(pool_size=2)(x)
    x = tf.keras.layers.Conv2D(24, 3, padding="same", activation="relu")(x)
    x = tf.keras.layers.AveragePooling2D(pool_size=2)(x)
    x = tf.keras.layers.Flatten()(x)
    x = tf.keras.layers.Dense(24, activation="relu")(x)
    x = tf.keras.layers.Dense(len(CLASS_NAMES))(x)
    outputs = tf.keras.layers.Activation("softmax", name="output_0")(x)
    return tf.keras.Model(inputs=inputs, outputs=outputs, name="can_detector")


def representative_dataset(examples: list[ImageExample]):
    sample = examples[: min(100, len(examples))]
    for example in sample:
        data = tf.io.read_file(str(example.path))
        image = tf.io.decode_image(data, channels=1, expand_animations=False)
        image.set_shape([None, None, 1])
        image = tf.cast(image, tf.float32)
        image = tf.image.resize(image, [IMAGE_SIZE, IMAGE_SIZE], method="bilinear")
        image = image / 255.0
        yield [tf.expand_dims(image, axis=0)]


def convert_to_tflite(model: tf.keras.Model, calibration_examples: list[ImageExample]) -> bytes:
    converter = tf.lite.TFLiteConverter.from_keras_model(model)
    converter.optimizations = [tf.lite.Optimize.DEFAULT]
    converter.representative_dataset = lambda: representative_dataset(calibration_examples)
    converter.target_spec.supported_ops = [tf.lite.OpsSet.TFLITE_BUILTINS_INT8]
    converter.inference_input_type = tf.int8
    converter.inference_output_type = tf.int8
    return converter.convert()


def classification_report(y_true: list[int], y_pred: list[int]) -> dict[str, object]:
    matrix = np.zeros((len(CLASS_NAMES), len(CLASS_NAMES)), dtype=int)
    for true_label, pred_label in zip(y_true, y_pred):
        matrix[true_label, pred_label] += 1
    accuracy = float(np.mean(np.array(y_true) == np.array(y_pred))) if y_true else 0.0
    return {
        "class_names": CLASS_NAMES,
        "accuracy": accuracy,
        "confusion_matrix": matrix.tolist(),
    }


def evaluate_keras(model: tf.keras.Model, val_ds: tf.data.Dataset) -> dict[str, object]:
    y_true: list[int] = []
    y_pred: list[int] = []
    for images, labels in val_ds:
        probs = model.predict(images, verbose=0)
        y_true.extend(int(value) for value in labels.numpy())
        y_pred.extend(int(value) for value in np.argmax(probs, axis=1))
    return classification_report(y_true, y_pred)


def quantize_for_tensor(tensor: np.ndarray, input_detail: dict[str, object]) -> np.ndarray:
    scale, zero_point = input_detail["quantization"]
    dtype = input_detail["dtype"]
    if scale == 0:
        return tensor.astype(dtype)
    q = np.round(tensor / scale + zero_point)
    info = np.iinfo(dtype)
    return np.clip(q, info.min, info.max).astype(dtype)


def dequantize_output(output: np.ndarray, output_detail: dict[str, object]) -> np.ndarray:
    scale, zero_point = output_detail["quantization"]
    if scale == 0:
        return output.astype(np.float32)
    return (output.astype(np.float32) - zero_point) * scale


def load_single_image(path: Path) -> np.ndarray:
    data = tf.io.read_file(str(path))
    image = tf.io.decode_image(data, channels=1, expand_animations=False)
    image.set_shape([None, None, 1])
    image = tf.cast(image, tf.float32)
    image = tf.image.resize(image, [IMAGE_SIZE, IMAGE_SIZE], method="bilinear")
    image = image / 255.0
    return tf.expand_dims(image, axis=0).numpy()


def evaluate_tflite(tflite_data: bytes, examples: list[ImageExample]) -> dict[str, object]:
    interpreter = tf.lite.Interpreter(model_content=tflite_data)
    interpreter.allocate_tensors()
    input_detail = interpreter.get_input_details()[0]
    output_detail = interpreter.get_output_details()[0]

    y_true: list[int] = []
    y_pred: list[int] = []
    can_scores: list[float] = []
    no_can_scores: list[float] = []

    for example in examples:
        image = load_single_image(example.path)
        interpreter.set_tensor(input_detail["index"], quantize_for_tensor(image, input_detail))
        interpreter.invoke()
        output = interpreter.get_tensor(output_detail["index"])[0]
        probs = dequantize_output(output, output_detail)
        pred = int(np.argmax(probs))
        y_true.append(example.label)
        y_pred.append(pred)
        no_can_scores.append(float(probs[0] * 100.0))
        can_scores.append(float(probs[1] * 100.0))

    report = classification_report(y_true, y_pred)
    report["can_score_percentiles"] = percentiles(can_scores)
    report["no_can_score_percentiles"] = percentiles(no_can_scores)
    return report


def percentiles(values: list[float]) -> dict[str, float]:
    if not values:
        return {}
    array = np.array(values, dtype=np.float32)
    return {
        "min": float(np.min(array)),
        "p10": float(np.percentile(array, 10)),
        "mean": float(np.mean(array)),
        "p90": float(np.percentile(array, 90)),
        "max": float(np.max(array)),
    }


def write_c_array(tflite_data: bytes, out_cc: Path, out_h: Path) -> None:
    header_guard = "JULIAN_CAN_DETECT_MODEL_DATA_H_"
    out_h.write_text(
        "\n".join(
            [
                "#ifndef " + header_guard,
                "#define " + header_guard,
                "",
                "extern const unsigned char g_can_detect_model_data[];",
                "extern const int g_can_detect_model_data_len;",
                "",
                "#endif  // " + header_guard,
                "",
            ]
        ),
        encoding="utf-8",
    )

    lines = [
        '#include "can_detect_model_data.h"',
        "",
        "alignas(8) const unsigned char g_can_detect_model_data[] = {",
    ]
    for index in range(0, len(tflite_data), 12):
        chunk = tflite_data[index : index + 12]
        lines.append("  " + ", ".join(f"0x{byte:02x}" for byte in chunk) + ",")
    lines.append("};")
    lines.append(f"const int g_can_detect_model_data_len = {len(tflite_data)};")
    lines.append("")
    out_cc.write_text("\n".join(lines), encoding="utf-8")


def main() -> None:
    args = parse_args()
    random.seed(SEED)
    np.random.seed(SEED)
    tf.random.set_seed(SEED)

    examples = load_examples(args)
    train_examples, val_examples = split_examples(examples, args.val_fraction)
    train_ds = make_dataset(train_examples, args.batch_size, shuffle=True, augment=True)
    val_ds = make_dataset(val_examples, args.batch_size, shuffle=False, augment=False)

    model = make_model()
    model.compile(
        optimizer=tf.keras.optimizers.Adam(learning_rate=1e-3),
        loss=tf.keras.losses.SparseCategoricalCrossentropy(),
        metrics=["accuracy"],
    )

    callbacks = [
        tf.keras.callbacks.EarlyStopping(
            monitor="val_accuracy", patience=12, restore_best_weights=True
        ),
        tf.keras.callbacks.ReduceLROnPlateau(
            monitor="val_loss", factor=0.5, patience=6, min_lr=1e-5
        ),
    ]
    model.fit(
        train_ds,
        validation_data=val_ds,
        epochs=args.epochs,
        callbacks=callbacks,
        verbose=2,
    )

    args.out_dir.mkdir(parents=True, exist_ok=True)
    keras_path = args.out_dir / "can_model.keras"
    tflite_path = args.out_dir / "can_detect_model.tflite"
    out_cc = args.out_dir / "can_detect_model_data.cc"
    out_h = args.out_dir / "can_detect_model_data.h"
    metrics_path = args.out_dir / "metrics.json"

    model.save(keras_path)
    tflite_data = convert_to_tflite(model, train_examples)
    tflite_path.write_bytes(tflite_data)
    write_c_array(tflite_data, out_cc, out_h)

    metrics = evaluate_keras(model, val_ds)
    metrics.update(
        {
            "dataset": str(args.dataset),
            "train_images": len(train_examples),
            "val_images": len(val_examples),
            "class_counts": dict(Counter(example.label for example in examples)),
            "class_names": CLASS_NAMES,
            "tflite": evaluate_tflite(tflite_data, val_examples),
        }
    )
    metrics_path.write_text(json.dumps(metrics, indent=2), encoding="utf-8")

    if args.emit_firmware:
        firmware_dir = Path(__file__).resolve().parents[1] / "cam_latas" / "code" / "main"
        shutil.copy2(out_cc, firmware_dir / "can_detect_model_data.cc")
        shutil.copy2(out_h, firmware_dir / "can_detect_model_data.h")
        print(f"Modelo copiado al firmware: {firmware_dir}")

    print(f"Modelo Keras: {keras_path}")
    print(f"Modelo TFLite INT8: {tflite_path}")
    print(f"Arreglo C: {out_cc}")
    print(f"Metricas: {metrics_path}")


if __name__ == "__main__":
    main()
