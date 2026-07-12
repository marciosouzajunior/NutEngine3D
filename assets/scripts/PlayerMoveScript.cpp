#include "PlayerMoveScript.h"
#include "../../src/core/InputState.h"

PlayerMoveScript::PlayerMoveScript(const nut::math::Vec3& unitsPerSecond)
    : m_unitsPerSecond(unitsPerSecond) {}

void PlayerMoveScript::onUpdate(float deltaTime) {
    if (!gameObject()) {
        return;
    }

    const nut::InputState* input = inputState();
    if (!input) {
        return;
    }

    gameObject()->transform.position.x += input->moveX * m_unitsPerSecond.x * deltaTime;
    gameObject()->transform.position.y += input->moveY * m_unitsPerSecond.y * deltaTime;
    if (input->primaryPressed) {
        gameObject()->transform.position.z += m_unitsPerSecond.z * deltaTime;
    }
}
