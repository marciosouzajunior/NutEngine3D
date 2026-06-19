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
        InitWindow(width, height, title);
        SetTargetFPS(60);
    }

    ~RaylibGraphics() override {
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

    void drawLine(int x1, int y1, int x2, int y2) override {
        DrawLine(x1, y1, x2, y2, WHITE);
    }

    void present() override {
        // With Raylib, present is basically just ending the drawing process
        // Note: BeginDrawing must be called externally or here before clearing.
        // We will adapt our loop: BeginDrawing() -> clear() -> draw lines -> present()
        EndDrawing();
    }

    // Helper to call BeginDrawing
    void beginFrame() {
        BeginDrawing();
    }

    bool shouldClose() const {
        return WindowShouldClose();
    }
};

} // namespace nut
