# NutEngine3D V0

NutEngine3D is an educational 3D mini-engine focused on learning computer graphics and future portability to microcontrollers (ESP32/Arduino) with TFT displays.

## Current Features (V0)
- Full 3D Pipeline with 4x4 Matrices (Model, View, Projection).
- Basic Scene Graph (parent/child object hierarchy).
- `Scene` lifecycle with engine-owned main loop.
- Camera with perspective control and Aspect Ratio adaptation (ideal for non-square TFT screens).
- Wireframe rendering.
- Abstracted drawing backend (currently running Raylib on PC for easier testing, but ready for `TFT_eSPI`).

## Current Architecture

- `Engine`: owns the application loop and renders the active scene.
- `SceneManager`: registers scenes and performs scene transitions.
- `Scene`: represents a game world/level authored by the user and stores its root objects.
- `GameObject`: represents gameplay objects such as players, enemies, props, or items. It stores transform/mesh data and supports parent/child scene graph relationships.
- `Script`: reusable C++ behavior attached to a `GameObject` and updated once per frame.
- `ObjLoader`: imports simple static `.obj` meshes by reading vertex positions and face outlines.
- `scene/`: contains JSON scene loading classes such as `SceneLoader`, `LoadedScene`, and `ScriptRegistry`.
- `game_scripts/`: contains C++ scripts for the current game/demo, such as `SpinScript`.

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

Scenes can be described in JSON using the `.nutscene` format. The current demo loads `assets/scenes/demo.nutscene`.

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

Scripts used by scene files must be registered by the game before loading:

```cpp
nut::ScriptRegistry scripts;
scripts.registerScript("SpinScript", SpinScript::createFromConfig);

nut::LoadedScene scene;
nut::SceneLoader::load("assets/scenes/demo.nutscene", scene, scripts);
```

## How Scene Loading Works

The scene file is data. It describes objects, transforms, meshes, scripts, and hierarchy. The C++ code still provides the real script classes.

Runtime loading follows this flow:

1. `main.cpp` creates a `ScriptRegistry`.
2. The game registers each script class that JSON files are allowed to use.
3. `SceneLoader` reads the `.nutscene` JSON file.
4. `SceneLoader` creates a `LoadedScene`.
5. For every JSON object, `SceneLoader` creates a `GameObject`.
6. If the object has a `mesh` path, `ObjLoader` imports that `.obj` into a `Mesh`.
7. If the object has `scripts`, `SceneLoader` asks `ScriptRegistry` to create each C++ script.
8. `LoadedScene` owns the created objects, meshes, and scripts so they stay alive while the scene runs.
9. `SceneManager` activates the scene, starts object scripts, and updates them every frame.

Example script registration:

```cpp
nut::ScriptRegistry scripts;
scripts.registerScript("SpinScript", SpinScript::createFromConfig);
```

Example JSON script entry:

```json
{
  "type": "SpinScript",
  "rotationSpeed": [0.6, 1.2, 0]
}
```

When the loader sees `"type": "SpinScript"`, it does not create the class directly from the string. Instead, it asks the registry:

```text
"SpinScript" -> SpinScript::createFromConfig(config)
```

Then `SpinScript::createFromConfig` reads its JSON parameters and returns a new `SpinScript` instance.

```cpp
std::unique_ptr<nut::Script> SpinScript::createFromConfig(const nut::Json& config) {
    nut::math::Vec3 rotationSpeed = readVec3(config.get("rotationSpeed"), nut::math::Vec3(0, 0, 0));
    return std::make_unique<SpinScript>(rotationSpeed);
}
```

This keeps the engine independent from game-specific scripts. The engine knows how to load scenes and attach scripts, but the game decides which script names exist.

This separation allows the engine to stay responsible for execution while the user defines the content and behavior of each scene.

## How to Build

You need a C++ compiler (GCC/Clang/MSVC) and **CMake** installed.
The Raylib dependency is automatically fetched and configured by CMake, you don't need to install it manually!

1. Open a terminal in the project folder.
2. Generate the build files:
   ```bash
   cmake -B build
   ```
   *(Note: On Windows with MinGW, you might need to use `cmake -G "MinGW Makefiles" -B build` instead)*
3. Build the project:
   ```bash
   cmake --build build
   ```
4. Run the executable:
   - On Windows: `.\build\NutEngine3D.exe` (or `.\build\Debug\NutEngine3D.exe`)
   - On Linux/Mac: `./build/NutEngine3D`

On Windows, you can also use `build.bat` and `run.bat` after the build folder has been generated.
The scene editor has its own helper scripts in `tools/scene_editor/build.bat` and `tools/scene_editor/run.bat`.

## Where to change settings

- **Screen Size:** In `src/main.cpp`, change the `screenWidth` and `screenHeight` variables.
- **Scene Content:** Edit `assets/scenes/demo.nutscene`.
- **Camera Position:** In `assets/scenes/demo.nutscene`, change `camera.position`.

## How the 3D Pipeline works

Every vertex of the 3D model goes through the following transformations:
1. **Local Space (Model):** Vertices relative to the center of the object (defined in `Mesh.h`).
2. **World Space (World Matrix):** The position/rotation of the object and its parent (`Transform::getWorldMatrix()`) are applied to the local vertices, moving them into the scene.
3. **Camera Space (View Matrix):** This matrix inverts the camera's position and rotation, "pulling" the entire world around the camera (which becomes the center of the universe at 0,0,0).
4. **Clip Space (Projection Matrix):** Applies perspective (distant objects appear smaller) and the screen's "Aspect Ratio", transforming everything into a normalized 3D cube (Normalized Device Coordinates).
5. **Screen Space (Viewport Transform):** Maps the normalized coordinates (from -1 to 1) to the actual pixels on the screen (width x height resolution).

## Future: Porting to ESP32 / TFT_eSPI

To run this engine on the ESP32, you simply need to:
1. Create a `TFTGraphics : public Graphics` class that implements the screen functions using the `TFT_eSPI` library.
2. Replace the instantiation of `RaylibGraphics` in the main file with `TFTGraphics`.
3. Compile using PlatformIO or Arduino IDE! The rest of the code (`math`, `core`, `WireframeRenderer`) will work exactly the same.
