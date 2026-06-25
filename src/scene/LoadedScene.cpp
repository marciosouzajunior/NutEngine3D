#include "LoadedScene.h"

namespace nut {

void LoadedScene::clearLoadedData() {
    clearObjects();
    m_ownedScripts.clear();
    m_ownedObjects.clear();
    m_ownedMeshes.clear();
}

GameObject* LoadedScene::createObject(const std::string& name) {
    m_ownedObjects.push_back(std::make_unique<GameObject>(name));
    return m_ownedObjects.back().get();
}

Mesh* LoadedScene::createMesh() {
    m_ownedMeshes.push_back(std::make_unique<Mesh>());
    return m_ownedMeshes.back().get();
}

void LoadedScene::attachScript(GameObject* object, std::unique_ptr<Script> script) {
    if (!object || !script) {
        return;
    }

    Script* rawScript = script.get();
    m_ownedScripts.push_back(std::move(script));
    object->addScript(rawScript);
}

void LoadedScene::onUpdate(float deltaTime) {
    // Scene-wide behavior can be added later.
    // Object-specific behavior comes from scripts loaded from the scene file.
    (void)deltaTime;
}

} // namespace nut
