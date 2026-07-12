# Nano Target

This folder is the Arduino Nano firmware target for NutEngine3D.

## What it does

- includes the compiled scene blob from `build/assets/nano/demo_scene.h`
- reconstructs the scene at runtime with `SceneBinaryLoader`
- renders through a Nano-specific `Graphics` backend

## Current state

The current target is the stabilized Nano path we used to validate the
embedded-first scene pipeline on real hardware.

- scene compiler -> `build/assets/nano/demo_scene.h`
- runtime loader -> `SceneBinaryLoader`
- Nano renderer -> `NutNanoGraphics` + `NutNanoDisplayAdapter`
- current hardware target matches the `NuTetris` setup:
  `SSD1306 128x64` OLED over `I2C`

You may still need to adjust pins or the display adapter for a different OLED module.

## Recommended workflow

Use `PlatformIO` as the primary embedded build path.

- open `targets/nano/` as the PlatformIO project root
- build the `nano` environment
- upload from PlatformIO
- use the serial monitor at `115200`

The Arduino IDE is no longer the primary target for this firmware layout.

## Files

- `src/main.cpp`: PlatformIO/embedded entry point
- `src/NutNanoGraphics.h`: `Graphics` implementation for Arduino
- `src/NutNanoDisplayAdapter.h`: the concrete `SSD1306` display adapter
- `src/NutEngineRuntime.cpp`: bridge that compiles the shared engine sources into the firmware
- `src/NanoNoHeap.cpp`: rejects heap allocation and keeps Arduino's default allocator out of the firmware
- `platformio.ini`: embedded build configuration for `Arduino Nano`

## Display target

The current adapter targets the same display as `NuTetris`:

- controller: `SSD1306`
- physical resolution: `128x64`
- bus: `I2C`
- address: `0x3C`
- Nano pins:
  `VCC -> 5V`, `GND -> GND`, `SDA -> A4`, `SCL -> A5`

The current adapter uses a paged `128x64` framebuffer model and a tuned I2C clock
for the validated Nano setup.

## Stress workflow

The normal `nano` environment keeps the stabilized baseline limits.

Use `nano_stress` when you want to probe larger object/child budgets without
changing the baseline firmware configuration. That environment raises selected
fixed-size runtime limits through build flags so you can map the real hardware
budget more safely.

### Measured gameplay budget

Current real-hardware results on the validated Nano/SSD1306 setup:

| Scenario | Visible objects | Meshes | Scripts | RAM | Result |
| --- | ---: | ---: | ---: | ---: | --- |
| Demo | 2 | 2 | 3 | 1532 B (74.8%) | Smooth and stable |
| Stress 5 | 5 cubes | 1 shared | 1 | 1656 B (80.9%) | Acceptable |
| Stress 5 + scripts | 5 cubes | 1 shared | 3 | 1708 B (83.4%) | Runs, not smooth |
| Stress 4 + scripts | 4 cubes | 1 shared | 3 | 1586 B (77.4%) | Acceptable |
| Mixed geometry | 2 cubes + 2 spheres | 2 | 3 | 1604 B (78.3%) | Acceptable |
| Geometry upper test | 4 spheres | 1 shared | 3 | 1604 B (78.3%) | Acceptable |

For game design, treat four simple visible objects plus about three active
script instances as the current recommended budget. Five visible objects are a
tested upper bound. Objects without meshes mainly cost RAM; each visible mesh
instance repeats vertex/edge rendering and mainly costs CPU.

The comfortable measured geometry range is about 48-84 rendered edges per
frame. Four low-poly spheres, totaling 48 projected vertices and about 120
edges, are still acceptable and currently define the tested upper geometry
budget.

A denser benchmark with 60 projected vertices and about 146 edges also runs at
1604 B RAM (78.3%), but animation is noticeably strained. Treat this as a
conditional limit for slow-paced scenes, not the normal gameplay budget.

Page-local line clipping made this dense test slightly better without adding
RAM. Reusing parent rotation calculations for children with zero local rotation
also remained stable, although the visual gain was not conclusive. The current
CPU optimizations help, but the 146-edge case still is not the recommended game
budget.

### Remaining validation

The next tests should isolate object count, vertex count, edge count, active
scripts, and script-property bytes instead of increasing them together. We also
still need a long-running stability test with continuous joystick input, stack
headroom measurements at each render stage, exact over-limit loader tests, and a
small gameplay prototype that includes input plus collision-like work. Until
those are complete, use the measured table as practical guidance rather than a
guaranteed maximum.

## Compile notes

`src/NutEngineRuntime.cpp` includes the shared engine implementation files from
the main repo, so the embedded target can reuse the same runtime code without
copying the engine into the firmware project.

`platformio.ini` provides the include paths for:

- `../../src`
- `../../build/assets/nano`

The shared script catalog now lives in `../../assets/scripts/ScriptCatalog.h`.
