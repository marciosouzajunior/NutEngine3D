#pragma once

#include "Camera.h"
#include "GameObject.h"
#include "InputState.h"
#include <stdint.h>
#ifndef ARDUINO
#include <string>
#endif

namespace nut {

class SceneManager;

// Scene is the user-authored game world or level.
// It owns the data being simulated and exposes lifecycle hooks to the engine.
class Scene {
protected:
    Camera m_camera;
    SceneManager* m_sceneManager;
    InputState m_inputState;

public:
    Scene();

    // A virtual destructor is important when deleting derived classes through a Scene pointer.
    virtual ~Scene() = default;

    // Called once before the main loop starts.
    // Use this to create objects, configure the camera, and prepare scene data.
    virtual void onEnter() {}

    // Called once per frame by the engine.
    // deltaTime is the elapsed time in seconds since the previous frame.
    virtual void onUpdate(float deltaTime) = 0;

    // Called once after the main loop ends.
    // Use this later for cleanup if a scene owns external resources.
    virtual void onExit() {}

    // Called by SceneManager when a scene is registered.
    void setSceneManager(SceneManager* sceneManager);

    // Scenes use this to ask the engine to switch to another registered scene.
    // The actual change happens after the current update finishes.
    void requestSceneChange(uint16_t sceneId);

    void setInputState(const InputState& inputState) {
        m_inputState = inputState;
    }

    const InputState& inputState() const {
        return m_inputState;
    }

#ifndef ARDUINO
    // Finds a GameObject by name anywhere in this scene.
    // The search starts at root objects and walks through children recursively.
    GameObject* findObject(const std::string& objectName);
    const GameObject* findObject(const std::string& objectName) const;
#endif

    // Adds a root object to the scene.
    // Children do not need to be added here because the renderer walks them recursively.
    virtual bool addObject(GameObject* obj) = 0;

    // Removes all root objects from the scene.
    // This does not delete objects; it only clears the list used by the renderer.
    virtual void clearObjects() = 0;

    // Access root objects through a small abstract API so each target/runtime
    // can choose its own storage strategy.
    virtual size_t rootObjectCount() const = 0;
    virtual GameObject* rootObjectAt(size_t index) = 0;
    virtual const GameObject* rootObjectAt(size_t index) const = 0;

    // Starts scripts attached to all root objects and their children.
    void startObjects() {
        for (size_t i = 0; i < rootObjectCount(); ++i) {
            GameObject* obj = rootObjectAt(i);
            if (obj) {
                obj->startScripts();
            }
        }
    }

    // Updates scripts attached to all root objects and their children.
    void updateObjects(float deltaTime) {
        for (size_t i = 0; i < rootObjectCount(); ++i) {
            GameObject* obj = rootObjectAt(i);
            if (obj) {
                obj->updateScripts(deltaTime);
            }
        }
    }

    Camera& camera() {
        return m_camera;
    }

    const Camera& camera() const {
        return m_camera;
    }

private:
#ifndef ARDUINO
    GameObject* findObjectInTree(GameObject* obj, const std::string& objectName);
    const GameObject* findObjectInTree(const GameObject* obj, const std::string& objectName) const;
#endif
};

} // namespace nut
