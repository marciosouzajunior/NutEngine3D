#pragma once

namespace nut {

// Abstract interface for 2D drawing.
// This isolates the 3D engine from the actual rendering library (Raylib, TFT_eSPI, etc).
class Graphics {
public:
    virtual ~Graphics() = default;

    // Get the width of the drawing area
    virtual int width() const = 0;

    // Get the height of the drawing area
    virtual int height() const = 0;

    // Clear the screen with a specific background color
    virtual void clear() = 0;

    // Draw a single line between two 2D points. 
    // Coordinates are in screen space (0 to width, 0 to height).
    virtual void drawLine(int x1, int y1, int x2, int y2) = 0;

    // Present the drawing to the screen (swap buffers if necessary)
    virtual void present() = 0;
};

} // namespace nut
