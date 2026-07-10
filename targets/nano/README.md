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
- `src/NanoAllocator.cpp`: tiny fixed arena used by the Nano target for any remaining global `new/delete`
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

## Compile notes

`src/NutEngineRuntime.cpp` includes the shared engine implementation files from
the main repo, so the embedded target can reuse the same runtime code without
copying the engine into the firmware project.

`platformio.ini` provides the include paths for:

- `../../src`
- `../../build/assets/nano`

The shared script catalog now lives in `../../assets/scripts/ScriptCatalog.h`.
