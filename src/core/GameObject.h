#pragma once

#include "Mesh.h"
#include "Script.h"
#include "Transform.h"
#include <string>
#include <vector>

namespace nut {

class Scene;

// GameObject represents an entity in a scene.
//
// Mental model:
// - Mesh = what shape this object has, written in local coordinates.
// - Transform = where/how that shape appears in the scene.
// - Script = code that makes this object do something during the game.
// - GameObject = Mesh + Transform + Scripts.
class GameObject {
private:
    std::string m_name;
    Scene* m_scene;
    std::vector<Script*> m_scripts;

public:
    // Transform tells the engine where/how this object appears in the scene.
    // Changing transform does not edit the mesh vertices; it changes the matrix
    // used to draw this object on screen.
    Transform transform;

    // Mesh is the shape data used by this object, in local coordinates.
    // Multiple GameObjects can point to the same Mesh and still appear in different
    // places because each GameObject has its own Transform.
    Mesh* mesh; // Pointer to allow sharing the same mesh across multiple objects.

    std::vector<GameObject*> children;

    explicit GameObject(const std::string& name = "GameObject")
        : m_name(name), m_scene(nullptr), mesh(nullptr) {}

    const std::string& name() const {
        return m_name;
    }

    void setName(const std::string& name) {
        m_name = name;
    }

    Scene* scene() {
        return m_scene;
    }

    const Scene* scene() const {
        return m_scene;
    }

    // Called by Scene when this object is added as a root object.
    // Children inherit the same scene so their scripts can also request scene changes.
    void setScene(Scene* scene) {
        m_scene = scene;

        for (GameObject* child : children) {
            if (child) {
                child->setScene(scene);
            }
        }
    }

    // Adds gameplay code to this GameObject.
    // The script object must stay alive while this GameObject uses it.
    void addScript(Script* script) {
        if (!script) {
            return;
        }

        for (Script* existingScript : m_scripts) {
            if (existingScript == script) {
                return;
            }
        }

        script->setGameObject(this);
        m_scripts.push_back(script);
    }

    // Finds the first script of a specific C++ type attached to this GameObject.
    // Example: HealthScript* health = player->getScript<HealthScript>();
    template <typename T>
    T* getScript() {
        for (Script* script : m_scripts) {
            T* typedScript = dynamic_cast<T*>(script);
            if (typedScript) {
                return typedScript;
            }
        }

        return nullptr;
    }

    template <typename T>
    const T* getScript() const {
        for (const Script* script : m_scripts) {
            const T* typedScript = dynamic_cast<const T*>(script);
            if (typedScript) {
                return typedScript;
            }
        }

        return nullptr;
    }

    // Called by Scene when the scene becomes active.
    void startScripts() {
        for (Script* script : m_scripts) {
            if (script) {
                script->onStart();
            }
        }

        for (GameObject* child : children) {
            if (child) {
                child->startScripts();
            }
        }
    }

    // Called by Scene once per frame.
    void updateScripts(float deltaTime) {
        for (Script* script : m_scripts) {
            if (script) {
                script->onUpdate(deltaTime);
            }
        }

        for (GameObject* child : children) {
            if (child) {
                child->updateScripts(deltaTime);
            }
        }
    }

    // Adds a child object to this object.
    // The child's transform parent is set to this object's transform,
    // meaning the child will move, rotate, and scale relative to this object.
    void addChild(GameObject* child) {
        if (child) {
            child->transform.parent = &this->transform;
            child->setScene(m_scene);
            children.push_back(child);
        }
    }

    // Removes child links from this object.
    // This does not delete the child objects; it only clears the hierarchy relationship.
    void clearChildren() {
        for (GameObject* child : children) {
            if (child) {
                child->transform.parent = nullptr;
                child->setScene(nullptr);
            }
        }

        children.clear();
    }
};

} // namespace nut
