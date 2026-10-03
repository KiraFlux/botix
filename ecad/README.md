# Electronics (KiCad)

KiCad 10.x source files for Botix PCBs.

## Directory Structure

| Path                                | Description                                                                               |
| ----------------------------------- | ----------------------------------------------------------------------------------------- |
| `botix.kicad_sym`                   | Symbol library.                                                                           |
| `botix_library.pretty/`             | Footprint library.                                                                        |
| `botix_power_module_v2_pcb_order/`  | Version 2, optimized for manufacturing (JLCPCB, PCBWay) – use this for production orders. |
| `botix_power_module_v2_ttm_double/` | Version 2, prepared for double‑sided toner transfer.                                      |
| `botix_power_module_v2_ttm_single/` | Version 2, single‑sided TTM variant – simpler for home fabrication.                       |

## Opening

Install **KiCad 10.x**. Open any `.kicad_pro` file in the corresponding folder – that is the main project. Schematics are `.kicad_sch`, PCB is `.kicad_pcb`.

> The `ttm` folders are prepared for DIY toner transfer (paper + laser printer + iron), with single‑ or double‑sided variants.

For detailed electrical schematics and BOM, refer to the [documentation](../docs/).

## License

This README is licensed under [CC BY-SA 4.0](../docs/LICENSE).

The hardware design files in `ecad/` are licensed under [CERN-OHL-S-2.0](LICENSE).

For the complete licensing information of all project components, refer to the [root repository README](../README.md#repository-structure--licensing).
