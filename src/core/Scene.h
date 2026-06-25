#pragma once

#include "Camera.h"
#include "GameObject.h"
#include <string>
#include <vector>

namespace nut {

class SceneManager;

// Scene is the user-authored game world or level.
// It owns the data being simulated and exposes lifecycle hooks to the engine.
class Scene {
protected:
    // Protected means child classes can use these members, but outside code cannot.
    // A custom scene should add root objects here and position this camera.
    std::vector<GameObject*> m_rootObjects;
    Camera m_camera;
    SceneManager* m_sceneManager;

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
    void requestSceneChange(const std::string& sceneName);

    // Finds a GameObject by name anywhere in this scene.
    // The search starts at root objects and walks through children recursively.
    GameObject* findObject(const std::string& objectName);
    const GameObject* findObject(const std::string& objectName) const;

    // Adds a root object to the scene.
    // Children do not need to be added here because the renderer walks them recursively.
    void addObject(GameObject* obj) {
        if (obj) {
            obj->setScene(this);
            m_rootObjects.push_back(obj);
        }
    }

    // Removes all root objects from the scene.
    // This does not delete objects; it only clears the list used by the renderer.
    void clearObjects() {
        for (GameObject* obj : m_rootObjects) {
            if (obj) {
                obj->setScene(nullptr);
            }
        }

        m_rootObjects.clear();
    }

    // Starts scripts attached to all root objects and their children.
    void startObjects() {
        for (GameObject* obj : m_rootObjects) {
            if (obj) {
                obj->startScripts();
            }
        }
    }

    // Updates scripts attached to all root objects and their children.
    void updateObjects(float deltaTime) {
        for (GameObject* obj : m_rootObjects) {
            if (obj) {
                obj->updateScripts(deltaTime);
            }
        }
    }

    Camera& camera() {
        return m_camera;
    }

    // The renderer starts from these root objects and traverses the scene graph.
    std::vector<GameObject*>& rootObjects() {
        return m_rootObjects;
    }

    const std::vector<GameObject*>& rootObjects() const {
        return m_rootObjects;
    }

    const Camera& camera() const {
        return m_camera;
    }

private:
    GameObject* findObjectInTree(GameObject* obj, const std::string& objectName);
    const GameObject* findObjectInTree(const GameObject* obj, const std::string& objectName) const;
};

} // namespace nut
