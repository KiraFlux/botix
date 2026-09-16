# Mechanics (FreeCAD)

<img src="../docs/assets/freecad/botix_assemble_view.jpg" height="400" style="object-fit: cover; display: block;">


FreeCAD 1.1.x 3D models of all mechanical parts for the Botix robot.

## Directory Structure

| Path                  | Description                                                                                                                                                                                                |
| --------------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `botix.fcstd`         | **Main assembly** – the whole robot. All parts and subassemblies are linked here.                                                                                                                          |
| `export/`             | **Printable** STEP files from FreeCAD via export script.                                                                                                                                                   |
| `import/`             | **Temporary** STEP files imported from KOMPAS‑3D. Used as external references during the migration to FreeCAD. Will be removed once all parts are rebuilt natively in FreeCAD.                             |
| `import/chassis/`     | Chassis parts (frame, platform, walls).                                                                                                                                                                    |
| `import/manipulator/` | Manipulator parts (levers, gears, covers).                                                                                                                                                                 |
| `import/fasteners/`   | Bushings and fasteners.                                                                                                                                                                                    |
| `import/mounts/`      | Sensor mounts (ultrasonic, Sharp).                                                                                                                                                                         |
| `models/`             | **Permanent** 3D models of purchased components (motors, boards, sensors) – **not printable**, used as references in the assembly. These are kept as external references and are not meant to be modified. |
| `models/devboards/`   | Development boards (Arduino Uno, ESP32, etc.).                                                                                                                                                             |
| `models/motors/`      | Motors GA25, servos MG90S, MG995.                                                                                                                                                                          |
| `models/power/`       | Power modules and voltage regulators.                                                                                                                                                                      |
| `models/sensors/`     | Sensors (HCSR04, LD06 LIDAR, Sharp, TCRT5000, VL53L1X, IMU).                                                                                                                                               |
| `models/misc/`        | Other parts (wheels, couplings, breadboards).                                                                                                                                                              |

## Notes on Directories

- **`import/`** – This folder is temporary. It contains STEP exports from the legacy CAD system (KOMPAS‑3D). They are referenced in `botix.fcstd` until each part is redesigned parametrically inside FreeCAD. Once all parts are redesigned parametrically inside FreeCAD and verified against the assembly, this folder will be deleted. Until then, it serves as a reference for the legacy design.
- **`models/`** – This folder is permanent. It contains STEP models of off‑the‑shelf components that are not 3D‑printed. They are used as external references to verify fit and clearances. Some models are taken from open libraries (GrabCAD, 3D ContentCentral, etc.) and retain original author information where available. If you find a model without proper attribution, please [open an issue](../../issues) so we can add it.

## Editing

Install **FreeCAD 1.1.x** or newer. Open `botix.fcstd`. All external references (STEP files from `import/` and `models/`) use relative paths and should work as long as the directory structure is preserved.

## Export for Printing

Use the automation script from the [`tools/`](../tools/) directory to generate STEP files for all printable parts.

## Contributing

If you want to add a new 3D model or improve the assembly, please read the [general contributing guidelines](../CONTRIBUTING.md) and the [3D Models section](../CONTRIBUTING.md#3d-models-mcad) for specific rules.

## License

This README file is part of the project documentation and is licensed under [CC BY-SA 4.0](../docs/LICENSE).

The mechanical design files in this directory (`mcad/`) – including FreeCAD models and generated STEP files – are distributed under [CERN-OHL-S-2.0](LICENSE).

For the complete licensing information of all project components, refer to the [root repository README](../README.md#repository-structure--licensing).

