# Mechanics (FreeCAD)

> **[Read this in Russian](README.ru.md)**

Source files for all mechanical parts, assemblies and the main assembly of the Botix robot.

## Directory Structure

| Path           | Purpose                                        |
| -------------- | ---------------------------------------------- |
| `botix.fcstd`  | Main assembly of the whole robot               |
| `chassis/`     | Chassis parts and assemblies                   |
| `attachments/` | Attachments and add‑ons mounted on the chassis |

> Models of purchased components and shared geometry live in [`cadref/`](../cadref/).

## Editing

Open or create a file following the example of the existing ones.

A file is either a single part, an assembly, or a library listing several parts. An assembly may contain a group named `printable` with the parts to be printed.

## Export

Generated files are not stored in the repository. Export to the local artifacts directory.

## License

This README is licensed under [CC BY‑SA 4.0](../docs/LICENSE).

The mechanical design files in `mcad/` are licensed under [CERN‑OHL‑S‑2.0](LICENSE).

For the complete licensing information of all project components, refer to the [root repository README](../README.md#repository-structure--licensing).
