#pragma once

#include "../../src/core/CompiledScriptInstance.h"
#include <stdint.h>

namespace nut {
class GameObject;
struct InputState;

namespace game {

struct PlayerMoveScript {
    static constexpr uint16_t kScriptId = 3;
    static constexpr const char* kTypeName = "PlayerMoveScript";

    static void update(
        CompiledScriptInstance& instance,
        GameObject& object,
        const InputState& input,
        float deltaTime
    );
};

} // namespace game
} // namespace nut
