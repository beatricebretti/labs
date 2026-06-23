#!/usr/bin/env python3
from __future__ import annotations

import argparse
import base64
import re
import time
from pathlib import Path

import serial
from PIL import Image


LABEL_TO_NUMERIC = {
    "none": (0, 1),
    "left": (1, 1),
    "center": (1, 2),
    "right": (1, 3),
}

FRAME_BEGIN_RE = re.compile(
    r"FRAME_BEGIN\s+id=(?P<id>\d+)\s+width=(?P<width>\d+)\s+"
    r"height=(?P<height>\d+)\s+bytes=(?P<bytes>\d+)\s+encoding=base64"
)
FRAME_END_RE = re.compile(r"FRAME_END\s+id=(?P<id>\d+)")


def default_dataset() -> Path:
    return Path(__file__).resolve().parents[3] / "dataset_embebidos_grupo3"


def next_image_number(folder: Path, source: str) -> int:
    pattern = re.compile(rf"{re.escape(source)}_(\d+)\.jpg$")
    numbers = []
    for path in folder.glob(f"{source}_*.jpg"):
        match = pattern.match(path.name)
        if match:
            numbers.append(int(match.group(1)))
    return max(numbers, default=0) + 1


def append_line(path: Path, line: str) -> None:
    if path.exists() and path.stat().st_size > 0:
        with path.open("rb+") as file:
            file.seek(-1, 2)
            if file.read(1) != b"\n":
                file.write(b"\n")
    with path.open("a", encoding="utf-8") as file:
        file.write(line + "\n")


def save_frame(
    raw: bytes,
    width: int,
    height: int,
    output_path: Path,
    save_size: int,
    jpeg_quality: int,
) -> None:
    image = Image.frombytes("L", (width, height), raw)
    if save_size > 0 and (width != save_size or height != save_size):
        image = image.resize((save_size, save_size), Image.Resampling.BILINEAR)
    image.convert("RGB").save(output_path, format="JPEG", quality=jpeg_quality)


def read_frame(port: serial.Serial) -> tuple[int, int, int, bytes]:
    while True:
        line = port.readline().decode("utf-8", errors="ignore").strip()
        match = FRAME_BEGIN_RE.search(line)
        if not match:
            continue

        frame_id = int(match.group("id"))
        width = int(match.group("width"))
        height = int(match.group("height"))
        expected_bytes = int(match.group("bytes"))
        encoded_lines: list[str] = []

        while True:
            frame_line = port.readline().decode("utf-8", errors="ignore").strip()
            end_match = FRAME_END_RE.search(frame_line)
            if end_match:
                end_id = int(end_match.group("id"))
                if end_id != frame_id:
                    raise RuntimeError(f"FRAME_END id={end_id} no calza con id={frame_id}")
                break
            if frame_line:
                encoded_lines.append(frame_line)

        raw = base64.b64decode("".join(encoded_lines), validate=True)
        if len(raw) != expected_bytes:
            raise RuntimeError(
                f"Frame {frame_id}: se esperaban {expected_bytes} bytes, llegaron {len(raw)}"
            )
        return frame_id, width, height, raw


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Lee fotos desde la ESP32-CAM por UART y las agrega al dataset."
    )
    parser.add_argument("--port", required=True, help="Puerto serial, por ejemplo /dev/ttyUSB0")
    parser.add_argument("--baud", default=115200, type=int)
    parser.add_argument("--dataset", default=default_dataset(), type=Path)
    parser.add_argument("--source", default="esp")
    parser.add_argument("--label", required=True, choices=sorted(LABEL_TO_NUMERIC))
    parser.add_argument("--count", default=20, type=int)
    parser.add_argument("--save-size", default=128, type=int)
    parser.add_argument("--jpeg-quality", default=95, type=int)
    args = parser.parse_args()

    dataset = args.dataset.resolve()
    folder = dataset / args.source
    labels_path = folder / "etiquetas.txt"
    class_labels_path = folder / "etiquetas_clases.txt"

    if not folder.exists():
        raise SystemExit(f"No existe carpeta de dataset: {folder}")
    if not labels_path.exists():
        raise SystemExit(f"No existe archivo de etiquetas: {labels_path}")

    present, quadrant = LABEL_TO_NUMERIC[args.label]
    next_number = next_image_number(folder, args.source)

    print(f"Dataset: {folder}")
    print(f"Etiqueta: {args.label} -> present={present}, quadrant={quadrant}")
    print(f"Primer archivo: {args.source}_{next_number:03d}.jpg")
    print("Esperando frames por serial...")

    saved = 0
    with serial.Serial(args.port, args.baud, timeout=15) as port:
        time.sleep(2.0)
        port.reset_input_buffer()

        while saved < args.count:
            frame_id, width, height, raw = read_frame(port)
            filename = f"{args.source}_{next_number:03d}.jpg"
            output_path = folder / filename

            save_frame(raw, width, height, output_path, args.save_size, args.jpeg_quality)
            append_line(labels_path, f"{filename}, {present}, {quadrant}")
            append_line(class_labels_path, f"{filename}, {args.label}")

            saved += 1
            next_number += 1
            print(
                f"[{saved}/{args.count}] frame={frame_id} guardado {output_path.name} "
                f"({width}x{height} -> {args.save_size}x{args.save_size})"
            )

    print("Captura terminada.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
