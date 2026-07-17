#pragma once

#include "../../src/core/CompiledScriptInstance.h"
#include <stdint.h>

namespace nut {
class GameObject;
struct InputState;

namespace game {

// Moves an object continuously by an authored velocity vector.
//
// Binary config layout:
// - float x speed
// - float y speed
// - float z speed
//
// This is the generic reusable version. Nano scenes that also need wrapping
// can use AutoTranslateWrapScript to avoid spending a second script instance.
struct AutoTranslateScript {
    static constexpr uint16_t kScriptId = 2;
    static constexpr const char* kTypeName = "AutoTranslateScript";

    static void update(
        CompiledScriptInstance& instance,
        GameObject& object,
        const InputState& input,
        float deltaTime
    );
};

} // namespace game
} // namespace nut
