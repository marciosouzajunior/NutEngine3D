#pragma once

#include "Scene.h"
#include <stdint.h>
#include <map>

namespace nut {

// SceneManager stores available scenes and controls which one is currently active.
// Game code decides when to change scenes; the manager performs the lifecycle calls.
class SceneManager {
private:
    std::map<uint16_t, Scene*> m_scenes;
    Scene* m_activeScene;
    Scene* m_pendingScene;

public:
    SceneManager();

    // Registers a scene using a compact numeric id.
    // The manager does not own the scene memory; whoever creates the scene must keep it alive.
    void registerScene(uint16_t id, Scene* scene);

    // Requests a scene change. If the id exists, the change happens at the end of the frame.
    void changeScene(uint16_t id);

    // Called by the engine once before the main loop starts.
    void start();

    // Called by the engine once per frame.
    void update(float deltaTime);

    // Called by the engine after the main loop ends.
    void shutdown();

    Scene* activeScene();
    const Scene* activeScene() const;

private:
    void applyPendingSceneChange();
};

} // namespace nut
