#pragma once

#include "RuntimeScene.h"

#include <stddef.h>
#include <stdint.h>

namespace nut {

// SceneBinaryLoader turns the compiled scene blob into a runtime RuntimeScene.
//
// Pipeline role:
// 1. validate and read the binary scene header
// 2. decode tables for strings, meshes, objects, and scripts
// 3. build the runtime scene graph
// 4. attach compact compiled script instances
// 5. preserve mesh access in the form each target needs
//
// It is the bridge between offline scene compilation and runtime simulation/rendering.
class SceneBinaryLoader {
public:
    static bool load(const uint8_t* data, size_t size, RuntimeScene& outScene);
};

} // namespace nut
