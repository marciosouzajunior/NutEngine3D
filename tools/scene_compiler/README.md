# NutSceneCompiler

Small standalone scene compiler for the embedded-first asset pipeline.

For the full authored-assets to runtime-assets flow, see:

- `docs/pipeline.md`

## Purpose

Reads a source `.nutscene` from `assets/` and writes a generated header with
the compiled scene blob ready to be included in firmware.

## Build

```bash
cmake -S tools/scene_compiler -B build/scene_compiler
cmake --build build/scene_compiler
```

## Run

```bash
build/scene_compiler/NutSceneCompiler assets/scenes/demo.nutscene build/assets/nano/demo_scene.h
```

## Windows helpers

On Windows, you can use:

- `tools/scene_compiler/build.bat`
- `tools/scene_compiler/run.bat`
- `tools/scene_compiler/run_scene.bat`

The run helper compiles `assets/scenes/demo.nutscene` into
`build/assets/nano/demo_scene.h`.

The generic scene helper lets you replace the Nano scene header with any
authored scene:

```bat
tools\scene_compiler\run_scene.bat assets\scenes\tunnel_run.nutscene
```

## Nano hierarchy rule

For the Nano target, scenes currently support only:

- root objects
- one direct child level under each root

Grandchildren are rejected by the compiler with a clear error message. This
keeps the Nano traversal and memory usage predictable.

## Current output format

The current output is `NutSceneBinaryV0` emitted as a C/C++ header.

- the source `.nutscene` remains the only authored JSON format
- the compiler flattens hierarchy, resolves mesh IDs, and serializes script config
- the generated header embeds the binary blob as a `uint8_t[]`

This keeps the Nano path simple because the compiled scene can ship inside the
firmware without requiring a runtime filesystem.

## Scene binary format

`NutSceneBinaryV0` is the current compiled runtime scene format emitted by this
tool.

The authored source format remains `.nutscene`. The compiler converts that JSON
into a binary blob and emits it as a generated C/C++ header for runtime
inclusion.

### Delivery form

Example generated file:

- `build/assets/nano/demo_scene.h`

That file contains:

- `uint8_t <name>_data[] PROGMEM`
- `size_t <name>_size`

### Binary layout

All numeric fields are little-endian.

#### Header

```text
magic[4]           "NUT0"
version            u16
stringTableSize    u16
meshCount          u16
objectCount        u16
scriptCount        u16
meshGeometrySize   u16
scriptConfigSize   u16
cameraPosX         f32
cameraPosY         f32
cameraPosZ         f32
cameraRotX         f32
cameraRotY         f32
cameraRotZ         f32
cameraFovDegrees   f32
```

#### Sections after the header

1. String table
2. Mesh path table
3. Mesh geometry blob
4. Object records
5. Script records
6. Script config blob

### Record shapes

#### Mesh record

```text
pathOffset         u16
vertexCount        u16
edgeCount          u16
```

The mesh geometry blob stores, in mesh order:

1. `vertexCount` vertices as `f32 x, y, z`
2. `edgeCount` edges as `u16 a, b`

#### Object record

```text
nameOffset         u16
parentIndex        i16
meshIndex          i16
firstScriptIndex   u16
scriptCount        u16
position[3]        f32 x3
rotation[3]        f32 x3
scale[3]           f32 x3
```

#### Script record

```text
scriptId           u16
configOffset       u16
configSize         u16
```

Known script ids and native script-name mappings are defined in:

- `assets/scripts/ScriptCatalog.h`

### Script config encoding

Script configs are serialized as a compact binary JSON tree:

- `0` = null
- `1` = bool false
- `2` = bool true
- `3` = number (`f32`)
- `4` = string (`u16 stringOffset`)
- `5` = array (`u16 count`, followed by encoded child values)
- `6` = object (`u16 count`, followed by `u16 keyOffset + encoded value`)

This keeps `.nutscene` as the only human-authored JSON while avoiding raw JSON
text inside the runtime asset.
