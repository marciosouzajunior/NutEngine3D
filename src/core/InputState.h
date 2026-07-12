#pragma once

#include <stdint.h>

namespace nut {

// Compact per-frame input snapshot shared by desktop and Nano runtimes.
//
// Axis values are normalized to roughly [-1, 1].
// A value near 0 means "centered" or "not pressed".
struct InputState {
    float moveX = 0.0f;
    float moveY = 0.0f;
    bool primaryPressed = false;
};

} // namespace nut
