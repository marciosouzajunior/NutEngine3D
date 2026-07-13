# Scenes

The repository keeps two authored scenes:

- `demo.nutscene`: rotating cube-and-sphere visual demo.
- `tunnel_run.nutscene`: joystick-controlled avoidance game and gameplay proof
  of concept.

Both follow the Nano hierarchy rule: root objects may have direct children,
but those children cannot have children of their own.

## Compile the visual demo

```bat
tools\scene_compiler\run_scene.bat assets\scenes\demo.nutscene
C:\Users\User\.platformio\penv\Scripts\platformio.exe run -d targets\nano -e nano -t upload
```

## Compile Tunnel Run

```bat
tools\scene_compiler\run_scene.bat assets\scenes\tunnel_run.nutscene
C:\Users\User\.platformio\penv\Scripts\platformio.exe run -d targets\nano -e nano_tunnel_run -t upload
```

The scene compiler always overwrites:

```text
build/assets/nano/demo_scene.h
```

Compile the intended scene immediately before building its matching PlatformIO
environment. The environment limits are part of that scene's Nano memory
budget; mixing a generated header with the wrong environment may be rejected by
the loader or reserve unnecessary SRAM.
