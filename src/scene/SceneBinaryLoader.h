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
    struct MeshRecord {
        uint16_t vertexCount;
        uint16_t edgeCount;
    };

    struct ObjectRecord {
        uint16_t nameOffset;
        int16_t parentIndex;
        int16_t meshIndex;
        uint16_t firstScriptIndex;
        uint16_t scriptCount;
        float position[3];
        float rotation[3];
        float scale[3];
    };

    struct ScriptRecord {
        uint16_t scriptId;
        uint16_t configOffset;
        uint16_t configSize;
    };

#ifdef ARDUINO
    struct LoadWorkspace {
        // Temporary decode-time storage only.
        // These arrays scale with the same NUT_MAX_* capacities exposed by
        // RuntimeLimits.h, so the Nano scratch requirement stays coupled to the
        // real runtime limits instead of to a manually maintained byte guess.
        uint8_t* stringTable;
        uint16_t stringTableSize;
        MeshRecord meshRecords[NUT_MAX_MESHES];
        uint16_t meshRecordCount;
        ObjectRecord objectRecords[NUT_MAX_OBJECTS];
        uint16_t objectRecordCount;
        ScriptRecord scriptRecords[NUT_MAX_SCRIPT_RECORDS];
        uint16_t scriptRecordCount;
        GameObject* objects[NUT_MAX_OBJECTS];
        uint16_t objectCount;
    };

    // Temporary Nano storage is derived directly from the configured runtime
    // limits. The scene-specific generated config chooses capacities; the core
    // then derives the exact workspace size from those capacities.
    static constexpr size_t NANO_SCRATCH_BYTES = sizeof(LoadWorkspace);

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
