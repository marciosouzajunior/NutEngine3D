#include "core/Engine.h"
#include "core/SceneManager.h"
#include "RaylibGraphics.h"
#include "render/WireframeRenderer.h"
#include "scene/RuntimeScene.h"
#include "scene/SceneBinaryLoader.h"
#include "../../build/assets/nano/demo_scene.h"
#include <iostream>

int main() {
    std::cout << "Starting NutEngine3D V0..." << std::endl;

    // 1. Initialize the Raylib graphics backend.
    // Render at the Nano OLED's native resolution and scale it for desktop viewing.
    const int screenWidth = 128;
    const int screenHeight = 64;
    const int displayScale = 3;
    nut::RaylibGraphics graphics(screenWidth, screenHeight, "NutEngine3D - Nano OLED Simulator (128x64)", displayScale);

    // 2. Create the renderer and engine.
    // The renderer knows how to draw a Scene. The engine knows when to update and draw.
    nut::WireframeRenderer renderer(&graphics);
    nut::Engine engine(&graphics, &renderer);

    // 3. Load the compiled demo scene blob.
    nut::RuntimeScene scene;
    if (!nut::SceneBinaryLoader::load(demo_scene_data, demo_scene_size, scene)) {
        std::cout << "Failed to load compiled demo scene blob" << std::endl;
        return 1;
    }

    // 4. Register scenes and choose which one starts first.
    nut::SceneManager sceneManager;
    sceneManager.registerScene("Demo", &scene);
    sceneManager.changeScene("Demo");

    // 5. Let the engine run the active scene.
    engine.run(sceneManager);

    std::cout << "Shutting down..." << std::endl;
    return 0;
}
