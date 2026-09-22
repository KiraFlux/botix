#!/usr/bin/python
#
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 KiraFlux

import argparse
from abc import ABC, abstractmethod
from dataclasses import dataclass
from itertools import chain
import json
import math
from pathlib import Path
import subprocess
from typing import Any, Callable, ClassVar, Final, Iterable, Mapping, Optional, Self, Sequence, TextIO


@dataclass(frozen=True)
class SpherePosition:
    azimuth: float
    elevation: float

    @classmethod
    def from_degrees(cls, azimuth_degrees: float, elevation_degrees: float) -> Self:
        return cls(math.radians(azimuth_degrees), math.radians(elevation_degrees))


@dataclass(frozen=True)
class Vector3[T: (float, int)]:

    length: ClassVar = 3

    x: T
    y: T
    z: T

    @classmethod
    def zero(cls) -> Self:
        return cls.from_scalar(0)

    @classmethod
    def one(cls) -> Self:
        return cls.from_scalar(1)

    @classmethod
    def from_scalar(cls, t: T) -> Self:
        return cls(t, t, t)

    @classmethod
    def from_spherical(cls, sphere_position: SpherePosition) -> Self:
        return cls(
            math.cos(sphere_position.elevation) * math.cos(sphere_position.azimuth),
            math.cos(sphere_position.elevation) * math.sin(sphere_position.azimuth),
            math.sin(sphere_position.elevation),
        )

    def __add__(self, other: Self) -> Self:
        return self.__class__(*map(sum, zip(self, other)))

    def __sub__(self, other: Self) -> Self:
        return self + (-other)

    def __mul__(self, scalar: T) -> Self:
        return self.__class__(*((scalar * a) for a in self))

    def __neg__(self) -> Self:
        return self * (-1.0)

    def __iter__(self) -> Iterable[T]:
        return iter((self.x, self.y, self.z))

    def __eq__(self, other: Self) -> bool:
        return all((a == b) for a, b in zip(self, other))

    def __getitem__(self, key: int) -> T:
        if key == 0:
            return self.x
        if key == 1:
            return self.y
        if key == 2:
            return self.z
        raise ValueError()

    def magnitude(self) -> float:
        return math.hypot(self.x, self.y, self.z)

    def pov_str(self) -> str:
        return f"<{self.x}, {self.y}, {self.z}>"


Vector3i = Vector3[int]
Vector3f = Vector3[float]


class Color(Vector3f):

    @classmethod
    def from_hex(cls, _hex: str) -> Self:
        if len(_hex) == 7 and _hex[0] == "#":
            return cls(*map(lambda i: int(_hex[i:i+2], base=16) / 255, range(1, len(_hex), 2)))

        raise ValueError(f"{_hex} not a valid HEX color format")


@dataclass(kw_only=True, frozen=True)
class Mesh:
    vertices: Sequence[Vector3f]
    faces: Sequence[Vector3i]

    @classmethod
    def from_obj(cls, obj_lines: Iterable[str]) -> Self:
        vertices = list[Vector3f]()
        faces = list[Vector3i]()

        for line in obj_lines:
            line = line.strip()
            if not line or line.startswith("#"):
                continue

            prefix, *tokens = line.split()

            if prefix == "v":
                vertices.append(Vector3f(*map(float, tokens)))

            elif prefix == "f":
                idx = tuple(int(x.split("/")[0]) - 1 for x in tokens)
                assert len(idx) >= 3

                for b, c in zip(idx[1:], idx[2:]):
                    faces.append(Vector3i(idx[0], b, c))

        return cls(vertices=vertices, faces=faces)

    def bounds(self) -> tuple[Vector3f, Vector3f]:
        return (
            self.map_all_vertices(min),
            self.map_all_vertices(max),
        )

    def map_all_vertices(self, f: Callable[[Iterable[float]], float]) -> Vector3f:
        return Vector3f(*map(lambda i: f(map(lambda v: v[i], self.vertices)), range(Vector3f.length)))


class PovObject(ABC):

    @abstractmethod
    def write_pov(self, pov: TextIO) -> None:
        """write object as POV render format"""
        raise NotImplementedError


