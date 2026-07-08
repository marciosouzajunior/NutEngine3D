#include "core/Engine.h"
#include "core/SceneManager.h"
#include "render/RaylibGraphics.h"
#include "render/WireframeRenderer.h"
#include "scene/LoadedScene.h"
#include "scene/SceneLoader.h"
#include "scene/ScriptRegistry.h"
#include "game_scripts/DummyScript.h"
#include "game_scripts/SpinScript.h"
#include <iostream>

int main() {
    std::cout << "Starting NutEngine3D V0..." << std::endl;

    // 1. Initialize the Raylib graphics backend.
    // This simulates a small 320x240 TFT screen on the desktop.
    const int screenWidth = 320;
    const int screenHeight = 240;
    nut::RaylibGraphics graphics(screenWidth, screenHeight, "NutEngine3D - TFT Simulator (320x240)");

    // 2. Create the renderer and engine.
    // The renderer knows how to draw a Scene. The engine knows when to update and draw.
    nut::WireframeRenderer renderer(&graphics);
    nut::Engine engine(&graphics, &renderer);

    // 3. Register scripts that scene JSON files are allowed to instantiate.
    nut::ScriptRegistry scripts;
    scripts.registerScript("DummyScript", DummyScript::createFromConfig);
    scripts.registerScript("SpinScript", SpinScript::createFromConfig);

    // 4. Load the demo scene from JSON.
    nut::LoadedScene scene;
    if (!nut::SceneLoader::load("assets/scenes/demo.nutscene", scene, scripts)) {
        std::cout << "Failed to load assets/scenes/demo.nutscene" << std::endl;
        return 1;
    }

    // 5. Register scenes and choose which one starts first.
    nut::SceneManager sceneManager;
    sceneManager.registerScene("Demo", &scene);
    sceneManager.changeScene("Demo");

    // 6. Let the engine run the active scene.
    engine.run(sceneManager);

    std::cout << "Shutting down..." << std::endl;
    return 0;
}
