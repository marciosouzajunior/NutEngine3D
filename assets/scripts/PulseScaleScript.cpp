#include "PulseScaleScript.h"

#include "../../src/core/GameObject.h"

#include <math.h>

namespace nut::game {

void PulseScaleScript::update(
    CompiledScriptInstance& instance,
    GameObject& object,
    const InputState& input,
    float deltaTime
) {
    (void)input;

    const float speed = instance.configFloat(0);
    const float minScale = instance.configFloat(1);
    const float maxScale = instance.configFloat(2);
    if (maxScale < minScale) {
        return;
    }

    const uint16_t frameMs = static_cast<uint16_t>(deltaTime * 1000.0f + 0.5f);
    instance.stateWord = static_cast<uint16_t>(instance.stateWord + frameMs);

    const float phase = static_cast<float>(instance.stateWord) * 0.001f * speed;
    const float wave = 0.5f + 0.5f * sinf(phase);
    const float scale = minScale + (maxScale - minScale) * wave;

    object.transform.scale.x = scale;
    object.transform.scale.y = scale;
    object.transform.scale.z = scale;
}

} // namespace nut::game
