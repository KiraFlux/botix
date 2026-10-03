# Documentation

Technical documentation for the Botix robot project: parts registry, visual gallery, assembly guide, and firmware developer guidelines.

## Directory Structure

| Path                                                   | Description                                                    |
| ------------------------------------------------------ | -------------------------------------------------------------- |
| `assets/`                                              | Images, renders, and photographs used in the documentation.    |
| [`catalog.md`](catalog.md)                             | Registry of parts, tools, and consumables used in the project. |
| [`gallery.md`](gallery.md)                             | Dated log of robot iterations and hardware revisions.          |
| [`assembly_guide.md`](assembly_guide.md)               | Step‑by‑step assembly instructions. *WIP*                      |
| [`firmware_contributing.md`](firmware_contributing.md) | Guidelines for firmware developers.                            |

## Writing Rules

- **Language**: English. Russian only in `*.ru.md` files.
- **Style**: neutral, concise. No emojis, no exclamation marks.
- **Format**: Markdown. Relative links only. No absolute GitHub URLs.
- **Images**: stored under `assets/`, referenced with `<img>` tags. Width fixed per context (150 px in catalog tables, 300 px in gallery grids, 400 px for hero).
- **Names**: `lower_case` for parts and anchors. Underscores between words. No spaces in filenames.
- **Dates**: `YYYY_MM` in folder and section names (`botix_uno_2026_07`, not `botix-uno-2026-07` or `07/2026`).
- **Anchors**: an anchor must match its visible text exactly — `<a id="platform"></a>platform`. Used for cross‑references from other documents.
- **Check before commit**: spelling, working links, no broken anchors, no references to removed files.

## License

This README and all other documentation in `docs/` are licensed under **CC BY‑SA 4.0** – see the [LICENSE](LICENSE) file for the full text.

For the complete licensing information of all project components, refer to the [root repository README](../README.md#repository-structure--licensing).
