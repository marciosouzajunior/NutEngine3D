#include "RuntimeScene.h"
#include <stdint.h>
#include <string.h>
#include <new>

namespace nut {

GameObject* RuntimeScene::allocateObject(ObjectNameParam name) {
#ifdef ARDUINO
    if (m_ownedObjects.size() >= NUT_MAX_OWNED_OBJECTS) {
        return nullptr;
    }

    void* storage = static_cast<void*>(m_objectStorage[m_ownedObjects.size()].bytes);
    memset(storage, 0, sizeof(GameObject));

    // Placement-new calls GameObject's constructor at this existing address.
    // Unlike ordinary `new GameObject`, it does not allocate memory: `storage`
    // already points to the next fixed slot owned by RuntimeScene.
    return new (storage) GameObject(name);
#else
    // This is ordinary desktop heap allocation. `std::nothrow` only changes
    // failure behavior so allocation failure returns nullptr instead of throwing.
    return new (std::nothrow) GameObject(name);
#endif
}

bool RuntimeScene::storeObject(GameObject* object) {
#ifdef ARDUINO
    return m_ownedObjects.push_back(object);
#else
    m_ownedObjects.push_back(object);
    return true;
#endif
}

void RuntimeScene::destroyObject(GameObject* object) {
    if (!object) {
        return;
    }

#ifdef ARDUINO
    // Placement-new does not pair with delete. Invoke the destructor directly;
    // the slot itself remains part of RuntimeScene and can be reused later.
    object->~GameObject();
#else
    delete object;
#endif
}

bool RuntimeScene::storeMesh(Mesh* mesh) {
#ifdef ARDUINO
    return m_ownedMeshes.push_back(mesh);
#else
    m_ownedMeshes.push_back(mesh);
    return true;
#endif
}

void RuntimeScene::destroyMesh(Mesh* mesh) {
    if (!mesh) {
        return;
    }

#ifdef ARDUINO
    mesh->~Mesh();
#else
    delete mesh;
#endif
}

bool RuntimeScene::storeScriptInstance(const NutScriptInstance& instance) {
#ifdef ARDUINO
    return m_scriptInstances.push_back(instance);
#else
    m_scriptInstances.push_back(instance);
    return true;
#endif
}

void RuntimeScene::resetBinarySceneState() {
#ifdef ARDUINO
    m_binaryMeshes.clear();
    m_binarySceneData = nullptr;
    m_binaryMeshDataOffset = 0;
#endif
}

RuntimeScene::~RuntimeScene() {
    clearLoadedData();
}

bool RuntimeScene::addObject(GameObject* obj) {
    if (!obj) {
        return false;
    }

    obj->setScene(this);
#ifdef ARDUINO
    return m_rootObjects.push_back(obj);
#else
    m_rootObjects.push_back(obj);
    return true;
#endif
}

void RuntimeScene::clearObjects() {
    for (GameObject* obj : m_rootObjects) {
        if (obj) {
            obj->setScene(nullptr);
        }
    }

    m_rootObjects.clear();
}

size_t RuntimeScene::rootObjectCount() const {
    return m_rootObjects.size();
}

GameObject* RuntimeScene::rootObjectAt(size_t index) {
    return index < m_rootObjects.size() ? m_rootObjects[index] : nullptr;
}

const GameObject* RuntimeScene::rootObjectAt(size_t index) const {
    return index < m_rootObjects.size() ? m_rootObjects[index] : nullptr;
}

size_t RuntimeScene::loadedObjectCount() const {
    return m_ownedObjects.size();
}

GameObject* RuntimeScene::loadedObjectAt(size_t index) {
    return index < m_ownedObjects.size() ? m_ownedObjects[index] : nullptr;
}

const GameObject* RuntimeScene::loadedObjectAt(size_t index) const {
    return index < m_ownedObjects.size() ? m_ownedObjects[index] : nullptr;
}

void RuntimeScene::clearLoadedData() {
    // Reset the scene graph visible through Scene first, then clear the
    // target-specific ownership/storage that was created by the loader.
    clearObjects();
    resetBinarySceneState();

    for (GameObject* object : m_ownedObjects) {
        destroyObject(object);
    }
    m_ownedObjects.clear();

    for (Mesh* mesh : m_ownedMeshes) {
        destroyMesh(mesh);
    }
    m_ownedMeshes.clear();

    m_scriptInstances.clear();
}

// Desktop keeps std::string names for editor/search features; Nano stores only a pointer.
GameObject* RuntimeScene::createObject(ObjectNameParam name) {
    GameObject* object = allocateObject(name);
    if (!object) {
        return nullptr;
    }

    if (!storeObject(object)) {
        destroyObject(object);
        return nullptr;
    }

    return object;
}

Mesh* RuntimeScene::createMesh() {
#ifdef ARDUINO
    // The Nano renderer reads mesh geometry directly from the compiled scene
    // blob in flash, so no Mesh object is built here.
    return nullptr;
#else
    Mesh* mesh = new (std::nothrow) Mesh();
    if (!mesh) {
        return nullptr;
    }

    if (!storeMesh(mesh)) {
        destroyMesh(mesh);
        return nullptr;
    }

    return mesh;
#endif
}

void RuntimeScene::onUpdate(float deltaTime) {
    // The runtime scene owns the compact compiled-script runtime.
    // That keeps desktop and Nano on the same behavioral path.
    runCompiledScripts(deltaTime);
}

bool RuntimeScene::addScriptInstance(const NutScriptInstance& instance) {
    if (instance.objectIndex >= m_ownedObjects.size()) {
        return false;
    }

    return storeScriptInstance(instance);
}

void RuntimeScene::runCompiledScripts(float deltaTime) {
    // This is the simulation step for compiled scripts.
    // Each instance targets an object by index and applies only the behavior
    // encoded in the compact scene payload.
    for (NutScriptInstance& script : m_scriptInstances) {
        if (script.objectIndex >= m_ownedObjects.size()) {
            continue;
        }

        GameObject* object = m_ownedObjects[script.objectIndex];
        if (!object) {
            continue;
        }

        switch (script.scriptId) {
        case SpinScript::kScriptId:
            object->transform.rotation.x += script.config.spin.rotationSpeedX * deltaTime;
            object->transform.rotation.y += script.config.spin.rotationSpeedY * deltaTime;
            object->transform.rotation.z += script.config.spin.rotationSpeedZ * deltaTime;
            break;
        case DummyScript::kScriptId:
            (void)script.config.dummy.enabled;
            break;
        case PlayerMoveScript::kScriptId:
            object->transform.position.x += inputState().moveX * script.config.playerMove.unitsPerSecondX * deltaTime;
            object->transform.position.y += inputState().moveY * script.config.playerMove.unitsPerSecondY * deltaTime;
            if (inputState().primaryPressed) {
                object->transform.position.z += script.config.playerMove.unitsPerSecondZ * deltaTime;
            }
            break;
        default:
            break;
        }
    }
}

#ifdef ARDUINO
void RuntimeScene::setBinarySceneData(const uint8_t* data, uint16_t meshDataOffset) {
    // Keep a pointer to the original compiled scene blob so the Nano renderer
    // can stream mesh geometry from flash instead of copying it into RAM.
    m_binarySceneData = data;
    m_binaryMeshDataOffset = meshDataOffset;
    m_binaryMeshes.clear();
}

bool RuntimeScene::addBinaryMeshInfo(uint16_t geometryOffset, uint16_t vertexCount, uint16_t edgeCount) {
    // Store only enough information for the Nano renderer to find a mesh inside
    // the binary blob later.
    BinaryMeshInfo info;
    info.geometryOffset = geometryOffset;
    info.vertexCount = vertexCount;
    info.edgeCount = edgeCount;
    return m_binaryMeshes.push_back(info);
}

bool RuntimeScene::getBinaryMeshInfo(int16_t meshIndex, uint16_t& geometryOffset, uint16_t& vertexCount, uint16_t& edgeCount) const {
    if (meshIndex < 0 || static_cast<size_t>(meshIndex) >= m_binaryMeshes.size()) {
        return false;
    }

    const BinaryMeshInfo& info = m_binaryMeshes[meshIndex];
    geometryOffset = info.geometryOffset;
    vertexCount = info.vertexCount;
    edgeCount = info.edgeCount;
    return true;
}

const uint8_t* RuntimeScene::binarySceneData() const {
    return m_binarySceneData;
}

uint16_t RuntimeScene::binaryMeshDataOffset() const {
    return m_binaryMeshDataOffset;
}
#endif

} // namespace nut
