# Game scripts

This directory contains the native C++ behavior of the game. These files are
compiled into the Nano firmware and into the desktop simulator. They are not
copied into the compiled scene header.

That distinction is important:

- `demo_scene.h` describes which objects and scripts exist, plus their values.
- `firmware.hex` contains the compiled machine code that executes those scripts.

The Nano cannot compile C++, load code from a filesystem, or interpret the
`.nutscene` JSON at runtime. The firmware and scene data are therefore built
separately and joined through compact numeric script IDs.

## Complete flow

Given this authored scene entry:

```json
{
  "type": "TunnelRunScript",
  "baseSpeed": 2.4,
  "speedStep": 0.12,
  "collisionRadius": 1.25
}
```

the pipeline works as follows:

```text
demo.nutscene
    |
    | "TunnelRunScript"
    v
ScriptCatalog.h
    |
    | name -> script ID 4
    v
NutSceneCompiler
    |
    | writes ID 4 + 12 config bytes
    v
demo_scene.h (scene data in PROGMEM)
    |
    | loaded when the Nano starts
    v
SceneBinaryLoader
    |
    | creates CompiledScriptInstance
    v
GameScriptDispatcher
    |
    | ID 4 -> TunnelRunScript::update(...)
    v
TunnelRunScript.cpp (native code in firmware.hex)
```

### 1. Authoring

The `.nutscene` file refers to a script by its readable type name. It also
contains the per-instance properties that should be editable for that scene.

### 2. Scene compilation

`ScriptCatalog.h` maps the authored name to a stable numeric ID. The scene
compiler writes that ID and the serialized properties into the binary scene
blob. It does not copy C++ source or machine code into the scene.

For `TunnelRunScript`, the binary scene stores three floats:

```text
scriptId = 4
config   = baseSpeed, speedStep, collisionRadius
```

### 3. Firmware compilation

The normal C++ compiler compiles `TunnelRunScript.cpp` and the other selected
script modules. The Nano selection is explicit in
`targets/nano/src/NanoEngineRuntime.cpp`; desktop lists its available scripts in
the root `CMakeLists.txt`. Their machine code becomes part of the final
executable or `firmware.hex`.

### 4. Scene loading

`SceneBinaryLoader` reads the script ID and copies at most 12 configuration
bytes into a core `CompiledScriptInstance`. The loader deliberately does not
include game script headers and does not interpret those bytes.

One instance occupies 20 bytes and contains:

- the target object index;
- the script ID;
- 12 opaque configuration bytes;
- 4 bytes of mutable runtime state.

### 5. Runtime dispatch

Once per logical frame, `GameScriptDispatcher` visits the loaded instances. Its
static switch maps each ID to the matching script module. The selected module
interprets the configuration and state, then updates its `GameObject`.

This explicit switch is intentional on the Nano. It avoids heap allocation,
registries, factories, virtual script instances, and runtime code loading.

## Responsibilities

### Core engine

The engine owns generic concepts only:

- `CompiledScriptInstance` storage;
- scene objects and transforms;
- input snapshots;
- binary loading;
- rendering.

The core must not include files from `assets/scripts` or contain cases for
specific script IDs.

### Game scripts

Files in this directory own:

- script IDs and authored names;
- the meaning of configuration properties;
- the meaning of mutable state slots;
- gameplay behavior;
- the static game script catalog and dispatcher.

For example, obstacle lanes, collision rules, difficulty, and game-over logic
belong to `TunnelRunScript.cpp`, not to `RuntimeScene`.

## Utility scripts

The current reusable script set is:

- `GameControllerScript`: placeholder root gameplay script for brand-new scenes.
- `SpinScript`: rotates an object from a `rotationSpeed` vec3.
- `AutoTranslateScript`: moves an object from a `unitsPerSecond` vec3.
- `ClampPositionScript`: clamps one selected position axis between `minValue` and `maxValue`.
- `PulseScaleScript`: animates a uniform scale pulse from `speed`, `minScale`, and `maxScale`.
- `WrapPositionScript`: wraps one selected axis between `minValue` and `maxValue`.
- `PlayerMoveScript`: applies joystick-driven movement for authored prototypes.
- `TunnelRunScript`: owns the full Tunnel Run gameplay loop.

New scenes created in the editor now start with one root `Game` object and a
`GameControllerScript` already attached. It is intentionally a no-op native
script so a new project can begin by editing one visible source file and
keeping the same scene compiler/runtime flow used by shipped content.

## Adding a script

To add another native script:

1. Create `MyScript.h` with a unique ID, authored type name, and `update()`.
2. Implement the behavior in `MyScript.cpp`.
3. Add its name/ID mapping to `ScriptCatalog.h`.
4. Add its property serialization to `NutSceneCompiler`.
5. Add one dispatch case to `GameScriptDispatcher.cpp`.
6. Add the `.cpp` to `NanoEngineRuntime.cpp` and to desktop `CMakeLists.txt` if
   that target should simulate scenes using the script.
7. Use its type and properties in a `.nutscene` file.
8. Rebuild the scene compiler, regenerate `demo_scene.h`, then rebuild firmware.

Configuration is currently limited to 12 bytes per instance. Mutable state is
limited to `stateWord`, `stateFlags`, and `stateByte` (4 bytes total). Increase
neither limit without measuring Nano SRAM and updating the binary/runtime
contract deliberately.

## Why not put script code in the scene header?

The generated header is a C++ representation of binary scene data, not a
program module. Embedding script source there would still require recompiling
the complete firmware, would mix generated data with authored behavior, and
would not enable runtime loading on the Nano.

Keeping code and scene data separate gives us:

- readable scripts under version control;
- one compact scene format shared by Nano and desktop;
- deterministic, allocation-free dispatch;
- clear ownership between engine infrastructure and game behavior.
