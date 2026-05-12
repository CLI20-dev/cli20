#!/usr/bin/env python3
from __future__ import annotations

import csv
import json
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
RESULTS = ROOT / "bench" / "results"


def print_tsv(path: Path, title: str) -> None:
    if not path.exists():
        return
    print(f"\n## {title}")
    with path.open(newline="") as f:
        rows = list(csv.reader(f, delimiter="\t"))
    if not rows:
        return
    widths = [max(len(row[i]) for row in rows) for i in range(len(rows[0]))]
    for row in rows:
        print("  ".join(cell.ljust(widths[i]) for i, cell in enumerate(row)))


def print_runtime(path: Path) -> None:
    if not path.exists():
        return
    print("\n## Runtime")
    data = json.loads(path.read_text())
    rows = [("name", "time_ns", "cpu_ns")]
    for bench in data.get("benchmarks", []):
        rows.append(
            (
                bench.get("name", ""),
                f"{bench.get('real_time', 0):.2f}",
                f"{bench.get('cpu_time', 0):.2f}",
            )
        )
    widths = [max(len(row[i]) for row in rows) for i in range(len(rows[0]))]
    for row in rows:
        print("  ".join(cell.ljust(widths[i]) for i, cell in enumerate(row)))


def main() -> None:
    print_runtime(RESULTS / "runtime.json")
    print_tsv(RESULTS / "size.tsv", "Binary Size")
    print_tsv(RESULTS / "compile_time" / "results.tsv", "Compile Time")


if __name__ == "__main__":
    main()
