# NutEngine3D

NutEngine3D is a minimalist wireframe 3D engine built for tiny displays and constrained hardware.

The main experiment is simple: how far can we push real-time 3D on an `Arduino Nano` driving an `SSD1306 128x64` OLED, while still keeping it readable and reasonably fluid?

<img src="nutengine-demo.gif" alt="NutEngine3D demo" width="280" />

## Why It Exists

NutEngine3D is aimed at people who enjoy:

- writing small and understandable graphics code
- experimenting with 3D on constrained displays
- building retro-futuristic tools and prototypes
- extending a tiny engine instead of wrestling a huge one
- exploring hardware/software boundaries just to see what happens

If you like unusual engines, tiny hardware, and projects that are fun to try just to see if they work, you will probably feel at home here.

The project is built around one constraint: use as much of the Nano as possible without turning the result into a slideshow.

## Current Features (V0)

- Full 3D pipeline with 4x4 matrices (Model, View, Projection).
- Basic scene graph with parent/child object hierarchy.
- `Scene` lifecycle with an engine-owned main loop.
- Camera with perspective control and aspect ratio adaptation.
- Wireframe rendering.
- Compiled scene loading through `SceneBinaryLoader`.
- Script metadata compiled into the scene blob.
- Nano-specific paged rendering path for `SSD1306`.
- Desktop simulator that runs the same compiled scene blob.
- Separate scene compiler and scene editor under `tools/`.

## Current Nano Footprint

Latest validated Nano firmware target:

- Flash: about `16702 / 30720 bytes` (`54%`)
- RAM: about `1580 / 2048 bytes` (`77%`)

Those numbers matter because a lot of the architecture is there simply to stay inside those limits while keeping decent motion on the real display.

## Current Architecture

- `Engine`: owns the application loop and renders the active scene.
- `SceneManager`: registers scenes and performs scene transitions.
- `Scene`: represents a game world/level authored by the user and stores its root objects.
- `GameObject`: represents gameplay objects such as players, enemies, props, or items. It stores transform/mesh data and supports parent/child scene graph relationships.
- `Script`: reusable C++ behavior attached to a `GameObject` and updated once per frame.
- `ObjLoader`: imports simple static `.obj` meshes by reading vertex positions and wireframe edges.
- `scene/`: contains compiled scene runtime classes such as `SceneBinaryLoader` and `RuntimeScene`.
- `assets/scripts/`: contains the user-authored C++ gameplay scripts plus the catalog used to map names, ids, and native factories.

Scripts can find and interact with other objects in the same scene:

```cpp
nut::GameObject* door = findObject("Door");
if (door) {
    door->transform.position.y += 1.0f;
}
```

OBJ loading is intentionally small and wireframe-oriented:

```cpp
nut::Mesh mesh;
if (nut::ObjLoader::load("assets/models/cube.obj", mesh)) {
    object.mesh = &mesh;
}
```

Use low-poly static OBJ files for now. Materials, textures, animation, and complex models are outside the current loader.

## Scene Files

Scenes are authored as JSON using the `.nutscene` format, so the content stays readable and easy to hand-edit.

An authored scene looks like this:

```json
{
  "name": "Demo",
  "camera": {
    "position": [0, 0, -8],
    "rotation": [0, 0, 0]
  },
  "objects": [
    {
      "name": "OrbitPivot",
      "scale": [1, 1, 1],
      "scripts": [
        {
          "type": "SpinScript",
          "rotationSpeed": [0.6, 1.2, 0]
        }
      ],
      "children": [
        {
          "name": "CenterCube",
          "mesh": "assets/models/cube.obj",
          "scale": [0.08, 0.08, 0.08]
        }
      ]
    }
  ]
}
```

Current script source convention:

- Native gameplay scripts live in `assets/scripts/`.
- The scene compiler maps authored script names to compiled script ids using `assets/scripts/ScriptCatalog.h`.
- The scene editor currently resolves script source files by convention as `assets/scripts/<Type>.cpp`.
- Header files follow the same convention as `assets/scripts/<Type>.h`.

