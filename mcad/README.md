# Mechanics (FreeCAD)

<img src="../docs/assets/freecad/botix_assemble_view.jpg" height="400" style="object-fit: cover; display: block;">

FreeCAD 1.1.x 3D models of all mechanical parts for the Botix robot.

## Directory Structure

| Path                  | Purpose                                                       | Note                                                 |
| --------------------- | ------------------------------------------------------------- | ---------------------------------------------------- |
| `src/`                | Source of truth: all parts and assemblies                     | One `.fcstd` per part, linked into the main assembly |
| `src/botix.fcstd`     | Main assembly of the whole robot                              | References every part and library model              |
| `src/chassis/`        | Chassis assemblies and printable parts                        |                                                      |
| `src/attachments/`    | Attachments and add‑ons mounted on the chassis                |                                                      |
| `lib/`                | Reusable geometry: off‑the‑shelf components and common shapes | Not printable                                        |
| `lib/fasteners.fcstd` | Bushing and fastener library                                  |                                                      |
| `lib/cutouts.fcstd`   | Reusable cutout shapes for parametric parts                   |                                                      |
| `lib/actuator/`       | Motors, servos                                                |                                                      |
| `lib/devboard/`       | Development boards                                            |                                                      |
| `lib/sensors/`        | Sensors                                                       |                                                      |
| `lib/misc/`           | Wheels, breadboards, other purchased parts                    |                                                      |

## Editing

Install **FreeCAD 1.1.x** or newer. Open `src/botix.fcstd`. All external references use relative paths and work as long as the directory structure is preserved.

Adding a new part:

1. Create the part as a separate `.fcstd` file in `src/chassis/` or `src/attachments/`.
2. Link it into `src/botix.fcstd` and verify fit against the library models in `lib/`.

## Export

Printable STEP files are generated from `src/` and attached to GitHub Releases. They are not committed to the repository.

## License

This README is licensed under [CC BY-SA 4.0](../docs/LICENSE).

The mechanical design files in `mcad/` are licensed under [CERN-OHL-S-2.0](LICENSE).

For the complete licensing information of all project components, refer to the [root repository README](../README.md#repository-structure--licensing).
