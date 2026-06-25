#pragma once

#include <string>

namespace nut {

class GameObject;
class Scene;

// Script is a reusable behavior that can be attached to a GameObject.
//
// Mental model:
// - GameObject = the thing in the scene.
// - Script = code that makes that thing do something during the game.
class Script {
private:
    // The GameObject that owns this script.
    // It starts as null and is filled when GameObject::addScript() is called.
    GameObject* m_gameObject;

public:
    Script();

    // A virtual destructor keeps cleanup safe when scripts are handled through Script*.
    virtual ~Script() = default;

    // Called when the scene becomes active, after objects are created.
    // Use this for setup that needs access to gameObject(), transform, or scene data.
    virtual void onStart() {}

    // Called once per frame while the owning GameObject is active in the scene.
    // deltaTime is the elapsed time in seconds since the previous frame.
    // Use it to make movement/animation frame-rate independent.
    virtual void onUpdate(float deltaTime) {}

    // The engine sets this when the script is attached to a GameObject.
    // User code normally should not call this directly; use GameObject::addScript().
    void setGameObject(GameObject* gameObject);

    // Access the object this script controls.
    // Example: gameObject()->transform.position.x += speed * deltaTime;
    GameObject* gameObject();
    const GameObject* gameObject() const;

    // Access the scene that owns this script's GameObject.
    // This is useful for finding and interacting with other objects.
    Scene* scene();
    const Scene* scene() const;

    // Finds another object in the same scene by name.
    // Example: GameObject* door = findObject("Door");
    GameObject* findObject(const std::string& objectName);
    const GameObject* findObject(const std::string& objectName) const;

    // Scripts can request scene changes through their owning GameObject/Scene.
    // The SceneManager applies the change after the current update finishes.
    void requestSceneChange(const std::string& sceneName);
};

} // namespace nut
