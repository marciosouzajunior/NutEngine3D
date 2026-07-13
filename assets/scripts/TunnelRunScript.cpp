#include "TunnelRunScript.h"
#include "../../src/core/GameObject.h"
#include "../../src/core/InputState.h"
#include "../../src/scene/RuntimeScene.h"

namespace nut::game {
namespace {

constexpr uint8_t kGameOver = 0x01;
constexpr uint8_t kButtonWasPressed = 0x02;
constexpr uint16_t kAutoRestartDelayMs = 2500;
constexpr float kLaneOffset = 4.0f;
constexpr float kPlayerHorizontalLimit = 5.0f;
constexpr size_t kPlayerObjectIndex = 1;
constexpr size_t kFirstObstacleObjectIndex = 2;
constexpr size_t kObstacleCount = 3;

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

void resetGame(RuntimeScene& scene, CompiledScriptInstance& instance) {
    GameObject* player = scene.loadedObjectAt(kPlayerObjectIndex);
    if (player) {
        player->transform.position = math::Vec3(0.0f, 0.0f, 0.0f);
    }

    for (size_t i = 0; i < kObstacleCount; ++i) {
        GameObject* obstacle = scene.loadedObjectAt(kFirstObstacleObjectIndex + i);
        if (obstacle) {
            placeObstacle(*obstacle, static_cast<uint16_t>(i), static_cast<float>(i + 1) * 8.0f);
        }
    }

    instance.stateWord = 0;
    instance.stateFlags &= static_cast<uint8_t>(~kGameOver);
}

} // namespace

void TunnelRunScript::update(
    CompiledScriptInstance& instance,
    RuntimeScene& scene,
    const InputState& input,
    float deltaTime
) {
    // Tunnel Run intentionally uses the authored flat object order:
    // Game=0, Player=1, and obstacles A/B/C=2..4. This avoids names, registries,
    // and per-object child capacity on the Nano runtime.
    if (scene.loadedObjectCount() < kFirstObstacleObjectIndex + kObstacleCount) return;

    const bool buttonPressed = input.primaryPressed;
    const bool buttonWasPressed = (instance.stateFlags & kButtonWasPressed) != 0;
    if (buttonPressed) {
        instance.stateFlags |= kButtonWasPressed;
    } else {
        instance.stateFlags &= static_cast<uint8_t>(~kButtonWasPressed);
    }

    if ((instance.stateFlags & kGameOver) != 0) {
        // Difficulty is no longer needed after a collision, so stateWord is
        // reused as a millisecond timer without increasing per-script SRAM.
        const uint16_t frameMs = static_cast<uint16_t>(deltaTime * 1000.0f);
        const bool restartPressed = buttonPressed && !buttonWasPressed;
        const bool restartElapsed = instance.stateWord >= kAutoRestartDelayMs - frameMs;
        if (restartPressed || restartElapsed) {
            resetGame(scene, instance);
        } else {
            instance.stateWord = static_cast<uint16_t>(instance.stateWord + frameMs);
        }
        return;
    }

    GameObject* player = scene.loadedObjectAt(kPlayerObjectIndex);
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
    GameObject* firstObstacle = scene.loadedObjectAt(kFirstObstacleObjectIndex);
    if (!firstObstacle) return;

    float farthestZ = firstObstacle->transform.position.z;
    for (size_t i = 1; i < kObstacleCount; ++i) {
        const GameObject* obstacle = scene.loadedObjectAt(kFirstObstacleObjectIndex + i);
        if (obstacle && obstacle->transform.position.z > farthestZ) {
            farthestZ = obstacle->transform.position.z;
        }
    }

    for (size_t i = 0; i < kObstacleCount; ++i) {
        GameObject* obstacle = scene.loadedObjectAt(kFirstObstacleObjectIndex + i);
        if (!obstacle) continue;

        obstacle->transform.position.z -= speed * deltaTime;
        const math::Vec3 delta = obstacle->transform.position - player->transform.position;
        const float radius = instance.configFloat(2);
        if (delta.x > -radius && delta.x < radius
            && delta.y > -radius && delta.y < radius
            && delta.z > -radius && delta.z < radius) {
            instance.stateFlags |= kGameOver;
            instance.stateWord = 0;
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
