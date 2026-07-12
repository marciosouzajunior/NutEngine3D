#include "SceneManager.h"

namespace nut {

SceneManager::SceneManager()
    : m_activeScene(nullptr), m_pendingScene(nullptr) {}

void SceneManager::registerScene(uint16_t id, Scene* scene) {
    if (!scene) {
        return;
    }

    m_scenes[id] = scene;
    scene->setSceneManager(this);
}

void SceneManager::changeScene(uint16_t id) {
    auto it = m_scenes.find(id);
    if (it == m_scenes.end()) {
        return;
    }

    m_pendingScene = it->second;
}

void SceneManager::start() {
    applyPendingSceneChange();
}

void SceneManager::update(float deltaTime) {
    if (m_activeScene) {
        m_activeScene->onUpdate(deltaTime);
        m_activeScene->updateObjects(deltaTime);
    }

    applyPendingSceneChange();
}

void SceneManager::shutdown() {
    if (m_activeScene) {
        m_activeScene->onExit();
        m_activeScene = nullptr;
    }
}

Scene* SceneManager::activeScene() {
    return m_activeScene;
}

const Scene* SceneManager::activeScene() const {
    return m_activeScene;
}

void SceneManager::applyPendingSceneChange() {
    if (!m_pendingScene || m_pendingScene == m_activeScene) {
        m_pendingScene = nullptr;
        return;
    }

    if (m_activeScene) {
        m_activeScene->onExit();
    }

    m_activeScene = m_pendingScene;
    m_pendingScene = nullptr;
    m_activeScene->onEnter();
    m_activeScene->startObjects();
}

} // namespace nut
