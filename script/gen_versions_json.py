#!/usr/bin/env python3
"""Generate versions.json from the directories present in the current directory.

Run this script from the gh-pages worktree root after updating its contents.
Output: versions.json with versions sorted as: nightly, latest, then v* descending.
"""
import json
import os
import re

dirs = [
    d for d in os.listdir(".")
    if os.path.isdir(d) and re.match(r"^(nightly|latest|v)", d)
]

order = {"nightly": 0, "latest": 1}
versions = sorted(
    dirs,
    key=lambda x: (
        order.get(x, 2),
        tuple(-int(n) for n in re.findall(r"\d+", x)) if x not in order else (),
    ),
)

with open("versions.json", "w") as f:
    json.dump({"versions": versions}, f, indent=2)
    f.write("\n")
