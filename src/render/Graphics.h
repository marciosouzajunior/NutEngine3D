#pragma once

#include "../core/InputState.h"

namespace nut {

// Graphics is the narrow "draw lines on a 2D surface" contract used by the engine.
//
// Pipeline role:
// - Renderers finish their 3D math and reduce everything to 2D line segments.
// - They call this interface to clear a frame, draw those lines, and present it.
// - Concrete backends decide what "present" means:
//   - desktop: show the rendered image in a window
//   - Nano: flush the current OLED page over I2C
//
// In other words, this layer is already after projection. It does not know about
// meshes, cameras, or scenes; it only knows about 2D drawing commands.
class Graphics {
public:
    // Because this class has virtual methods, the destructor should also be virtual.
    // This keeps cleanup correct when using Graphics* to point to a derived backend.
    virtual ~Graphics() = default;

    // Start the rendering of a new frame.
    virtual void beginFrame() = 0;

    // Get the width of the drawing area
    virtual int width() const = 0;

    // Get the height of the drawing area
    virtual int height() const = 0;

    // Clear the screen before drawing the next frame.
    virtual void clear() = 0;

    // Draw a single line between two 2D points. 
    // Coordinates are in screen space (0 to width, 0 to height).
    virtual void drawLine(int x1, int y1, int x2, int y2) = 0;

    // Finish the frame and present the drawing to the screen.
    virtual void present() = 0;

    // Returns true when the application should stop running
    // for example, when the user closes the window.
    virtual bool shouldClose() const = 0;

    // Elapsed time in seconds since the previous frame.
    virtual float deltaTime() const = 0;

    // Sample the current user input state for this frame.
    virtual InputState sampleInput() const = 0;
};

} // namespace nut
