#pragma once

#include "../../src/core/CompiledScriptInstance.h"
#include <stdint.h>

namespace nut {
class GameObject;
struct InputState;

namespace game {

// Complete game-side Tunnel Run script module. The engine sees only its
// numeric id and opaque bytes; this module owns their gameplay meaning.
struct TunnelRunScript {
    static constexpr uint16_t kScriptId = 4;
    static constexpr const char* kTypeName = "TunnelRunScript";

    static void update(
        CompiledScriptInstance& instance,
        GameObject& object,
        const InputState& input,
        float deltaTime
    );
};

} // namespace game
} // namespace nut
