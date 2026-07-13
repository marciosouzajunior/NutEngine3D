#include "DummyScript.h"

namespace nut::game {

void DummyScript::update(
    CompiledScriptInstance& instance,
    GameObject& object,
    const InputState& input,
    float deltaTime
) {
    (void)object;
    (void)input;
    (void)deltaTime;
    (void)instance.configByte(0);
}

} // namespace nut::game
