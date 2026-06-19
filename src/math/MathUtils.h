#pragma once

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace nut {
namespace math {

    // Converts degrees to radians
    // Useful for trigonometric functions that expect radians
    inline float degToRad(float degrees) {
        return degrees * (float)(M_PI / 180.0);
    }

    // Converts radians to degrees
    inline float radToDeg(float radians) {
        return radians * (float)(180.0 / M_PI);
    }

} // namespace math
} // namespace nut
