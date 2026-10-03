# Contributing to Botix

> **[Read this in Russian](CONTRIBUTING.ru.md)**

Thank you for your interest in contributing to Botix! Whether you are fixing a typo, proposing a new feature, or improving the hardware design – your help is invaluable. This document outlines the workflow and expectations for all contributors.

---

## General Rules

- **Be respectful** – follow the [Contributor Covenant Code of Conduct](https://www.contributor-covenant.org/version/2/1/code_of_conduct/).
- **Use issues** – before starting significant work, open an issue to discuss your idea. This avoids duplicate efforts and ensures alignment with the project direction.
- **One logical change per pull request** – keep PRs focused and easy to review.
- **Respect licenses** – each component has its own license (see the root [`README.md`](README.md)). Ensure your contributions are compatible.

---

## Branching Model

The project uses a single long‑lived branch and short‑lived working branches. `main` is always the current, stable state of the project.

- `main` – the single source of truth. Always stable, always buildable. Merges only via pull requests.
- Working branches are **short‑lived**. Create them from `main` and open a pull request **against `main`** when done.
- Branch names must make the **kind of work** obvious at a glance. Use the same types as commit messages:

  | Type        | Meaning                                        |
  | ----------- | ---------------------------------------------- |
  | `feat/`     | New feature or capability                      |
  | `fix/`      | Bug fix or correction                          |
  | `docs/`     | Documentation change                           |
  | `refactor/` | Internal restructuring without behavior change |
  | `chore/`    | Maintenance, dependencies, tooling             |

  Examples: `feat/add-sharp-mount`, `fix/servo-limits`, `docs/typo-assembly-guide`, `refactor/export-script`.

- **Squash on merge.** Each PR becomes one logical commit on `main`.
- **Delete the branch after merge.** Branches are working state, not history.

---

## Subsystem Rules

Each subsystem keeps its own contribution rules in its `README.md`. Read the one for the part of the project you are touching before opening a PR.

| Subsystem       | Rules                                                                                                                 |
| --------------- | --------------------------------------------------------------------------------------------------------------------- |
| Documentation   | [`docs/README.md`](docs/README.md)                                                                                    |
| CAD Reference   | [`cadref/README.md`](cadref/README.md)                                                                                |
| Electronics     | [`ecad/README.md`](ecad/README.md)                                                                                    |
| Mechanics       | [`mcad/README.md`](mcad/README.md)                                                                                    |
| ESP32 Firmware  | [`botix-esp32/README.md`](botix-esp32/README.md) and [`docs/firmware_contributing.md`](docs/firmware_contributing.md) |
| Tooling Scripts | [`tools/README.md`](tools/README.md)                                                                                  |  |

> Nothing subsystem‑specific is duplicated in this file.

---

## Commit and Pull Request Guidelines

- **Base branch is always `main`.** Open your PR against `main`, regardless of the subsystem.
- **One logical commit per feature/fix** (or squash during merge).
- **Commit messages:** use the format `<type>: <short description>` (e.g., `feat: add new lidar driver`, `fix: correct servo limits`, `docs: update assembly guide`).
- In the PR description, mention the affected subsystem and link to any related issue (`Closes #...`).
- Keep PRs small and focused. Large, mixed‑purpose PRs are hard to review and will likely be asked to split.

---

## Continuous Integration

> Not implemented yet. Will be added as the project matures.

Planned checks on every PR against `main`:

- **Markdown and scripts** – validation of formatting and syntax.
- **MCAD pipeline** – triggered by changes under `mcad/`. Regenerates printable STEP files from `mcad/` and part renders for the documentation. Rendering is incremental.
- **Firmware build** – triggered by changes under `botix-esp32/`.

Once CI produces commits to `docs/assets/`, do not be alarmed – this is expected. Pull before continuing your work.

---

## License

This file is part of the project documentation and is licensed under [CC BY‑SA 4.0](docs/LICENSE).

---

If you have any questions, feel free to open an issue or reach out to the maintainers.
