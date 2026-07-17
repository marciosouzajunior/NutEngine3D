#pragma once

#include "../../src/core/CompiledScriptInstance.h"
#include <stdint.h>

namespace nut {
class GameObject;
struct InputState;

namespace game {

// Moves an object along one authored axis and wraps it back into a configured
// range when it leaves that interval.
//
// Binary config layout:
// - byte 0: axis index (0=x, 1=y, 2=z)
// - bytes 1..4: float speed along that axis
// - bytes 5..6: int16 minValue packed as value * 100
// - bytes 7..8: int16 maxValue packed as value * 100
//
// This keeps the scene-facing behavior of AutoTranslateScript +
// WrapPositionScript while fitting within one 12-byte script payload.
struct AutoTranslateWrapScript {
    static constexpr uint16_t kScriptId = 10;
    static constexpr const char* kTypeName = "AutoTranslateWrapScript";

    static void update(
        CompiledScriptInstance& instance,
        GameObject& object,
        const InputState& input,
        float deltaTime
    );
};

} // namespace game
} // namespace nut
