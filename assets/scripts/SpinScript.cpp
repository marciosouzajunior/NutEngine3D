#include "SpinScript.h"
#include "../../src/core/GameObject.h"

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
