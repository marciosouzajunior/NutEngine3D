#include "SpinScript.h"
#include "../../src/core/GameObject.h"

namespace nut::game {

void SpinScript::update(
    CompiledScriptInstance& instance,
    GameObject& object,
    const InputState& input,
    float deltaTime
) {
    (void)input;

    object.transform.rotation.x += instance.configFloat(0) * deltaTime;
    object.transform.rotation.y += instance.configFloat(1) * deltaTime;
    object.transform.rotation.z += instance.configFloat(2) * deltaTime;
}

} // namespace nut::game
