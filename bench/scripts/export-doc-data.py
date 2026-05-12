from __future__ import annotations

import csv
import json
import re
import sys
from pathlib import Path


LIBRARY_NAMES = {
    "Cli20": "cli20",
    "Cli11": "cli11",
    "Argparse": "argparse",
    "Cxxopts": "cxxopts",
    "Boost": "boost",
}

CASE_NAMES = {
    "Simple": "simple",
    "ManyOptions": "many_options",
    "Subcommand": "subcommand",
}

RUNTIME_RE = re.compile(
    r"^BM_(Cli20|Cli11|Argparse|Cxxopts|Boost)(Simple|ManyOptions|Subcommand)$"
)


def read_runtime(path: Path, standard: str) -> list[dict[str, object]]:
    data = json.loads(path.read_text())
    rows: list[dict[str, object]] = []
    for bench in data.get("benchmarks", []):
        match = RUNTIME_RE.match(str(bench.get("name", "")))
        if not match:
            continue
        library_name, case_name = match.groups()
        rows.append(
            {
                "standard": standard,
                "case": CASE_NAMES[case_name],
                "library": LIBRARY_NAMES[library_name],
                "timeNs": bench.get("real_time", 0),
                "cpuNs": bench.get("cpu_time", 0),
                "allocations": bench.get("allocs", 0),
                "allocatedBytes": bench.get("bytes", 0),
            }
        )
    return rows


def read_size(path: Path) -> list[dict[str, object]]:
    if not path.exists():
        return []
    with path.open(newline="") as file:
        rows = csv.DictReader(file, delimiter="\t")
        return [
            {
                "standard": row["standard"].replace("++", "xx"),
                "case": row["case"],
                "library": row["library"],
                "unstrippedBytes": int(row["unstripped_bytes"]),
                "strippedBytes": int(row["stripped_bytes"]),
            }
            for row in rows
            if row.get("stripped_bytes", "").isdigit()
        ]


def read_compile_time(path: Path) -> list[dict[str, object]]:
    if not path.exists():
        return []
    with path.open(newline="") as file:
        rows = csv.DictReader(file, delimiter="\t")
        return [
            {
                "standard": row["standard"].replace("++", "xx"),
                "case": row["case"],
                "library": row["library"],
                "seconds": float(row["seconds"]),
            }
            for row in rows
        ]


def collect(result_dir: Path) -> dict[str, object]:
    runtime_rows: list[dict[str, object]] = []
    size_rows: list[dict[str, object]] = []
    compile_rows: list[dict[str, object]] = []

    for runtime_path in sorted(result_dir.glob("runtime_cxx*.json")):
        standard = runtime_path.stem.removeprefix("runtime_")
        runtime_rows.extend(read_runtime(runtime_path, standard))

    for size_path in sorted(result_dir.glob("size_cxx*.tsv")):
        size_rows.extend(read_size(size_path))

    for compile_path in sorted(result_dir.glob("compile_time/cxx*/results.tsv")):
        compile_rows.extend(read_compile_time(compile_path))

    standards = sorted(
        {
            str(row["standard"])
            for row in [*runtime_rows, *size_rows, *compile_rows]
        }
    )
    cases = ["simple", "many_options", "subcommand"]
    libraries = ["cli20", "cli11", "argparse", "cxxopts", "boost"]

    return {
        "schemaVersion": 1,
        "standards": standards,
        "cases": cases,
        "libraries": libraries,
        "runtime": runtime_rows,
        "size": size_rows,
        "compileTime": compile_rows,
    }


def main() -> None:
    if len(sys.argv) != 3:
        print("usage: export-doc-data.py RESULT_DIR OUTPUT_JSON", file=sys.stderr)
        raise SystemExit(2)

    result_dir = Path(sys.argv[1]).resolve()
    output = Path(sys.argv[2]).resolve()
    data = collect(result_dir)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(data, indent=2, sort_keys=True) + "\n")


if __name__ == "__main__":
    main()
