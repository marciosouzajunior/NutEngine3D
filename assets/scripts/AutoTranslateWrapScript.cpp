#include "AutoTranslateWrapScript.h"

#include "../../src/core/GameObject.h"

namespace nut::game {
namespace {

int16_t readPackedI16(const CompiledScriptInstance& instance, size_t offset) {
    const uint16_t low = instance.configByte(offset);
    const uint16_t high = instance.configByte(offset + 1);
    return static_cast<int16_t>(low | (high << 8));
}

float readPackedF32(const CompiledScriptInstance& instance, size_t offset) {
    union FloatBits {
        uint32_t bits;
        float value;
    } decoded {};

    decoded.bits = static_cast<uint32_t>(instance.configByte(offset))
        | (static_cast<uint32_t>(instance.configByte(offset + 1)) << 8)
        | (static_cast<uint32_t>(instance.configByte(offset + 2)) << 16)
        | (static_cast<uint32_t>(instance.configByte(offset + 3)) << 24);
    return decoded.value;
}

float unpackFixed100(int16_t value) {
    return static_cast<float>(value) / 100.0f;
}

} // namespace

void AutoTranslateWrapScript::update(
    CompiledScriptInstance& instance,
    GameObject& object,
    const InputState& input,
    float deltaTime
) {
    (void)input;

    const uint8_t axis = instance.configByte(0);
    const float speed = readPackedF32(instance, 1);
    const float minValue = unpackFixed100(readPackedI16(instance, 5));
    const float maxValue = unpackFixed100(readPackedI16(instance, 7));
    if (axis > 2 || maxValue <= minValue) {
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

    *coordinate += speed * deltaTime;
    if (*coordinate > maxValue) {
        *coordinate = minValue;
    } else if (*coordinate < minValue) {
        *coordinate = maxValue;
    }
}

} // namespace nut::game
