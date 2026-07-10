# Pipeline

This is the current content pipeline used by NutEngine3D.

The project is built around an embedded-first workflow:

- author scenes in a readable format
- compile them into a compact runtime blob
- run that same compiled scene on the Nano and on the desktop simulator

## Source files

These are the files you edit by hand or with the scene editor:

- `assets/scenes/*.nutscene`
- `assets/models/*.obj`
- `assets/scripts/*.h`
- `assets/scripts/*.cpp`

In practice:

- `.nutscene` is the source of truth for scene structure
- `.obj` stores the wireframe meshes
- C++ scripts define runtime behavior

## Generated output

Compiled scene output goes to:

- `build/assets/nano/`
- `build/assets/desktop/`

Typical generated file:

- `build/assets/nano/demo_scene.h`

That header embeds the compiled scene as:

- `uint8_t[]`
- `size_t`

This keeps the Nano runtime simple because the scene can be linked directly into
the firmware image.

## Current flow

1. Edit a scene in `assets/scenes/demo.nutscene`.
2. Reference meshes from `assets/models/*.obj`.
3. Attach scripts by type name such as `SpinScript`.
4. Run `tools/scene_compiler`.
5. The compiler resolves meshes, hierarchy, transforms, script ids, and script config.
6. The compiler writes a generated header into `build/assets/...`.
7. The Nano target and the desktop target both include that generated header.
8. `SceneBinaryLoader` reconstructs the runtime scene from the compiled blob.

## Script mapping

Scripts are authored by name in `.nutscene`, but compiled by id for runtime use.

The shared mapping lives in:

- `assets/scripts/ScriptCatalog.h`

That file is the bridge between:

- authored script names in scenes
- generated script ids in compiled scene data
- native script factories used by the desktop runtime

## Why the split exists

The project keeps two different representations on purpose:

- authored format: readable `.nutscene`
- runtime format: compiled binary scene blob

The Nano benefits from this a lot:

- less parsing work at startup
- less runtime memory pressure
- simpler firmware integration
- one compact format shared by both targets

## Related docs

- `tools/scene_compiler/README.md`: how to build and run the compiler
- `tools/scene_compiler/README.md`: also includes the current binary scene format layout