@dataclass(kw_only=True, frozen=True)
class Solid(PovObject):

    @dataclass(kw_only=True, frozen=True)
    class Material:
        type Entry = Mapping[str, float]

        ambient: float
        diffuse: float
        specular: float
        roughness: float

        def __post_init__(self):
            assert 0.0 <= self.ambient <= 1.0
            assert 0.0 <= self.diffuse <= 1.0
            assert 0.0 <= self.specular <= 1.0
            assert 0.0 <= self.roughness <= 1.0

        @classmethod
        def from_entry(cls, entry: Entry) -> Self:
            return cls(
                ambient=float(entry["ambient"]),
                diffuse=float(entry["diffuse"]),
                specular=float(entry["specular"]),
                roughness=float(entry["roughness"]),
            )

    @dataclass(kw_only=True, frozen=True)
    class Transformation:
        translate: Vector3f
        rotate: Vector3f
        scale: Vector3f

    mesh: Mesh
    transformation: Transformation
    material: Material
    pigment_color: Color

    def write_pov(self, pov: TextIO) -> None:
        pov.write("mesh2 {\n")

        pov.write(f"vertex_vectors {{ {len(self.mesh.vertices)}, {','.join(v.pov_str() for v in self.mesh.vertices)} }}\n")
        pov.write(f"face_indices {{ {len(self.mesh.faces)}, {','.join(v.pov_str() for v in self.mesh.faces)} }}\n")

        pov.write(f"pigment {{ color rgb {self.pigment_color.pov_str()} }}\n")
        pov.write(f"finish {{ ambient {self.material.ambient} diffuse {self.material.diffuse} specular {self.material.specular} roughness {self.material.roughness} }}\n")

        if self.transformation.translate != Vector3f.zero():
            pov.write(f"translate {self.transformation.translate.pov_str()}\n")

        if self.transformation.rotate != Vector3f.zero():
            pov.write(f"rotate {self.transformation.rotate.pov_str()}\n")

        if self.transformation.scale != Vector3f.one():
            pov.write(f"scale {self.transformation.scale.pov_str()}\n")

        pov.write("}\n")


@dataclass(kw_only=True, frozen=True)
class AreaLight(PovObject):

    @dataclass(kw_only=True, frozen=True)
    class Config:
        color: Color
        size: int
        samples: int

        def __post_init__(self):
            assert self.size > 0
            assert self.samples > 0

    @dataclass(kw_only=True, frozen=True)
    class Description:
        type Entry = Mapping[str, Any]

        config: AreaLight.Config
        position: SpherePosition

        @classmethod
        def from_entry(cls, entry: Entry) -> Self:
            return cls(
                config=AreaLight.Config(
                    color=Color.from_hex(entry["color"]),
                    size=int(entry["size"]),
                    samples=int(entry["samples"]),
                ),
                position=SpherePosition.from_degrees(
                    float(entry["azimuth"]),
                    float(entry["elevation"]),
                ),
            )

    config: Config
    position: Vector3f

    @classmethod
    def from_description(cls, description: Description, distance: float, azimuth_offset: float) -> Self:
        return cls(
            config=description.config,
            position=Vector3f.from_spherical(
                SpherePosition(
                    azimuth=description.position.azimuth + azimuth_offset,
                    elevation=description.position.elevation,
                )
            ) * distance
        )

    def write_pov(self, pov: TextIO) -> None:
        pov.write("light_source {\n")

        pov.write(f"{self.position.pov_str()}\n")
        pov.write(f"color rgb {self.config.color.pov_str()}\n")

        v1 = Vector3f(self.config.size / 2, 0, 0)
        v2 = Vector3f(0, self.config.size / 2, 0)
        pov.write(f"area_light {v1.pov_str()}, {v2.pov_str()}, {self.config.samples}, {self.config.samples}\n")

        pov.write(f"adaptive 1\n")
        pov.write(f"jitter\n")
        pov.write(f"fade_power 0\n")

        pov.write("}\n")


@dataclass(kw_only=True, frozen=True)
class Camera(PovObject):
    location: Vector3f
    look_at: Vector3f
    fov: float

    def write_pov(self, pov: TextIO) -> None:
        pov.write("camera {\n")

        pov.write(f"location {self.location.pov_str()}\n")
        pov.write(f"look_at {self.look_at.pov_str()}\n")

        forward = self.look_at - self.location
        mag = forward.magnitude()

        degenerate = mag > 0 and abs(forward.z / mag) > 0.999

        if degenerate:
            pov.write(f"up {Vector3f(1, 0, 0).pov_str()}\n")
        else:
            pov.write(f"sky {Vector3f(0, 0, 1).pov_str()}\n")

        pov.write("right x*image_width/image_height\n")
        pov.write(f"angle {self.fov}\n")

        pov.write("}\n")


@dataclass(kw_only=True, frozen=True)
class Scene(PovObject):

    @dataclass(kw_only=True, frozen=True)
    class Config:
        background_color: Color
        ambient_light: Color
        assumed_gamma: float
        max_trace_level: int

    config: Config
    camera: Camera
    lights: Sequence[AreaLight]
    solids: Sequence[Solid]

    def write_pov(self, pov: TextIO) -> None:
        pov.write("#version 3.7;\n")
        pov.write("#include \"colors.inc\"\n")

        pov.write(f"global_settings {{ assumed_gamma {self.config.assumed_gamma} ambient_light rgb {self.config.ambient_light.pov_str()} max_trace_level {self.config.max_trace_level} }}\n")
        pov.write(f"background {{ color rgb {self.config.background_color.pov_str()} }}\n")

        for pov_object in chain(
            (self.camera,),
            self.lights,
            self.solids,
        ):
            pov_object.write_pov(pov)

        pov.write("\n")


