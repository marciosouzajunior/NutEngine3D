#pragma once

#include "Scene.h"
#include "SceneManager.h"
#include "../render/Graphics.h"
#include "../render/WireframeRenderer.h"

namespace nut {

// Engine owns the main loop for the generic/native runtime.
//
// Pipeline role:
// 1. ask the platform backend whether the app should keep running
// 2. advance scene simulation for the current frame
// 3. tell the renderer to convert the active scene into 2D draw commands
// 4. ask the Graphics backend to present the finished frame
//
// On Nano the loop is handwritten in targets/nano/main.cpp because that target
// uses page-based presentation, but the high-level responsibilities are the same.
class Engine {
private:
    // Raw pointers are used here because the engine does not own these objects.
    // main.cpp creates them and keeps them alive while Engine::run is executing.
    Graphics* m_graphics;
    WireframeRenderer* m_renderer;

public:
    Engine(Graphics* graphics, WireframeRenderer* renderer)
        : m_graphics(graphics), m_renderer(renderer) {}

    void run(SceneManager& sceneManager) {
        // If the graphics backend or renderer is missing, there is nothing safe to run.
        if (!m_graphics || !m_renderer) {
            return;
        }

        sceneManager.start();

        // Main game loop:
        // 1. Ask the platform/window if it should keep running.
        // 2. Update the active scene.
        // 3. Draw the active scene.
        while (!m_graphics->shouldClose()) {
            const float deltaTime = m_graphics->deltaTime();

            // Scene-specific behavior happens here.
            // Later, this can update scripts/components attached to objects.
            Scene* activeScene = sceneManager.activeScene();
            if (activeScene) {
                activeScene->setInputState(m_graphics->sampleInput());
            }
            sceneManager.update(deltaTime);

            // Rendering is centralized in the engine, so scenes define content
            // and behavior without needing to know the frame drawing order.
            activeScene = sceneManager.activeScene();
            if (activeScene) {
                m_graphics->beginFrame();
                m_graphics->clear();
                m_renderer->render(*activeScene);
                m_graphics->present();
            }
        }

        sceneManager.shutdown();
    }
};

} // namespace nut
