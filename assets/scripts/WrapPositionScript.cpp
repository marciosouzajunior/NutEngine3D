#include "WrapPositionScript.h"

#include "../../src/core/GameObject.h"

#include <math.h>

namespace nut::game {

void WrapPositionScript::update(
    CompiledScriptInstance& instance,
    GameObject& object,
    const InputState& input,
    float deltaTime
) {
    (void)input;
    (void)deltaTime;

    const int axis = static_cast<int>(roundf(instance.configFloat(0)));
    const float minValue = instance.configFloat(1);
    const float maxValue = instance.configFloat(2);
    if (axis < 0 || axis > 2 || maxValue <= minValue) {
        return;
    }

    float* coordinate = nullptr;
    if (axis == 0) {
        coordinate = &object.transform.position.x;
    } else if (axis == 1) {
        coordinate = &object.transform.position.y;
    } else {
        coordinate = &object.transform.position.z;
    }

    if (*coordinate > maxValue) {
        *coordinate = minValue;
    } else if (*coordinate < minValue) {
        *coordinate = maxValue;
    }
}

} // namespace nut::game
