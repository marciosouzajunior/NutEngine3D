#include "SpinScript.h"
#include "../core/GameObject.h"

namespace {

nut::math::Vec3 readVec3(const nut::Json& value, const nut::math::Vec3& defaultValue) {
    if (!value.isArray() || value.size() < 3) {
        return defaultValue;
    }

    return nut::math::Vec3(
        static_cast<float>(value.at(0).asNumber(defaultValue.x)),
        static_cast<float>(value.at(1).asNumber(defaultValue.y)),
        static_cast<float>(value.at(2).asNumber(defaultValue.z))
    );
}

} // namespace

SpinScript::SpinScript(const nut::math::Vec3& rotationSpeed)
    : m_rotationSpeed(rotationSpeed) {}

void SpinScript::onUpdate(float deltaTime) {
    if (!gameObject()) {
        return;
    }

    // Scripts modify the GameObject they are attached to.
    // Here we rotate only the Transform; the Mesh shape stays unchanged.
    gameObject()->transform.rotation.x += m_rotationSpeed.x * deltaTime;
    gameObject()->transform.rotation.y += m_rotationSpeed.y * deltaTime;
    gameObject()->transform.rotation.z += m_rotationSpeed.z * deltaTime;
}

std::unique_ptr<nut::Script> SpinScript::createFromConfig(const nut::Json& config) {
    nut::math::Vec3 rotationSpeed = readVec3(config.get("rotationSpeed"), nut::math::Vec3(0, 0, 0));
    return std::make_unique<SpinScript>(rotationSpeed);
}
