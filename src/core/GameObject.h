#pragma once

#include "Mesh.h"
#include "FixedVector.h"
#include "RuntimeLimits.h"
#include "Script.h"
#include "Transform.h"
#include <stdint.h>
#ifdef ARDUINO
#include <Arduino.h>
#else
#include <string>
#endif

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
#ifdef ARDUINO
    // Non-owning pointer into SceneBinaryLoader's persistent string table.
    // Keeping only this 2-byte AVR pointer avoids one buffer/allocation per
    // object. The loader-owned table must outlive every object in the scene.
    const char* m_name;
#else
    std::string m_name;
#endif
    FixedVector<Script*, NUT_MAX_SCRIPTS_PER_OBJECT> m_scripts;
    FixedVector<GameObject*, NUT_MAX_CHILDREN_PER_OBJECT> m_children;
#ifdef ARDUINO
    int16_t m_meshIndex;
#endif
    bool m_enabled;
    Scene* m_scene;

public:
    // Transform tells the engine where/how this object appears in the scene.
    // Changing transform does not edit the mesh vertices; it changes the matrix
    // used to draw this object on screen.
    Transform transform;

    // Mesh is the shape data used by this object, in local coordinates.
    // Multiple GameObjects can point to the same Mesh and still appear in different
    // places because each GameObject has its own Transform.
    Mesh* mesh; // Pointer to allow sharing the same mesh across multiple objects.

#ifdef ARDUINO
    explicit GameObject(const char* name = "GameObject")
        : m_name(name)
        , m_scripts()
        , m_children()
        , m_meshIndex(-1)
        , m_enabled(true)
        , m_scene(nullptr)
        , transform()
        , mesh(nullptr)
#else
    explicit GameObject(const std::string& name = "GameObject")
        : m_name(name)
        , m_scripts()
        , m_children()
        , m_enabled(true)
        , m_scene(nullptr)
        , transform()
        , mesh(nullptr)
#endif
    {}

#ifdef ARDUINO
    const char* name() const {
        return m_name ? m_name : "";
    }

    void setName(const char* name) {
        m_name = name;
    }

    int16_t meshIndex() const {
        return m_meshIndex;
    }

    void setMeshIndex(int16_t meshIndex) {
        m_meshIndex = meshIndex;
    }
#else
    const std::string& name() const {
        return m_name;
    }

    void setName(const std::string& name) {
        m_name = name;
    }
#endif

    Scene* scene() {
        return m_scene;
    }

    const Scene* scene() const {
        return m_scene;
    }

    bool isEnabled() const {
        return m_enabled;
    }

    void setEnabled(bool enabled) {
        m_enabled = enabled;
    }

    // Called by Scene when this object is added as a root object.
    // Children inherit the same scene so their scripts can also request scene changes.
    void setScene(Scene* scene) {
        m_scene = scene;

        for (GameObject* child : m_children) {
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
        if (m_scripts.push_back(script)) {
            script->setGameObject(this);
        }
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
        if (m_enabled) {
            for (Script* script : m_scripts) {
                if (script) {
                    script->onStart();
                }
            }
        }

        for (GameObject* child : m_children) {
            if (child) {
                child->startScripts();
            }
        }
    }

    // Called by Scene once per frame.
    void updateScripts(float deltaTime) {
        if (m_enabled) {
            for (Script* script : m_scripts) {
                if (script) {
                    script->onUpdate(deltaTime);
                }
            }
        }

        for (GameObject* child : m_children) {
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
            if (m_children.push_back(child)) {
                child->transform.parent = &this->transform;
                child->setScene(m_scene);
            }
        }
    }

    size_t childObjectCount() const {
        return m_children.size();
    }

    GameObject* childObjectAt(size_t index) {
        return index < m_children.size() ? m_children[index] : nullptr;
    }

    const GameObject* childObjectAt(size_t index) const {
        return index < m_children.size() ? m_children[index] : nullptr;
    }

    // Removes child links from this object.
    // This does not delete the child objects; it only clears the hierarchy relationship.
    void clearChildren() {
        for (GameObject* child : m_children) {
            if (child) {
                child->transform.parent = nullptr;
                child->setScene(nullptr);
            }
        }

        m_children.clear();
    }
};

} // namespace nut
