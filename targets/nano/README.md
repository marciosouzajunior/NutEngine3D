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
- generated scene limits -> `build/assets/nano/demo_scene_limits.h`
- runtime loader -> `SceneBinaryLoader`
- Nano renderer -> `NanoGraphics` + `NanoDisplayAdapter`
- current hardware target matches the `NuTetris` setup:
  `SSD1306 128x64` OLED over `I2C`

You may still need to adjust pins or the display adapter for a different OLED module.

## Recommended workflow

Use `PlatformIO` as the primary embedded build path.

- open `targets/nano/` as the PlatformIO project root
- build the `nano` environment
- upload from PlatformIO
- use the serial monitor at `115200`

The normal authoring flow now comes from the scene editor or shared compiler
pipeline rather than manual `platformio.ini` profile switching.

The Arduino IDE is no longer the primary target for this firmware layout.

## Files

- `src/main.cpp`: PlatformIO/embedded entry point
- `src/NanoGraphics.h`: `Graphics` implementation for Arduino
- `src/NanoDisplayAdapter.h`: the concrete `SSD1306` display adapter
- `src/NanoEngineRuntime.cpp`: bridge that selects the shared engine sources and native scripts compiled into the firmware
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

## Runtime configuration

The PlatformIO project now keeps one normal Nano environment:

- `nano`

The active scene is chosen by regenerating:

- `build/assets/nano/demo_scene.h`
- `build/assets/nano/demo_scene_limits.h`

The generated limits header defines the `NUT_CFG_*` capacities for the current
scene. The core still consumes those values through `RuntimeLimits.h`, but the
target Nano owns which generated file is included before the shared runtime.

When the editor detects that `demo_scene_limits.h` changed, it automatically
runs `PlatformIO clean` before the next Nano build or upload so the firmware is
rebuilt against the new limits reliably.

Reference measurements still come from clean builds on the validated
Nano/SSD1306 setup:

- `demo.nutscene`: 1453 bytes RAM (70.9%)
- `tunnel_run.nutscene`: 1602 bytes RAM (78.2%)

Historical performance findings and rejected optimizations are retained in the
root `AGENTS.md` engineering guide. Temporary benchmark scenes and PlatformIO
environments are not kept in the main project after their conclusions have
been incorporated there.

## Compile notes

`src/NanoEngineRuntime.cpp` includes the shared engine implementation files and
explicitly lists the native scripts available to the scenes shipped in that
firmware. Add a script there only when the firmware needs it; the compiled scene
contains the script ID and properties, but not its executable C++ code.

`platformio.ini` provides the include paths for:

- `../../src`
- `../../build/assets/nano`

The shared script catalog now lives in `../../assets/scripts/ScriptCatalog.h`.
