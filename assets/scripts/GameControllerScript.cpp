#include "GameControllerScript.h"

#include "../../src/core/GameObject.h"

namespace nut::game {

void GameControllerScript::update(
    CompiledScriptInstance& instance,
    GameObject& object,
    const InputState& input,
    float deltaTime
) {
    // This script is intentionally a no-op starter.
    //
    // Why it exists:
    // - new scenes should already show the full engine flow working
    // - the root "Game" object should have one obvious place to begin gameplay
    // - a first project can start here without learning Tunnel Run internals
    //
    // Mental model:
    // - `object` is usually the root "Game" object of the scene
    // - `input` is the per-frame snapshot already collected by the engine
    // - `deltaTime` is the frame step in seconds
    // - `instance` gives you 4 bytes of mutable state:
    //   - `stateWord`  : general 16-bit counter/timer/state
    //   - `stateFlags` : packed booleans in bits
    //   - `stateByte`  : one extra small value
    //
    // Suggested first steps for a new game:
    // 1. Read input and update one object or one game state variable.
    // 2. Use `stateFlags` or `stateWord` for simple mode switches and timers.
    // 3. Keep the logic deterministic and allocation-free.
    // 4. Move repeated or reusable behavior into separate utility scripts.
    //
    // Example 1: toggle a "started" flag the first time the player presses the
    // primary button.
    //
    //   constexpr uint8_t kStarted = 0x01;
    //   if (input.primaryPressed) {
    //       instance.stateFlags |= kStarted;
    //   }
    //
    // Example 2: count elapsed milliseconds in `stateWord` for a simple intro
    // timer or cooldown.
    //
    //   const uint16_t frameMs = static_cast<uint16_t>(deltaTime * 1000.0f);
    //   instance.stateWord = static_cast<uint16_t>(instance.stateWord + frameMs);
    //   if (instance.stateWord > 1500) {
    //       // Intro finished, spawn enabled, etc.
    //   }
    //
    // Example 3: move the root object itself from input.
    //
    //   object.transform.position.x += input.moveX * 2.0f * deltaTime;
    //   object.transform.position.y += input.moveY * 2.0f * deltaTime;
    //
    // Example 4: drive a known child object directly.
    // This is a simple pattern when the scene hierarchy is small and authored
    // intentionally for Nano.
    //
    //   GameObject* player = object.childObjectAt(0);
    //   if (player) {
    //       player->transform.rotation.y += 1.5f * deltaTime;
    //   }
    //
    // Example 5: keep one tiny state machine in `stateByte`.
    //
    //   enum Phase : uint8_t { Intro = 0, Playing = 1, GameOver = 2 };
    //   if (instance.stateByte == Intro && input.primaryPressed) {
    //       instance.stateByte = Playing;
    //   }
    //
    // Keep this file as the project-specific "where gameplay begins" script.
    // If the logic grows large, split pieces into their own scripts instead of
    // turning one update() into a monolith.
    (void)instance;
    (void)object;
    (void)input;
    (void)deltaTime;
}

} // namespace nut::game
