<div align="center">

# Botix

<img src="docs/assets/gallery/botix_uno_2026_07/photo_front_left.jpg" height="400" style="object-fit: cover; display: block;">

**Open‑source educational mobile robot project**

</div>

---

> **[Read this in Russian](README.ru.md)**

<br><br>

<div align="center">

## Overview

</div>

Designed for school robotics clubs, Olympiad preparation (All-Russian Olympiad, Moscow Olympiad, Russian Robotics Olympiad), and fast prototyping at hackathons. All parts are 3D‑printable, electronics are cheap and widely available.

- Two drive motors with encoders (JGA25 12V, 77-170 RPM).
- Optional [two‑degree‑of‑freedom manipulator](docs/gallery.md#botix_esp32_2026_05) (arm + gripper).
- Chassis prints in ~5 hours on a standard FDM printer; flat parts, tongue‑and‑groove assembly with screws.

<br><br>

<div align="center">

## Variants

<table>

<tr>
<td colspan="2" align="center">

**Basic**

</td>
<td colspan="2" align="center">

**Advanced**

</td>
</tr>

<tr>
<td style="padding: 0;"><img src="docs/assets/gallery/botix_uno_2026_07/photo_front_left.jpg" height="300" style="object-fit: cover; display: block;"></td>
<td style="padding: 0;"><img src="docs/assets/gallery/botix_uno_2026_07/photo_back.jpg" height="300" style="object-fit: cover; display: block;"></td>
<td style="padding: 0;"><img src="docs/assets/gallery/botix_esp32_2026_07/photo_front_left.jpg" height="300" style="object-fit: cover; display: block;"></td>
<td style="padding: 0;"><img src="docs/assets/gallery/botix_esp32_2026_07/photo_back.jpg" height="300" style="object-fit: cover; display: block;"></td>
</tr>

<tr valign="top">
<td colspan="2">

**For:** 
- Olympiad practice (VSOSH, MOSH, RRO), learning programming, control algorithms

**Features:**
- Arduino UNO + any motor shield
- Classic competition‑ready setup
- Simple, reliable, well‑documented

</td>
<td colspan="2">

**For:** 
- Complex algorithms, research, hackathons

**Features:**
- ESP32 as main controller (works standalone like Basic)
- Optional **[botix-esp32 firmware](botix-esp32/)** adds:
  - Remote control & telemetry via WiFi or ESP‑NOW
  - Integration with MAVLink (QGC, ROS) or custom raw protocol
  - Persistent configuration & CLI for debugging
  - LIDAR & encoder support

</td>
</tr>

<tr>
<td colspan="2" align="center">

[more_photos](docs/gallery.md#botix_uno_2026_07), 
[assembly_guide](docs/assembly_guide.md)

</td>
<td colspan="2" align="center">

[more_photos](docs/gallery.md#botix_esp32_2026_07), 
[assembly_guide](docs/assembly_guide.md), 
[firmware_docs](botix-esp32/README.md)

</td>
</tr>

</table>

</div>

> View the [printable parts catalog](docs/catalog.md).

<br><br>

<div align="center">

## Hardware

<table>
<tr>
<td align="center">

**Electronics**

</td>
<td align="center">

**Mechanics**

</td>
</tr>

<tr valign="top">
<td>

- **Controller** - any Arduino‑compatible board (Arduino UNO, ESP32, etc.).  
  See the [assembly guide](docs/assembly_guide.md) for specific models and wiring.

- **Power board** - custom design, powered by 3x14500 cells (3S).  
  Three independent regulators:  
  - 2 x Mini560 Pro (5V)  
  - 1 x LM2596S module (adjustable)  

  Variants (see gallery for photos):  
  - [PCB manufacturing](docs/gallery.md#botix_power_v2_pcb_order_2026_07) - production order  
  - [DIY toner transfer (double‑sided)](docs/gallery.md#botix_power_v2_ttm_double_2026_07)  
  - [DIY toner transfer (single‑sided)](docs/gallery.md#botix_power_v2_ttm_single_2026_07)

</td>
<td>

- All 3D models are in [`mcad/`](mcad/).  
- Main assembly: `mcad/src/botix.fcstd`.  
- Off‑the‑shelf component models are in `mcad/lib/`.  

</td>
</tr>

<tr>
<td align="center">

[docs](ecad/README.md)

</td>
<td align="center">

[docs](mcad/README.md)

</td>
</tr>

</table>

</div>

> For visual references of different robot configurations and hardware revisions, see the [project gallery](docs/gallery.md).

<br><br>

<div align="center">

## Software

<table>

<tr>
<td align="center">

**botix-esp32 firmware**

</td>
<td align="center">

**pybotix – host Python API (coming soon)**

</td>
</tr>

<tr valign="top">
<td>

The **[modular firmware](botix-esp32/)** for the Advanced variant is written in C++20 and built with PlatformIO. It provides:

- Interactive CLI for configuration and debugging
- MAVLink and RAW protocols
- ESP‑NOW and WiFi UDP transports
- Persistent configuration in NVS
- Units for motors, servos, encoders, and LIDAR

</td>
<td>

`pybotix` will be a Python package for the **host computer** (PC, Raspberry Pi). It will provide:

- Simple and safe robot control from Python
- Swarm support
- High‑level primitives for navigation and behaviors
- Eventually, block‑based code generation for education

</td>
</tr>

<tr>
<td align="center">

[firmware_docs](botix-esp32/README.md)

</td>
<td align="center">

</td>
</tr>

</table>

</div>

> `pybotix` runs on the host, not on the robot. It communicates with the robot via MAVLink or RAW protocols over WiFi/ESP‑NOW.

<br><br>

<div align="center">

## Who Is This For?

</div>

- **School students** - prepare for practical rounds of robotics Olympiads (VSOSH, MOSH, RRO). The Arduino‑based option fully matches competition regulations.
- **University students** - learn mobile robotics, navigation algorithms.
- **Hackathon participants** - the ESP32 variant with firmware provides a ready‑made infrastructure so you can focus on high‑level logic.
- **Teachers** - quickly set up robotics clubs on a low budget.

<br><br>

<div align="center">

## Quick Start

</div>

1. Read the [assembly guide](docs/assembly_guide.md).
2. Print parts from [`mcad/src/`](mcad/src/). Printable STEP files are attached to the [releases](../../releases).
3. Refer to schematics in [`ecad/`](ecad/) to assemble the electronics.
4. - For Basic: write and upload your own Arduino sketch using the motor shield library.
   - For Advanced: go to [`botix-esp32/`](botix-esp32/) and follow the firmware guide.

Before you start, make sure you have the following software installed:

| Purpose           | Software   | Version |
| ----------------- | ---------- | ------- |
| CAD (mechanics)   | FreeCAD    | 1.1.x   |
| CAD (electronics) | KiCad      | 10.x.x  |
| Slicer            | OrcaSlicer | 2.4.x   |
| Firmware build    | PlatformIO | 6.1.19  |
| Scripts runtime   | Python     | 3.14.x  |
| Render engine     | POV-Ray    | 3.7.x   |

> All tools are free and open‑source.

<br><br>

<div align="center">

## Contributing

Found a bug? Have an idea? Open an Issue or submit a Pull Request.  

</div>

> See [`CONTRIBUTING.md`](CONTRIBUTING.md) for guidelines.

<br><br>

<div align="center">

## Repository Structure & Licensing

| Path                  | License                                 | Description                                        |
| --------------------- | --------------------------------------- | -------------------------------------------------- |
| **Any Markdown file** | [CC BY-SA 4.0](docs/LICENSE)            | Overview, documentation                            |
| `docs/`               | [CC BY-SA 4.0](docs/LICENSE)            | Project documentation.                             |
| `docs/assets/`        | [CC BY-SA 4.0](docs/LICENSE)            | Images, photos, and renders used in documentation. |
| `ecad/`               | [CERN-OHL-S-2.0](ecad/LICENSE)          | PCB sources in KiCad.                              |
| `mcad/`               | [CERN-OHL-S-2.0](mcad/LICENSE)          | 3D models in FreeCAD.                              |
| `botix-esp32/`        | [GPL-3.0-or-later](botix-esp32/LICENSE) | Advanced ESP32 firmware source.                    |
| `tools/`              | [GPL-3.0-or-later](tools/LICENSE)       | Automation scripts.                                |

</div>

> Full texts of all licenses are provided in the respective directories.
