#pragma once

#include "../../src/core/CompiledScriptInstance.h"
#include <stdint.h>

namespace nut {
class GameObject;
struct InputState;

namespace game {

// Wraps one authored axis back into a configured interval when the coordinate
// leaves that range.
//
// Binary config layout:
// - float axis index (0=x, 1=y, 2=z)
// - float minValue
// - float maxValue
//
// This remains useful as a standalone utility, but Nano scenes that always
// translate and wrap together can use AutoTranslateWrapScript to halve the
// number of script instances.
struct WrapPositionScript {
    static constexpr uint16_t kScriptId = 5;
    static constexpr const char* kTypeName = "WrapPositionScript";

    static void update(
        CompiledScriptInstance& instance,
        GameObject& object,
        const InputState& input,
        float deltaTime
    );
};

} // namespace game
} // namespace nut
