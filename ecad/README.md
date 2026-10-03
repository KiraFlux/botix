# Electronics (KiCad)

> **[Read this in Russian](README.ru.md)**

Source files for all Botix PCBs.

## Directory Structure

| Path                                | Purpose                                           |
| ----------------------------------- | ------------------------------------------------- |
| `botix.kicad_sym`                   | Symbol library                                    |
| `botix_library.pretty/`             | Footprint library                                 |
| `botix_power_module_v2_pcb_order/`  | Manufacturing order (JLCPCB, PCBWay)              |
| `botix_power_module_v2_ttm_double/` | Double‑sided toner transfer                       |
| `botix_power_module_v2_ttm_single/` | Single‑sided toner transfer, simpler for home fab |

> The `ttm` folders are prepared for DIY toner transfer (paper + laser printer + iron).

## Editing

Open or create a file following the example of the existing ones.

A `.kicad_pro` file is the project. Schematics are `.kicad_sch`, PCB is `.kicad_pcb`.

## Export

Generated files are not stored in the repository. Export to the local artifacts directory.

## License

This README is licensed under [CC BY‑SA 4.0](../docs/LICENSE).

The hardware design files in `ecad/` are licensed under [CERN‑OHL‑S‑2.0](LICENSE).

For the complete licensing information of all project components, refer to the [root repository README](../README.md#repository-structure--licensing).
