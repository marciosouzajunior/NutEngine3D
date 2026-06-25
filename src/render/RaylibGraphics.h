#pragma once

#include "Graphics.h"
#include <raylib.h>

namespace nut {

// Concrete implementation of Graphics using Raylib.
class RaylibGraphics : public Graphics {
private:
    int m_width;
    int m_height;

public:
    RaylibGraphics(int width, int height, const char* title) 
        : m_width(width), m_height(height) 
    {
        // Raylib owns the OS window after InitWindow is called.
        InitWindow(width, height, title);

        // Request 60 frames per second. Raylib will try to keep this pace.
        SetTargetFPS(60);
    }

    ~RaylibGraphics() override {
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
        // Raylib requires BeginDrawing before any draw calls for the frame.
        BeginDrawing();
    }

    void drawLine(int x1, int y1, int x2, int y2) override {
        DrawLine(x1, y1, x2, y2, WHITE);
    }

    void present() override {
        // Raylib displays the frame when EndDrawing is called.
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
