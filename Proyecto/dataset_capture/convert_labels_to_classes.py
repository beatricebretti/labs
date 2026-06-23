#!/usr/bin/env python3
from __future__ import annotations

import argparse
import re
from pathlib import Path


CLASS_BY_QUADRANT = {
    1: "left",
    2: "center",
    3: "right",
}


def default_dataset() -> Path:
    return Path(__file__).resolve().parents[3] / "dataset_embebidos_grupo3"


def parse_label_line(line: str) -> tuple[str, int, int] | None:
    line = line.strip()
    if not line:
        return None
    parts = [part.strip() for part in re.split(r"\s*[,|;]\s*|\s+", line) if part.strip()]
    if len(parts) != 3:
        raise ValueError(f"linea invalida: {line!r}")
    filename, present, quadrant = parts
    return filename, int(present), int(quadrant)


def class_name_for(present: int, quadrant: int) -> str:
    if present == 0:
        return "none"
    if present != 1:
        raise ValueError(f"present debe ser 0 o 1, recibido {present}")
    try:
        return CLASS_BY_QUADRANT[quadrant]
    except KeyError as exc:
        raise ValueError(f"cuadrante debe ser 1, 2 o 3, recibido {quadrant}") from exc


def convert_source(dataset: Path, source: str) -> None:
    folder = dataset / source
    labels_path = folder / "etiquetas.txt"
    class_labels_path = folder / "etiquetas_clases.txt"

    rows: list[str] = []
    for line_no, line in enumerate(labels_path.read_text(encoding="utf-8").splitlines(), 1):
        try:
            parsed = parse_label_line(line)
        except Exception as exc:
            raise ValueError(f"{labels_path}:{line_no}: {exc}") from exc
        if parsed is None:
            continue
        filename, present, quadrant = parsed
        rows.append(f"{filename}, {class_name_for(present, quadrant)}")

    class_labels_path.write_text("\n".join(rows) + "\n", encoding="utf-8")
    print(f"{class_labels_path}: {len(rows)} etiquetas escritas")


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Convierte etiquetas numericas a etiquetas de 4 clases."
    )
    parser.add_argument("--dataset", default=default_dataset(), type=Path)
    parser.add_argument("--sources", nargs="+", default=["esp", "celular"])
    args = parser.parse_args()

    dataset = args.dataset.resolve()
    if not dataset.exists():
        raise SystemExit(f"No existe dataset: {dataset}")

    for source in args.sources:
        convert_source(dataset, source)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
