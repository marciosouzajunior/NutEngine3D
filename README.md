# NutEngine3D V0

NutEngine3D is an educational 3D mini-engine focused on learning computer graphics and future portability to microcontrollers (ESP32/Arduino) with TFT displays.

## Current Features (V0)
- Full 3D Pipeline with 4x4 Matrices (Model, View, Projection).
- Basic Scene Graph (parent/child object hierarchy).
- Camera with perspective control and Aspect Ratio adaptation (ideal for non-square TFT screens).
- Wireframe rendering.
- Abstracted drawing backend (currently running Raylib on PC for easier testing, but ready for `TFT_eSPI`).

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

You can also use the included scripts (`setup.bat`, `build.bat`, and `run.bat`) on Windows for convenience.

## Where to change settings

- **Screen Size:** In `src/main.cpp`, change the `screenWidth` and `screenHeight` variables.
- **Camera Position:** In `src/main.cpp`, change `camera.transform.position`.

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
