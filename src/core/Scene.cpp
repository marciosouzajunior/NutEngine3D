#include "Scene.h"
#include "SceneManager.h"

namespace nut {

Scene::Scene() : m_sceneManager(nullptr) {}

void Scene::setSceneManager(SceneManager* sceneManager) {
    m_sceneManager = sceneManager;
}

void Scene::requestSceneChange(const std::string& sceneName) {
    if (m_sceneManager) {
        m_sceneManager->changeScene(sceneName);
    }
}

GameObject* Scene::findObject(const std::string& objectName) {
    for (GameObject* obj : m_rootObjects) {
        GameObject* found = findObjectInTree(obj, objectName);
        if (found) {
            return found;
        }
    }

    return nullptr;
}

const GameObject* Scene::findObject(const std::string& objectName) const {
    for (const GameObject* obj : m_rootObjects) {
        const GameObject* found = findObjectInTree(obj, objectName);
        if (found) {
            return found;
        }
    }

    return nullptr;
}

GameObject* Scene::findObjectInTree(GameObject* obj, const std::string& objectName) {
    if (!obj) {
        return nullptr;
    }

    if (obj->name() == objectName) {
        return obj;
    }

    for (GameObject* child : obj->children) {
        GameObject* found = findObjectInTree(child, objectName);
        if (found) {
            return found;
        }
    }

    return nullptr;
}

const GameObject* Scene::findObjectInTree(const GameObject* obj, const std::string& objectName) const {
    if (!obj) {
        return nullptr;
    }

    if (obj->name() == objectName) {
        return obj;
    }

    for (const GameObject* child : obj->children) {
        const GameObject* found = findObjectInTree(child, objectName);
        if (found) {
            return found;
        }
    }

    return nullptr;
}

} // namespace nut
