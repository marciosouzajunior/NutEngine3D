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
#ifdef ARDUINO
    // Temporary storage supplied by the Nano target. It can be reused after
    // load() returns because only the decoded string table remains persistent.
    // Fits the largest currently configured Nano environment (stress4). If a
    // future NUT_CFG_* limit needs more, SceneBinaryLoader.cpp fails at compile
    // time instead of allowing a runtime overflow.
#ifndef NUT_CFG_SCENE_LOADER_SCRATCH_BYTES
#define NUT_CFG_SCENE_LOADER_SCRATCH_BYTES 264
#endif
    static constexpr size_t NANO_SCRATCH_BYTES = NUT_CFG_SCENE_LOADER_SCRATCH_BYTES;

    static bool load(
        const uint8_t* data,
        size_t size,
        RuntimeScene& outScene,
        void* scratch,
        size_t scratchSize
    );
#else
    static bool load(const uint8_t* data, size_t size, RuntimeScene& outScene);
#endif
};

} // namespace nut