@dataclass(frozen=True, kw_only=True)
class SceneBuilder:

    @dataclass(frozen=True, kw_only=True)
    class View:

        type Entry = Mapping[str, Any]

        name: str
        position: SpherePosition

        @classmethod
        def from_entry(cls, name: str, entry: Entry) -> Self:
            return cls(
                name=name,
                position=SpherePosition.from_degrees(
                    float(entry["azimuth"]),
                    float(entry["elevation"]),
                ),
            )

    @dataclass(frozen=True, kw_only=True)
    class Config:
        view_fov: float
        view_margin: float
        views: Sequence[SceneBuilder.View]

        light_distance_factor: float
        light_descriptions: Sequence[AreaLight.Description]

        scene_config: Scene.Config

        solid_material: Solid.Material
        solid_pigment_color: Color

    config: Config
    mesh: Mesh

    def build(self, view_position: SpherePosition) -> Scene:
        bound_min, bound_max = self.mesh.bounds()
        center = (bound_min + bound_max) * 0.5

        view_d = (
            self.config.view_margin
            * 0.5
            * (bound_max - bound_min).magnitude()
            / math.tan(math.radians(self.config.view_fov / 2))
        )

        area_lights = map(
            lambda d: AreaLight.from_description(
                d,
                view_d * self.config.light_distance_factor,
                view_position.azimuth,
            ),
            self.config.light_descriptions
        )

        solids = (
            Solid(
                mesh=self.mesh,
                transformation=Solid.Transformation(translate=-center, rotate=Vector3f.zero(), scale=Vector3f.one()),
                material=self.config.solid_material,
                pigment_color=self.config.solid_pigment_color,
            ),
        )

        return Scene(
            config=self.config.scene_config,
            camera=Camera(
                location=Vector3f.from_spherical(view_position) * view_d,
                look_at=Vector3f.zero(),
                fov=self.config.view_fov,
            ),
            lights=tuple(area_lights),
            solids=tuple(solids),
        )


