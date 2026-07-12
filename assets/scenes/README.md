# Scenes

These scenes exist to validate the real Nano runtime on hardware.

Current authored baseline:

- `demo.nutscene`: current reference scene for Nano. It follows the two-level
  hierarchy rule and is expected to load and render normally.

Stress scenes still kept on purpose:

- `stress_children_4.nutscene`: valid two-level stress with 4 child objects
- `stress_children_6.nutscene`: valid two-level stress with 6 child objects
- `stress_children_8.nutscene`: valid two-level stress with 8 child objects
- `stress_children_mixed_6.nutscene`: two-level stress that mixes cubes and light spheres
- `stress_objects_4.nutscene`: pushes object/root-object counts
- `stress_scripts_4.nutscene`: pushes compiled script-record count
- `stress_draw_dual_sphere.nutscene`: stays close to current object limits but raises draw cost

## Nano hierarchy rule

For the Nano target, scenes support only:

- root objects
- one direct child level under each root

Scenes with grandchildren are now rejected by the scene compiler instead of
being kept as runtime stress cases.

For broader Nano experiments, prefer the `stress_children_*` scenes because
they stay inside the supported hierarchy model.

## How to use

Compile one of these scenes into the Nano header:

```bat
tools\scene_compiler\run_scene.bat assets\scenes\stress_objects_4.nutscene
```

That overwrites:

```text
build/assets/nano/demo_scene.h
```

Then build or upload the Nano target:

```bat
C:\Users\User\.platformio\penv\Scripts\platformio.exe run -d targets\nano -e nano
```

For runtime metrics, use the stress environment:

```bat
C:\Users\User\.platformio\penv\Scripts\platformio.exe run -d targets\nano -e nano_stress -t upload
```

## What to record

- flash usage
- RAM usage
- whether the scene loads successfully
- whether the board resets or corrupts serial output
- average frame time
- attempted and drawn edge counts
