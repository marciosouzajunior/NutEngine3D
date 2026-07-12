#include "Scene.h"
#ifndef ARDUINO
#include "SceneManager.h"
#endif

namespace nut {

Scene::Scene() : m_sceneManager(nullptr) {}

void Scene::setSceneManager(SceneManager* sceneManager) {
    m_sceneManager = sceneManager;
}

void Scene::requestSceneChange(uint16_t sceneId) {
#ifndef ARDUINO
    if (m_sceneManager) {
        m_sceneManager->changeScene(sceneId);
    }
#else
    (void)sceneId;
#endif
}

#ifndef ARDUINO
GameObject* Scene::findObject(const std::string& objectName) {
    for (size_t i = 0; i < rootObjectCount(); ++i) {
        GameObject* obj = rootObjectAt(i);
        GameObject* found = findObjectInTree(obj, objectName);
        if (found) {
            return found;
        }
    }

    return nullptr;
}

const GameObject* Scene::findObject(const std::string& objectName) const {
    for (size_t i = 0; i < rootObjectCount(); ++i) {
        const GameObject* obj = rootObjectAt(i);
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

    for (size_t i = 0; i < obj->childObjectCount(); ++i) {
        GameObject* child = obj->childObjectAt(i);
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

    for (size_t i = 0; i < obj->childObjectCount(); ++i) {
        const GameObject* child = obj->childObjectAt(i);
        const GameObject* found = findObjectInTree(child, objectName);
        if (found) {
            return found;
        }
    }

    return nullptr;
}
#endif

} // namespace nut