@dataclass(frozen=True, kw_only=True)
class RenderJob:

    obj_file: Path
    work_dir: Path
    project_name: str
    resolution: int
    dry_run: bool
    verbose: bool

    scene_builder: SceneBuilder.Config

    __view_isometric_elevation: ClassVar[Final] = 35.264
    _default_views_set: ClassVar[Final[Mapping[str, SceneBuilder.View.Entry]]] = {
        "top":    {"azimuth": 0, "elevation": 89},
        "bottom": {"azimuth": 0, "elevation": -89},
        "front":  {"azimuth": 0, "elevation": 0},
        "back":   {"azimuth": 180, "elevation": 0},
        "right":  {"azimuth": 90, "elevation": 0},
        "left":   {"azimuth": 270, "elevation": 0},

        "front_top_right": {"azimuth": 45, "elevation":  __view_isometric_elevation},
        "back_top_right":  {"azimuth": 135, "elevation": __view_isometric_elevation},
        "back_top_left":   {"azimuth": 225, "elevation": __view_isometric_elevation},
        "front_top_left":  {"azimuth": 315, "elevation": __view_isometric_elevation},

        "front_bottom_right": {"azimuth": 45, "elevation": -__view_isometric_elevation},
        "back_bottom_right":  {"azimuth": 135, "elevation": -__view_isometric_elevation},
        "back_bottom_left":   {"azimuth": 225, "elevation": -__view_isometric_elevation},
        "front_bottom_left":  {"azimuth": 315, "elevation": -__view_isometric_elevation},
    }

    _default_solid_material: ClassVar[Final[Solid.Material.Entry]] = {
        "ambient": 0.15,
        "diffuse": 0.9,
        "specular": 0.0,
        "roughness": 1.0,
    }

    _default_light_descriptions: ClassVar[Final[Sequence[AreaLight.Description.Entry]]] = (
        {
            "color": "#FFB772",
            "size": 400,
            "samples": 8,
            "azimuth": 0,
            "elevation": 45,
        },
        {
            "color": "#729EFF",
            "size": 500,
            "samples": 6,
            "azimuth": -120,
            "elevation": 25,
        },
        {
            "color": "#8CD8BF",
            "size": 500,
            "samples": 6,
            "azimuth": 120,
            "elevation": 25,
        },
        {
            "color": "#7F7F8C",
            "size": 600,
            "samples": 4,
            "azimuth": 0,
            "elevation": -65,
        },
    )

    @classmethod
    def from_cli(cls, argv: Optional[Sequence[str]] = None) -> Self:
        parser = argparse.ArgumentParser(
            description="Render a part from OBJ via POV-Ray.",
        )

        parser.add_argument("obj", type=Path, metavar="OBJ")
        parser.add_argument("--dry-run", action="store_true")
        parser.add_argument("-o", "--out", type=Path, default=Path.cwd(), metavar="DIR")
        parser.add_argument("-n", "--name", type=str, default=None, metavar="NAME")
        parser.add_argument("-r", "--resolution", type=int, default=1024, metavar="N")
        parser.add_argument("-b", "--background", type=str, default="#000000", metavar="#RRBBGG")
        parser.add_argument("-p", "--default-pigment", type=str, default="#ffffff", metavar="#RRBBGG")
        parser.add_argument("-v", "--verbose", action="store_true")
        parser.add_argument("--views", type=str, default="all", metavar="LIST")
        parser.add_argument("--fov", type=float, default=30, metavar="DEG")
        parser.add_argument("--lights-file", type=Path, default=None, metavar="FILE")
        parser.add_argument("--material-file", type=Path, default=None, metavar="FILE")
        parser.add_argument("--views-file", type=Path, default=None, metavar="FILE")

        ns = parser.parse_args(argv)

        def get_entries[T](views_file: Optional[Path], default: T) -> T:
            if views_file is None:
                return default

            if not views_file.exists():
                parser.error(f"{views_file=!s} not exists.")

            with views_file.open("r") as f:
                return json.load(f)

        def selected_views(active_view_set: Mapping[str, SceneBuilder.View.Entry], names: Iterable[str]) -> Sequence[SceneBuilder.View]:
            names = tuple(active_view_set.keys() if "all" in names else names)

            if unknown := tuple(filter(lambda n: n not in active_view_set, names)):
                parser.error(f"unknown view(s): {', '.join(unknown)}. Available: {', '.join(active_view_set.keys())}")

            return tuple(map(lambda n: SceneBuilder.View.from_entry(n, active_view_set[n]), names))

        return cls(
            obj_file=ns.obj.resolve(),
            work_dir=ns.out.resolve(),
            project_name=ns.name or ns.obj.stem,
            resolution=ns.resolution,
            dry_run=ns.dry_run,
            verbose=ns.verbose,
            scene_builder=SceneBuilder.Config(
                view_fov=ns.fov,
                view_margin=1.05,
                views=selected_views(get_entries(ns.views_file, cls._default_views_set), filter(None, map(str.strip, ns.views.split(",")))),

                light_distance_factor=2.0,
                light_descriptions=tuple(map(AreaLight.Description.from_entry, get_entries(ns.lights_file, cls._default_light_descriptions))),

                scene_config=Scene.Config(
                    assumed_gamma=2.2,
                    background_color=Color.from_hex(ns.background),
                    ambient_light=Color.from_scalar(0.10),
                    max_trace_level=10,
                ),

                solid_material=Solid.Material.from_entry(get_entries(ns.material_file, cls._default_solid_material)),
                solid_pigment_color=Color.from_hex(ns.default_pigment),
            ),
        )


def main() -> int:
    job = RenderJob.from_cli()

    _maybe_builder: Optional[SceneBuilder] = None

    def build_scene(view_position: SpherePosition) -> Scene:
        nonlocal _maybe_builder
        if _maybe_builder is None:
            with open(job.obj_file, "r") as f:
                _maybe_builder = SceneBuilder(config=job.scene_builder, mesh=Mesh.from_obj(f))
        return _maybe_builder.build(view_position)

    total_renders = len(job.scene_builder.views)
    failed_renders = 0

    for i, view in enumerate(job.scene_builder.views):
        stem = job.work_dir / f"{job.project_name}_{view.name}"
        pov_file = stem.with_suffix(".pov")

        if job.dry_run:
            print(pov_file.stem)
            continue

        print(f"[{i + 1}/{total_renders}]: {view.name}")

        print("Building scene.")
        scene = build_scene(view.position)

        print(f"Writing {pov_file}.")

        with pov_file.open("w") as f:
            scene.write_pov(f)

        print("Rendering.")
        r = subprocess.run((
            "povray",
            f"+W{job.resolution}", f"+H{job.resolution}",
            "+A", "+AM2", "+R3",
            f"+I{pov_file}", f"+O{stem}",
            "+L/usr/share/povray-3.7/include",
        ), capture_output=True, text=True, timeout=300)

        print(f"Render complete: {stem.name}.png")

        if job.verbose:
            print(r.stdout)
            print(r.stderr)

        if 0 != r.returncode:
            print(r.stderr)
            failed_renders += 1

        pov_file.unlink()

    return 0 if (failed_renders == 0) else 1


exit(main())
