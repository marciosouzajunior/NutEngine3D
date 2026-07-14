#include "AutoTranslateScript.h"

#include "../../src/core/GameObject.h"

namespace nut::game {

void AutoTranslateScript::update(
    CompiledScriptInstance& instance,
    GameObject& object,
    const InputState& input,
    float deltaTime
) {
    (void)input;

    object.transform.position.x += instance.configFloat(0) * deltaTime;
    object.transform.position.y += instance.configFloat(1) * deltaTime;
    object.transform.position.z += instance.configFloat(2) * deltaTime;
}

} // namespace nut::game
