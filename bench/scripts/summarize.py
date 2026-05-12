from __future__ import annotations

import csv
import json
import sys
from argparse import ArgumentParser
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
RESULTS = Path(__import__("os").environ.get("RESULTS_DIR", ROOT / "bench" / "results"))


def read_tsv(paths: list[Path]) -> list[list[str]]:
    rows: list[list[str]] = []
    header: list[str] | None = None
    for path in paths:
        if not path.exists():
            continue
        with path.open(newline="") as f:
            file_rows = list(csv.reader(f, delimiter="\t"))
        if not file_rows:
            continue
        if header is None:
            header = file_rows[0]
            rows.append(header)
        rows.extend(file_rows[1:])
    return rows


def markdown_table(rows: list[tuple[str, ...]] | list[list[str]], title: str) -> str:
    if not rows:
        return ""
    out = [f"## {title}", ""]
    out.append("| " + " | ".join(rows[0]) + " |")
    out.append("| " + " | ".join("---" for _ in rows[0]) + " |")
    for row in rows[1:]:
        out.append("| " + " | ".join(row) + " |")
    out.append("")
    return "\n".join(out)


def tsv_table(paths: list[Path], title: str) -> str:
    return markdown_table(read_tsv(paths), title)


def runtime_table(paths: list[Path]) -> str:
    rows = [("standard", "name", "time_ns", "cpu_ns", "allocs", "bytes")]
    for path in paths:
        if not path.exists():
            continue
        standard = path.stem.removeprefix("runtime_")
        data = json.loads(path.read_text())
        for bench in data.get("benchmarks", []):
            rows.append(
                (
                    standard,
                    bench.get("name", ""),
                    f"{bench.get('real_time', 0):.2f}",
                    f"{bench.get('cpu_time', 0):.2f}",
                    f"{bench.get('allocs', 0):.2f}",
                    f"{bench.get('bytes', 0):.2f}",
                )
            )
    return markdown_table(rows, "Runtime")


def main() -> None:
    parser = ArgumentParser()
    parser.add_argument("-o", "--output", type=Path)
    args = parser.parse_args()

    sections = [
        "# cli20 benchmark report",
        "",
        runtime_table(sorted(RESULTS.glob("runtime_cxx*.json"))),
        tsv_table(sorted(RESULTS.glob("size_cxx*.tsv")), "Binary Size"),
        tsv_table(
            sorted(RESULTS.glob("compile_time/cxx*/results.tsv")),
            "Compile Time",
        ),
    ]
    report = "\n".join(section for section in sections if section)
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(report + "\n")
    sys.stdout.write(report + "\n")


if __name__ == "__main__":
    main()
