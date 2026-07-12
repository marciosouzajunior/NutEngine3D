#pragma once

#include "../core/Mesh.h"
#include "../core/Scene.h"
#include "../../assets/scripts/DummyScript.h"
#include "../../assets/scripts/PlayerMoveScript.h"
#include "../../assets/scripts/SpinScript.h"
#include <stddef.h>
#include <stdint.h>
#ifdef ARDUINO
#include "../core/FixedVector.h"
#include "../core/RuntimeLimits.h"
#else
#include <string>
#include <vector>
#endif

namespace nut {

// RuntimeScene is the runtime form of a compiled scene blob.
//
// Pipeline role:
// - SceneBinaryLoader fills this object after reading the compiled binary scene.
// - The update step runs compact compiled script instances stored here.
// - The renderers read object transforms from this scene every frame.
//
// Important split between targets:
// - desktop: materializes Mesh objects so the generic renderer can traverse them
// - Nano: keeps only compact mesh metadata and streams vertex/edge data from flash
//
// So this class sits between:
// - input side: SceneBinaryLoader
// - simulation side: compiled script updates
// - output side: renderers
class RuntimeScene : public Scene {
public:
    // Compiled config payload for SpinScript inside the scene binary.
    struct NutSpinConfig {
        float rotationSpeedX = 0.0f;
        float rotationSpeedY = 0.0f;
        float rotationSpeedZ = 0.0f;
    };

    // Compiled config payload for DummyScript inside the scene binary.
    struct NutDummyConfig {
        bool enabled = true;
    };

    // Compiled config payload for PlayerMoveScript inside the scene binary.
    struct NutPlayerMoveConfig {
        float unitsPerSecondX = 0.0f;
        float unitsPerSecondY = 0.0f;
        float unitsPerSecondZ = 0.0f;
    };

    // One runtime script instance decoded from the scene binary.
    //
    // It points to a GameObject by index and stores only the compact config
    // needed to update that script at runtime.
    struct NutScriptInstance {
        uint16_t objectIndex = 0;
        uint16_t scriptId = 0;
        union Config {
            NutSpinConfig spin;
            NutDummyConfig dummy;
            NutPlayerMoveConfig playerMove;

            Config() : spin() {}
        } config;
    };

private:
#ifdef ARDUINO
    using ObjectNameParam = const char*;
    using RootObjectList = FixedVector<GameObject*, NUT_MAX_ROOT_OBJECTS>;
    using OwnedObjectList = FixedVector<GameObject*, NUT_MAX_OWNED_OBJECTS>;
    using OwnedMeshList = FixedVector<Mesh*, NUT_MAX_OWNED_MESHES>;
    using ScriptInstanceList = FixedVector<NutScriptInstance, NUT_MAX_OWNED_SCRIPTS>;

    // Placement-new constructs each GameObject inside one of these fixed SRAM
    // slots instead of allocating it from the heap. Every slot must therefore
    // have both sizeof(GameObject) bytes and GameObject's required alignment.
    // Aligning only a 2D byte array would not guarantee that later rows start
    // at an address valid for GameObject.
    struct ObjectStorageSlot {
        alignas(GameObject) uint8_t bytes[sizeof(GameObject)];
    };

    // Nano-side descriptor telling the renderer where one mesh lives in the
    // binary scene blob. The actual geometry stays in flash.
    struct BinaryMeshInfo {
        uint16_t geometryOffset = 0;
        uint16_t vertexCount = 0;
        uint16_t edgeCount = 0;
    };
    FixedVector<BinaryMeshInfo, NUT_MAX_MESHES> m_binaryMeshes;
    ObjectStorageSlot m_objectStorage[NUT_MAX_OWNED_OBJECTS];
    const uint8_t* m_binarySceneData = nullptr;
    uint16_t m_binaryMeshDataOffset = 0;
#else
    using ObjectNameParam = const std::string&;
    using RootObjectList = std::vector<GameObject*>;
    using OwnedObjectList = std::vector<GameObject*>;
    using OwnedMeshList = std::vector<Mesh*>;
    using ScriptInstanceList = std::vector<NutScriptInstance>;
#endif
    RootObjectList m_rootObjects;
    OwnedObjectList m_ownedObjects;
    OwnedMeshList m_ownedMeshes;
    ScriptInstanceList m_scriptInstances;

    GameObject* allocateObject(ObjectNameParam name);
    bool storeObject(GameObject* object);
    void destroyObject(GameObject* object);

    bool storeMesh(Mesh* mesh);
    void destroyMesh(Mesh* mesh);

    bool storeScriptInstance(const NutScriptInstance& instance);
    void resetBinarySceneState();

public:
    ~RuntimeScene() override;

    // Drop every loaded object/script/mesh from the previous compiled scene.
    void clearLoadedData();

    bool addObject(GameObject* obj) override;
    void clearObjects() override;
    size_t rootObjectCount() const override;
    GameObject* rootObjectAt(size_t index) override;
    const GameObject* rootObjectAt(size_t index) const override;

    // Access every runtime-owned object in load order.
    // The Nano renderer uses this flat list to avoid recursive scene traversal.
    size_t loadedObjectCount() const;
    GameObject* loadedObjectAt(size_t index);
    const GameObject* loadedObjectAt(size_t index) const;

    GameObject* createObject(ObjectNameParam name);

    // Create a runtime mesh container.
    // On Nano this is intentionally a no-op because mesh geometry is streamed.
    Mesh* createMesh();

    // Per-frame simulation entry point for compiled scene behavior.
    void onUpdate(float deltaTime) override;

    // Store one decoded compiled script instance.
    bool addScriptInstance(const NutScriptInstance& instance);

    // Execute all compiled script behavior for the current frame.
    void runCompiledScripts(float deltaTime);

#ifdef ARDUINO
    // Register the source scene blob and the start of its mesh-geometry region.
    void setBinarySceneData(const uint8_t* data, uint16_t meshDataOffset);

    // Store Nano mesh metadata so the renderer can stream geometry later.
    bool addBinaryMeshInfo(uint16_t geometryOffset, uint16_t vertexCount, uint16_t edgeCount);

    // Query one mesh description from the binary-mesh metadata table.
    bool getBinaryMeshInfo(int16_t meshIndex, uint16_t& geometryOffset, uint16_t& vertexCount, uint16_t& edgeCount) const;

    // Raw access used by the Nano renderer when it streams geometry from flash.
    const uint8_t* binarySceneData() const;
    uint16_t binaryMeshDataOffset() const;
#endif
};

} // namespace nut
