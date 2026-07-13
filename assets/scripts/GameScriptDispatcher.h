#pragma once

namespace nut {
class RuntimeScene;

namespace game {

// Game composition point: iterate compiled instances and route each numeric id
// to the script module linked into this firmware. No registry or heap is used.
void updateScripts(RuntimeScene& scene, float deltaTime);

} // namespace game
} // namespace nut
