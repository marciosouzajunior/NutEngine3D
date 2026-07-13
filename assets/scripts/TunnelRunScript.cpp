#include "TunnelRunScript.h"
#include "../../src/core/GameObject.h"
#include "../../src/core/InputState.h"

namespace nut::game {
namespace {

constexpr uint8_t kGameOver = 0x01;
constexpr uint8_t kButtonWasPressed = 0x02;
constexpr float kLaneOffset = 4.0f;
constexpr float kPlayerHorizontalLimit = 5.0f;

float clampValue(float value, float minimum, float maximum) {
    if (value < minimum) return minimum;
    if (value > maximum) return maximum;
    return value;
}

void placeObstacle(GameObject& obstacle, uint16_t laneIndex, float z) {
    static const int8_t laneDirection[] = {-1, 0, 1, 1, 0, -1};
    const uint8_t lane = static_cast<uint8_t>(laneIndex % 6);
    obstacle.transform.position.x = static_cast<float>(laneDirection[lane]) * kLaneOffset;
    obstacle.transform.position.y = 0.0f;
    obstacle.transform.position.z = z;
}

void resetGame(GameObject& root, CompiledScriptInstance& instance) {
    GameObject* player = root.childObjectAt(0);
    if (player) {
        player->transform.position = math::Vec3(0.0f, 0.0f, 0.0f);
    }

    for (size_t i = 1; i < 4; ++i) {
        GameObject* obstacle = root.childObjectAt(i);
        if (obstacle) {
            placeObstacle(*obstacle, static_cast<uint16_t>(i - 1), static_cast<float>(i) * 8.0f);
        }
    }

    instance.stateWord = 0;
    instance.stateFlags &= static_cast<uint8_t>(~kGameOver);
}

} // namespace

void TunnelRunScript::update(
    CompiledScriptInstance& instance,
    GameObject& root,
    const InputState& input,
    float deltaTime
) {
    if (root.childObjectCount() < 4) return;

    const bool buttonPressed = input.primaryPressed;
    const bool buttonWasPressed = (instance.stateFlags & kButtonWasPressed) != 0;
    if (buttonPressed) {
        instance.stateFlags |= kButtonWasPressed;
    } else {
        instance.stateFlags &= static_cast<uint8_t>(~kButtonWasPressed);
    }

    if ((instance.stateFlags & kGameOver) != 0) {
        if (buttonPressed && !buttonWasPressed) resetGame(root, instance);
        return;
    }

    GameObject* player = root.childObjectAt(0);
    if (!player) return;

    player->transform.position.x = clampValue(
        player->transform.position.x,
        -kPlayerHorizontalLimit,
        kPlayerHorizontalLimit
    );
    player->transform.position.y = clampValue(player->transform.position.y, -1.4f, 1.4f);

    const uint16_t difficultySteps = instance.stateWord < 30 ? instance.stateWord : 30;
    const float speed = instance.configFloat(0)
        + static_cast<float>(difficultySteps) * instance.configFloat(1);
    float farthestZ = root.childObjectAt(1)->transform.position.z;
    for (size_t i = 2; i < 4; ++i) {
        const GameObject* obstacle = root.childObjectAt(i);
        if (obstacle && obstacle->transform.position.z > farthestZ) {
            farthestZ = obstacle->transform.position.z;
        }
    }

    for (size_t i = 1; i < 4; ++i) {
        GameObject* obstacle = root.childObjectAt(i);
        if (!obstacle) continue;

        obstacle->transform.position.z -= speed * deltaTime;
        const math::Vec3 delta = obstacle->transform.position - player->transform.position;
        const float radius = instance.configFloat(2);
        if (delta.x > -radius && delta.x < radius
            && delta.y > -radius && delta.y < radius
            && delta.z > -radius && delta.z < radius) {
            instance.stateFlags |= kGameOver;
            player->transform.position.z = -20.0f;
            return;
        }

        if (obstacle->transform.position.z < -1.5f) {
            farthestZ += 8.0f;
            ++instance.stateWord;
            placeObstacle(*obstacle, instance.stateWord, farthestZ);
        }
    }
}

} // namespace nut::game
