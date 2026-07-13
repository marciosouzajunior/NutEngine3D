#include "PlayerMoveScript.h"
#include "../../src/core/GameObject.h"
#include "../../src/core/InputState.h"

namespace nut::game {

void PlayerMoveScript::update(
    CompiledScriptInstance& instance,
    GameObject& object,
    const InputState& input,
    float deltaTime
) {
    object.transform.position.x += input.moveX * instance.configFloat(0) * deltaTime;
    object.transform.position.y += input.moveY * instance.configFloat(1) * deltaTime;
    if (input.primaryPressed) {
        object.transform.position.z += instance.configFloat(2) * deltaTime;
    }
}

} // namespace nut::game
