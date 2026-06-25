#include "Script.h"
#include "GameObject.h"
#include "Scene.h"

namespace nut {

Script::Script() : m_gameObject(nullptr) {}

void Script::setGameObject(GameObject* gameObject) {
    m_gameObject = gameObject;
}

GameObject* Script::gameObject() {
    return m_gameObject;
}

const GameObject* Script::gameObject() const {
    return m_gameObject;
}

Scene* Script::scene() {
    if (!m_gameObject) {
        return nullptr;
    }

    return m_gameObject->scene();
}

const Scene* Script::scene() const {
    if (!m_gameObject) {
        return nullptr;
    }

    return m_gameObject->scene();
}

GameObject* Script::findObject(const std::string& objectName) {
    if (!scene()) {
        return nullptr;
    }

    return scene()->findObject(objectName);
}

const GameObject* Script::findObject(const std::string& objectName) const {
    if (!scene()) {
        return nullptr;
    }

    return scene()->findObject(objectName);
}

void Script::requestSceneChange(const std::string& sceneName) {
    if (m_gameObject && m_gameObject->scene()) {
        m_gameObject->scene()->requestSceneChange(sceneName);
    }
}

} // namespace nut
