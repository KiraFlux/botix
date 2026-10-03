#!/usr/bin/env python3
#
# SPDX-License-Identifier: GPL-3.0-or-later
# FIXME: Generated via LLM!

"""
Fix external file paths inside .fcstd files (FreeCAD project = zip).

  python3 fixfcstd.py mcad --inspect
  python3 fixfcstd.py mcad --old mcad/lib --new cadref
"""

import argparse
import re
import shutil
import zipfile
from pathlib import Path

FILE_ATTR = re.compile(r'file="([^"]+)"')


def inspect(path: Path) -> None:
    with zipfile.ZipFile(path) as z:
        xml = z.read("Document.xml").decode("utf-8")
    paths = sorted(set(FILE_ATTR.findall(xml)))
    if not paths:
        return
    print(path)
    for p in paths:
        print(f"  {p}")


def patch(path: Path, old: str, new: str, backup: bool) -> bool:
    with zipfile.ZipFile(path) as z:
        xml = z.read("Document.xml").decode("utf-8")
    if old not in xml:
        return False

    n = xml.count(old)
    xml = xml.replace(old, new)

    if backup:
        bak = path.with_suffix(path.suffix + ".bak")
        if not bak.exists():
            shutil.copy2(path, bak)

    tmp = path.with_suffix(path.suffix + ".tmp")
    with zipfile.ZipFile(path) as zin, zipfile.ZipFile(tmp, "w", zipfile.ZIP_DEFLATED) as zout:
        for item in zin.infolist():
            data = zin.read(item.filename)
            if item.filename == "Document.xml":
                data = xml.encode("utf-8")
            zout.writestr(item, data)
    tmp.replace(path)
    print(f"  patched: {path} ({n} occurrence(s))")
    return True


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("root", type=Path)
    ap.add_argument("--old")
    ap.add_argument("--new")
    ap.add_argument("--inspect", action="store_true")
    ap.add_argument("--no-backup", action="store_true")
    args = ap.parse_args()

    if args.inspect:
        for fcstd in sorted(args.root.rglob("*.fcstd")):
            inspect(fcstd)
        return

    if not args.old or not args.new:
        ap.error("--old and --new required unless --inspect")

    for fcstd in sorted(args.root.rglob("*.fcstd")):
        patch(fcstd, args.old, args.new, backup=not args.no_backup)


if __name__ == "__main__":
    main()
