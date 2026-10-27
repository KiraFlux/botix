#!/usr/bin/python
#
# SPDX-FileCopyrightText: 2026 KiraFlux
# SPDX-License-Identifier: GPL-3.0-or-later
# Botix - https://github.com/KiraFlux/botix

import argparse
import sys

from dataclasses import dataclass
from pathlib import Path
from typing import Optional, Self, Sequence

import freecad
import Mesh


@dataclass(frozen=True, kw_only=True)
class ExportJob:
    """One-shot export pipeline over one or more FreeCAD documents."""

    fcstd_sources: Sequence[Path]
    output_dir: Path
    export_stp: bool
    export_obj: bool
    target_group_names: Sequence[str]
    dry_run: bool
    name_doc_prefix: bool

    @classmethod
    def from_cli(cls, argv: Optional[Sequence[str]] = None) -> Self:
        parser = argparse.ArgumentParser(
            description="Export target parts from one or more FreeCAD assemblies.",
        )

        parser.add_argument(
            "src",
            type=Path,
            nargs="+",
            metavar="FCSTD",
            help="one or more source FreeCAD documents "
                 "(shell wildcards are expanded by the shell, not by this tool)",
        )
        parser.add_argument(
            "--dry-run",
            action="store_true",
            help="parse arguments only, do not export",
        )
        parser.add_argument(
            "-o", "--out",
            type=Path,
            default=Path.cwd(),
            metavar="DIR",
            help="output directory (default: current directory)",
        )
        parser.add_argument(
            "-v", "--verbose",
            action="store_true",
            help="print verbose diagnostics",
        )
        parser.add_argument(
            "--stp",
            action="store_true",
            help="export each part as STEP (.stp)",
        )
        parser.add_argument(
            "--obj",
            action="store_true",
            help="export each part as Wavefront OBJ (.obj)",
        )
        parser.add_argument(
            "-g", "--target-group-name",
            type=str,
            nargs="+",
            default=["printable"],
            metavar="NAME",
            help="one or more group names whose children are exported "
                 "(default: %(default)s)",
        )
        parser.add_argument(
            "--name-doc-prefix",
            action=argparse.BooleanOptionalAction,
            default=True,
            help="prefix exported names with the document label (default: %(default)s)",
        )

        ns = parser.parse_args(argv)

        return cls(
            fcstd_sources=tuple(p.resolve() for p in ns.src),
            output_dir=ns.out.resolve(),
            export_stp=ns.stp,
            export_obj=ns.obj,
            target_group_names=tuple(ns.target_group_name),
            dry_run=ns.dry_run,
            name_doc_prefix=ns.name_doc_prefix,
        )

    def output_file(self, name: str, ext: str) -> Path:
        """Compose the output path for `name` with the given extension."""
        return self.output_dir / f"{name}.{ext}"

    def is_target_group(self, g) -> bool:
        """Return True if `g` is one of the configured target groups."""
        return (
            g is not None
            and g.TypeId == "App::DocumentObjectGroup"
            and g.Label in self.target_group_names
        )

    def is_target_object(self, o) -> bool:
        """Return True if `o` is a target part inside a target group."""
        return (
            self.is_target_group(o.getParent())
            and o.TypeId in ("App::Part", "Part::FeaturePython")
        )

    def on_part(self, prefix: str, p) -> bool:
        """Export a single part; return True on success."""

        qualified_name = str(p.Label)
        if self.name_doc_prefix and qualified_name not in prefix:
            qualified_name = f"{prefix}_{qualified_name}"

        print(f"part: {qualified_name}")

        if not p.isValid():
            print("  invalid shape — skipped")
            return False

        if self.export_stp:
            stp_path = self.output_file(qualified_name, "stp")
            print(f"  stp: {stp_path}")
            if not self.dry_run:
                p.Shape.exportStep(str(stp_path))

        if self.export_obj:
            obj_path = self.output_file(qualified_name, "obj")
            print(f"  obj: {obj_path}")
            if not self.dry_run:
                mesh = Mesh.Mesh(p.Shape.tessellate(0.01))
                mesh.write(str(obj_path))

        return True

    def on_document(self, src: Path) -> bool:
        """Process a single FreeCAD document; return True if every part succeeded."""

        print(f"source: {src}")

        if not src.exists():
            print("source file does not exist")
            return False

        doc = freecad.app.openDocument(str(src))
        try:
            name = str(doc.Label).removesuffix("_asm")
            print(f"document: {name}")

            result = tuple(map(
                lambda p: self.on_part(name, p),
                filter(self.is_target_object, doc.Objects),
            ))

            if len(result) == 0:
                print("nothing to process")
                return False

            print(f"processed {sum(result)}/{len(result)} parts")
            return all(result)
        finally:
            freecad.app.closeDocument(doc.Name)

    def run(self) -> bool:
        """Run the export pipeline over every source document."""

        results = tuple(map(self.on_document, self.fcstd_sources))

        if len(results) == 0:
            print("nothing to process")
            return False

        print(f"processed {sum(results)}/{len(results)} documents")
        return all(results)


sys.exit(0 if ExportJob.from_cli().run() else 1)
