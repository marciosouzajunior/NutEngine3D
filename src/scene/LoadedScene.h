#pragma once

#include "../core/Mesh.h"
#include "../core/Scene.h"
#include "../core/Script.h"
#include <memory>
#include <vector>

namespace nut {

// LoadedScene owns objects, meshes, and scripts created from a .nutscene file.
// This is the runtime representation that SceneLoader fills from JSON.
class LoadedScene : public Scene {
private:
    std::vector<std::unique_ptr<GameObject>> m_ownedObjects;
    std::vector<std::unique_ptr<Mesh>> m_ownedMeshes;
    std::vector<std::unique_ptr<Script>> m_ownedScripts;

public:
    void clearLoadedData();

    GameObject* createObject(const std::string& name);
    Mesh* createMesh();
    void attachScript(GameObject* object, std::unique_ptr<Script> script);

    void onUpdate(float deltaTime) override;
};

} // namespace nut
