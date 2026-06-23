#!/usr/bin/env python3
from __future__ import annotations

import argparse
import re
from collections import Counter
from pathlib import Path


def parse_label_line(line: str) -> tuple[str, int, int] | None:
    line = line.strip()
    if not line:
        return None
    parts = [part.strip() for part in re.split(r"\s*[,|;]\s*|\s+", line) if part.strip()]
    if len(parts) != 3:
        raise ValueError(f"linea invalida: {line!r}")
    return parts[0], int(parts[1]), int(parts[2])


def load_rows(folder: Path) -> list[tuple[str, int, int]]:
    rows: list[tuple[str, int, int]] = []
    labels_path = folder / "etiquetas.txt"
    for line_no, line in enumerate(labels_path.read_text(encoding="utf-8").splitlines(), 1):
        try:
            row = parse_label_line(line)
        except Exception as exc:
            raise ValueError(f"{labels_path}:{line_no}: {exc}") from exc
        if row is not None:
            rows.append(row)
    return rows


def inspect_source(dataset: Path, source: str) -> int:
    folder = dataset / source
    rows = load_rows(folder)
    files_in_labels = {filename for filename, _, _ in rows}
    jpg_files = {path.name for path in folder.glob("*.jpg")}
    missing = sorted(files_in_labels - jpg_files)
    extra = sorted(jpg_files - files_in_labels)

    present_counts = Counter(present for _, present, _ in rows)
    quadrant_counts = Counter(quadrant for _, present, quadrant in rows if present == 1)
    positive_crops = present_counts.get(1, 0)
    negative_crops = (present_counts.get(1, 0) * 2) + (present_counts.get(0, 0) * 3)

    print(f"{source}:")
    print(f"  imagenes etiquetadas: {len(rows)}")
    print(f"  presentes: {present_counts.get(1, 0)}")
    print(f"  ausentes: {present_counts.get(0, 0)}")
    print(f"  cuadrantes positivos: {dict(sorted(quadrant_counts.items()))}")
    print(f"  crops positivos al entrenar por cuadrante: {positive_crops}")
    print(f"  crops negativos al entrenar por cuadrante: {negative_crops}")
    print(f"  imagenes faltantes: {len(missing)}")
    print(f"  jpg sin etiqueta: {len(extra)}")
    if missing[:5]:
        print(f"  faltantes ejemplo: {missing[:5]}")
    if extra[:5]:
        print(f"  extra ejemplo: {extra[:5]}")
    print()
    return 1 if missing else 0


def main() -> int:
    parser = argparse.ArgumentParser(description="Inspecciona etiquetas del dataset.")
    parser.add_argument("--dataset", default="../../dataset_embebidos_grupo3", type=Path)
    parser.add_argument("--sources", nargs="+", default=["celular", "esp"])
    args = parser.parse_args()

    dataset = args.dataset.resolve()
    if not dataset.exists():
        raise SystemExit(f"No existe dataset: {dataset}")

    status = 0
    for source in args.sources:
        status |= inspect_source(dataset, source)
    return status


if __name__ == "__main__":
    raise SystemExit(main())
