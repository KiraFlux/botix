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
| `lib/actuator/`       | Motors, servos                                                |                                                      |
| `lib/devboard/`       | Development boards                                            |                                                      |
| `lib/sensors/`        | Sensors                                                       |                                                      |
| `lib/misc/`           | Wheels, breadboards, other purchased parts                    |                                                      |
| `lib/fasteners.fcstd` | Bushing and fastener library                                  |                                                      |
| `lib/cutouts.fcstd`   | Reusable cutout shapes for parametric parts                   |                                                      |
| `import/`             | Temporary STEP imports from the legacy CAD system (KOMPAS‑3D) | Removed once migration is complete                   |

## Editing

Install **FreeCAD 1.1.x** or newer. Open `src/botix.fcstd`. All external references use relative paths and work as long as the directory structure is preserved.

Adding a new part:

1. Create the part as a separate `.fcstd` file in `src/chassis/` or `src/attachments/`.
2. Link it into `src/botix.fcstd` and verify fit against the library models in `lib/`.

Do not commit generated files – they are produced from `src/`.

## Contributing

See the [general contributing guidelines](../CONTRIBUTING.md) and the [3D Models section](../CONTRIBUTING.md#3d-models-mcad).

## License

This README is licensed under [CC BY-SA 4.0](../docs/LICENSE).

The mechanical design files in `mcad/` – including FreeCAD models and generated STEP files – are licensed under [CERN-OHL-S-2.0](LICENSE).

For the complete licensing information of all project components, refer to the [root repository README](../README.md#repository-structure--licensing).
