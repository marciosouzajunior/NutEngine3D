#pragma once

#include "../../src/render/Graphics.h"
#include <raylib.h>

namespace nut {

// RaylibGraphics is the desktop Graphics backend.
//
// Pipeline role:
// - receive already-projected 2D lines from WireframeRenderer
// - draw them into a small logical framebuffer
// - upscale that framebuffer to a desktop window in present()
//
// It does not perform 3D math. Its job starts after the renderer has already
// decided which 2D lines should appear on the frame.
class RaylibGraphics : public Graphics {
private:
    int m_width;
    int m_height;
    int m_displayScale;
    RenderTexture2D m_renderTarget;

public:
    RaylibGraphics(int width, int height, const char* title, int displayScale = 1)
        : m_width(width), m_height(height), m_displayScale(displayScale < 1 ? 1 : displayScale)
    {
        // The desktop simulator renders at the target's native resolution
        // and scales the final image so framing matches the hardware.
        InitWindow(width * m_displayScale, height * m_displayScale, title);
        m_renderTarget = LoadRenderTexture(width, height);

        // Request 60 frames per second. Raylib will try to keep this pace.
        SetTargetFPS(60);
    }

    ~RaylibGraphics() override {
        UnloadRenderTexture(m_renderTarget);
        // CloseWindow releases the native window and Raylib resources.
        CloseWindow();
    }

    int width() const override {
        return m_width;
    }

    int height() const override {
        return m_height;
    }

    void clear() override {
        ClearBackground(BLACK);
    }

    void beginFrame() override {
        // Start drawing into the logical off-screen framebuffer.
        // The final upscale to the visible window happens in present().
        BeginTextureMode(m_renderTarget);
    }

    void drawLine(int x1, int y1, int x2, int y2) override {
        DrawLine(x1, y1, x2, y2, WHITE);
    }

    void present() override {
        // Finish the logical framebuffer and copy it to the desktop window.
        // This keeps the desktop framing close to the 128x64 Nano display.
        EndTextureMode();
        BeginDrawing();
        ClearBackground(BLACK);
        Rectangle source = {
            0.0f,
            0.0f,
            static_cast<float>(m_width),
            -static_cast<float>(m_height)
        };
        Rectangle dest = {
            0.0f,
            0.0f,
            static_cast<float>(m_width * m_displayScale),
            static_cast<float>(m_height * m_displayScale)
        };
        DrawTexturePro(m_renderTarget.texture, source, dest, Vector2{0.0f, 0.0f}, 0.0f, WHITE);
        EndDrawing();
    }

    bool shouldClose() const override {
        // WindowShouldClose becomes true when the user closes the window.
        return WindowShouldClose();
    }

    float deltaTime() const override {
        // GetFrameTime returns seconds since the previous rendered frame.
        return GetFrameTime();
    }
};

} // namespace nut
