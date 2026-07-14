#pragma once

#include "../../src/core/CompiledScriptInstance.h"
#include <stdint.h>

namespace nut {
class GameObject;
struct InputState;
namespace scene {
class RuntimeScene;
}

namespace game {

// Placeholder script for new scenes.
//
// Attach this to the root "Game" object and replace its no-op update with the
// first gameplay loop of a new project. Keeping this as a normal native script
// makes the complete flow visible from day one: scene -> script ID -> runtime.
struct GameControllerScript {
    static constexpr uint16_t kScriptId = 6;
    static constexpr const char* kTypeName = "GameControllerScript";

    static void update(
        CompiledScriptInstance& instance,
        GameObject& object,
        const InputState& input,
        float deltaTime
    );
};

} // namespace game
} // namespace nut