Example:

- `SpinScript` -> `assets/scripts/SpinScript.cpp`
- `SpinScript` -> `assets/scripts/SpinScript.h`

## How Scene Loading Works

The scene file is the thing you edit, but the runtime path is compiled:

1. The user edits `assets/scenes/*.nutscene`.
2. `tools/scene_compiler` or `tools/scene_editor` reads that JSON scene.
3. The shared compiler pipeline resolves mesh references, hierarchy, transforms, script ids, and script config.
4. The pipeline emits generated headers such as:
   - `build/assets/nano/demo_scene.h`
   - `build/assets/nano/demo_scene_limits.h`
5. The Nano firmware and desktop simulator both include the compiled scene header.
6. The Nano target consumes the generated limits header before the shared runtime defaults.
7. `SceneBinaryLoader` reconstructs the runtime scene from the compiled binary blob.

So in practice there are two formats:

- Authored format: `.nutscene`
- Runtime format: compiled scene binary embedded as `uint8_t[]`

That split is one of the main reasons the Nano version works.

## Targets

The project is organized by runtime target:

- `targets/desktop/`: desktop simulator entry point and helper scripts
- `targets/nano/`: PlatformIO firmware target for Arduino Nano
- `src/`: shared engine/runtime code
- `tools/`: authoring and build pipeline tools

## Nano Target

The main target is `targets/nano/` with PlatformIO.

Current validated hardware setup:

- controller: `SSD1306`
- resolution: `128x64`
- bus: `I2C`
- address: `0x3C`
- Nano pins: `VCC -> 5V`, `GND -> GND`, `SDA -> A4`, `SCL -> A5`

The Nano target includes its own display adapter, Nano `Graphics` implementation, allocator, and runtime bridge.

Recommended workflow:

1. Open `targets/nano/` in VS Code with PlatformIO.
2. Build the `nano` environment.
3. Upload to the board.
4. Open the serial monitor at `115200`.

There is also a helper script for regenerating the demo scene header:

- `targets/nano/compile_demo_scene.bat`

## Desktop Simulator

The desktop executable is useful as a simulator for the same compiled-scene path used by the Nano target.

Build with CMake:

```bash
cmake -B build
cmake --build build
```

On Windows, desktop helper scripts live under:

- `targets/desktop/build.bat`
- `targets/desktop/run.bat`

## Tooling

## Scene compiler

Build:

```bash
cmake -S tools/scene_compiler -B build/scene_compiler
cmake --build build/scene_compiler
```

Run:

```bash
build/scene_compiler/NutSceneCompiler assets/scenes/demo.nutscene build/assets/nano/demo_scene.h
```

On Windows there are also:

- `tools/scene_compiler/build.bat`
- `tools/scene_compiler/run.bat`

## Scene editor

The editor lives in `tools/scene_editor/`.

It edits the authored `.nutscene` source, not the compiled header. That keeps the authoring side readable while the runtime stays compact.

The current editor workflow is:

1. select `demo.nutscene` or `tunnel_run.nutscene`
2. save the authored scene JSON
3. compile the active scene into the generated Nano headers
4. build/upload the Nano firmware or run the desktop simulator

The selected scene plus local build settings are stored in
`tools/scene_editor/editor_user_settings.json`, which is ignored from git.

## Notes On Performance

Recent work has focused on:

- reducing Nano RAM pressure
- avoiding unnecessary runtime allocations
- caching projected lines before page presentation
- early line rejection for OLED pages
- tuned `I2C` clock for faster display transfer
- simplifying geometry to preserve animation fluency

The goal is not to pack in every feature. The goal is to get the most convincing 3D scene we can out of the Nano while it still feels good to watch.

## Next Steps

Good next areas from here:

- more scene content, including intro and game-over scenes
- smarter frame pacing and selective frame skip
- dirty-page / partial redraw experiments
- more compact mesh formats
- stronger script compilation for Nano-first behaviors
- eventual expansion to a roomier ESP runtime while preserving the same toolchain ideas
