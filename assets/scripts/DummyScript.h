#pragma once

#include "../../src/core/CompiledScriptInstance.h"
#include <stdint.h>

namespace nut {
class GameObject;
struct InputState;

namespace game {

struct DummyScript {
    static constexpr uint16_t kScriptId = 2;
    static constexpr const char* kTypeName = "DummyScript";

    static void update(
        CompiledScriptInstance& instance,
        GameObject& object,
        const InputState& input,
        float deltaTime
    );
};

} // namespace game
} // namespace nut
