#include "SceneBinaryLoader.h"

#ifdef ARDUINO
#include <Arduino.h>
#include <avr/pgmspace.h>
#include <new>
#include "../core/FixedVector.h"
#include "../core/RuntimeLimits.h"
#else
#include <iostream>
#include <string>
#include <vector>
#endif

namespace nut {
namespace {

#ifdef ARDUINO
void logMessage(const __FlashStringHelper* message) {
    Serial.println(message);
}

void logMessageValue(const __FlashStringHelper* prefix, int value) {
    Serial.print(prefix);
    Serial.println(value);
}

#define NUT_LOG_LITERAL(text) F(text)
#else
void logMessage(const char* message) {
    std::cout << message << std::endl;
}

void logMessageValue(const char* prefix, int value) {
    std::cout << prefix << value << std::endl;
}

#define NUT_LOG_LITERAL(text) text
#endif

#ifdef ARDUINO
#define NUT_TRACE_LOAD(text) do {} while (0)
#define NUT_TRACE_VALUE(text, value) do { (void)(value); } while (0)
#else
#define NUT_TRACE_LOAD(text) do {} while (0)
#define NUT_TRACE_VALUE(text, value) do { (void)(value); } while (0)
#endif

struct Header {
    // Global scene-level metadata written by the scene compiler at the start
    // of the binary blob.
    uint16_t version = 0;
    uint16_t stringTableSize = 0;
    uint16_t meshCount = 0;
    uint16_t objectCount = 0;
    uint16_t scriptCount = 0;
    uint16_t meshGeometrySize = 0;
    uint16_t scriptConfigSize = 0;
    float cameraPosition[3] {0.0f, 0.0f, -5.0f};
    float cameraRotation[3] {0.0f, 0.0f, 0.0f};
    float cameraFovDegrees = 60.0f;
};

using MeshRecord = SceneBinaryLoader::MeshRecord;
using ObjectRecord = SceneBinaryLoader::ObjectRecord;
using ScriptRecord = SceneBinaryLoader::ScriptRecord;

class ByteReader {
private:
    const uint8_t* m_data;
    size_t m_size;
    size_t m_pos;

    uint8_t readByteAt(size_t index) const {
#ifdef ARDUINO
        return pgm_read_byte(m_data + index);
#else
        return m_data[index];
#endif
    }

public:
    ByteReader(const uint8_t* data, size_t size)
        : m_data(data), m_size(size), m_pos(0) {}

    bool canRead(size_t count) const {
        return m_pos + count <= m_size;
    }

    size_t position() const {
        return m_pos;
    }

    bool seek(size_t newPos) {
        if (newPos > m_size) {
            return false;
        }

        m_pos = newPos;
        return true;
    }

    bool skip(size_t count) {
        return seek(m_pos + count);
    }

    bool readU8(uint8_t& value) {
        if (!canRead(1)) {
            return false;
        }

        value = readByteAt(m_pos++);
        return true;
    }

    bool readU16(uint16_t& value) {
        if (!canRead(2)) {
            return false;
        }

        value = static_cast<uint16_t>(readByteAt(m_pos))
              | (static_cast<uint16_t>(readByteAt(m_pos + 1)) << 8);
        m_pos += 2;
        return true;
    }

    bool readI16(int16_t& value) {
        uint16_t raw = 0;
        if (!readU16(raw)) {
            return false;
        }

        value = static_cast<int16_t>(raw);
        return true;
    }

    bool readF32(float& value) {
        if (!canRead(4)) {
            return false;
        }

        union {
            uint32_t bits;
            float number;
        } convert {};

        convert.bits = static_cast<uint32_t>(readByteAt(m_pos))
                     | (static_cast<uint32_t>(readByteAt(m_pos + 1)) << 8)
                     | (static_cast<uint32_t>(readByteAt(m_pos + 2)) << 16)
                     | (static_cast<uint32_t>(readByteAt(m_pos + 3)) << 24);
        value = convert.number;
        m_pos += 4;
        return true;
    }
};

#ifdef ARDUINO
using LoadWorkspace = SceneBinaryLoader::LoadWorkspace;

// Persistent storage for all object names in the active Nano scene.
//
// The scene blob does not repeat a complete string in every object record.
// Instead it contains one packed block such as "Cube\0Sphere\0", and each
// record contains a 16-bit offset into that block. During loading we copy the
// block from PROGMEM here, then GameObject::m_name points directly at the
// appropriate byte. No per-name allocation or copy is needed.
//
// Unlike LoadWorkspace, this array cannot share memory with the renderer:
// those GameObject pointers remain valid only while this table stays alive.
static uint8_t g_sceneStringTable[NUT_MAX_STRING_TABLE_BYTES];

static_assert(sizeof(LoadWorkspace) == SceneBinaryLoader::NANO_SCRATCH_BYTES,
    "Nano scene-loader scratch must match the configured loader workspace.");
#else
using StringTableBuffer = std::vector<uint8_t>;
using MeshRecordBuffer = std::vector<MeshRecord>;
using ObjectRecordBuffer = std::vector<ObjectRecord>;
using ScriptRecordBuffer = std::vector<ScriptRecord>;
using MeshPointerBuffer = std::vector<Mesh*>;
using ObjectPointerBuffer = std::vector<GameObject*>;
#endif

bool readHeader(ByteReader& reader, Header& header) {
    // The loader starts by checking the magic and reading the fixed-size scene header.
    uint8_t magic[4] {};
    for (uint8_t& byte : magic) {
        if (!reader.readU8(byte)) {
            return false;
        }
    }

    if (magic[0] != 'N' || magic[1] != 'U' || magic[2] != 'T' || magic[3] != '0') {
        return false;
    }

    return reader.readU16(header.version)
        && reader.readU16(header.stringTableSize)
        && reader.readU16(header.meshCount)
        && reader.readU16(header.objectCount)
        && reader.readU16(header.scriptCount)
        && reader.readU16(header.meshGeometrySize)
        && reader.readU16(header.scriptConfigSize)
        && reader.readF32(header.cameraPosition[0])
        && reader.readF32(header.cameraPosition[1])
        && reader.readF32(header.cameraPosition[2])
        && reader.readF32(header.cameraRotation[0])
        && reader.readF32(header.cameraRotation[1])
        && reader.readF32(header.cameraRotation[2])
        && reader.readF32(header.cameraFovDegrees);
}

#ifdef ARDUINO
const char* readStringFromTable(
    const SceneBinaryLoader::LoadWorkspace& workspace,
    uint16_t offset
) {
    if (offset >= workspace.stringTableSize) {
        return "GameObject";
    }

    // The offset lands on the first character of a null-terminated name.
    // Returning a pointer avoids constructing or allocating another string.
    return reinterpret_cast<const char*>(&workspace.stringTable[offset]);
}
#else
std::string readStringFromTable(const StringTableBuffer& table, uint16_t offset) {
    if (offset >= table.size()) {
        return "";
    }

    const char* start = reinterpret_cast<const char*>(table.data() + offset);
    return std::string(start);
}
#endif

bool decodeScriptConfig(
    const uint8_t* data,
    size_t size,
    const SceneBinaryLoader::ScriptRecord& scriptRecord,
    uint16_t objectIndex,
    CompiledScriptInstance& instance
) {
    // The engine does not interpret game configuration. It only validates the
    // fixed payload capacity and copies the bytes into the runtime instance.
    if (scriptRecord.configOffset + scriptRecord.configSize > size
        || scriptRecord.configSize > CompiledScriptInstance::CONFIG_CAPACITY) {
        return false;
    }

    instance.objectIndex = objectIndex;
    instance.scriptId = scriptRecord.scriptId;

    ByteReader configReader(data + scriptRecord.configOffset, scriptRecord.configSize);
    for (uint16_t i = 0; i < scriptRecord.configSize; ++i) {
        uint8_t byte = 0;
        if (!configReader.readU8(byte)) {
            return false;
        }
        instance.setConfigByte(i, byte);
    }
    return true;
}

} // namespace

bool SceneBinaryLoader::load(
    const uint8_t* data,
    size_t size,
    RuntimeScene& outScene
#ifdef ARDUINO
    , void* scratch,
    size_t scratchSize
#endif
) {
    // High-level loading pipeline:
    // 1. read header
    // 2. decode string/mesh/object/script tables
    // 3. clear the previous runtime scene
    // 4. build runtime objects/meshes
    // 5. decode compact script instances
    // 6. rebuild hierarchy links
    if (!data || size == 0) {
        logMessage(NUT_LOG_LITERAL("Scene binary is empty."));
        return false;
    }

    NUT_TRACE_LOAD("loader-core: begin");
    ByteReader reader(data, size);
    Header header;
    if (!readHeader(reader, header)) {
        logMessage(NUT_LOG_LITERAL("Could not read scene binary header."));
        return false;
    }
    NUT_TRACE_LOAD("loader-core: header ok");
    NUT_TRACE_VALUE("loader-core: meshes=", header.meshCount);
    NUT_TRACE_VALUE("loader-core: objects=", header.objectCount);
    NUT_TRACE_VALUE("loader-core: scripts=", header.scriptCount);

    if (header.version != 2) {
        logMessageValue(NUT_LOG_LITERAL("Unsupported scene binary version: "), header.version);
        return false;
    }

    #ifdef ARDUINO
    if (header.stringTableSize > NUT_MAX_STRING_TABLE_BYTES
        || header.meshCount > NUT_MAX_MESHES
        || header.objectCount > NUT_MAX_OBJECTS
        || header.scriptCount > NUT_MAX_SCRIPT_RECORDS) {
        logMessage(NUT_LOG_LITERAL("Scene exceeds Nano runtime limits."));
        return false;
    }
    #endif

    #ifdef ARDUINO
    if (!scratch
        || scratchSize < sizeof(LoadWorkspace)
        || (reinterpret_cast<uintptr_t>(scratch) % alignof(LoadWorkspace)) != 0) {
        logMessage(NUT_LOG_LITERAL("Invalid Nano scene-loader scratch."));
        return false;
    }

    // Construct the temporary workspace in target-owned memory. No heap is
    // involved, and the caller may reuse these bytes as soon as load returns.
    // Default-initialize the POD without clearing its arrays. Every record is
    // decoded before use, while the counters below define the valid ranges.
    LoadWorkspace& workspace = *new (scratch) LoadWorkspace;
    workspace.stringTable = g_sceneStringTable;
    workspace.stringTableSize = 0;
    workspace.meshRecordCount = 0;
    workspace.objectRecordCount = 0;
    workspace.scriptRecordCount = 0;
    workspace.objectCount = 0;
    #else
    StringTableBuffer stringTable;
    #endif
#ifndef ARDUINO
    stringTable.resize(header.stringTableSize);
#endif
    for (uint16_t i = 0; i < header.stringTableSize; ++i) {
        // Copy the packed names once from the flash-backed scene reader into
        // persistent SRAM. Object records decoded later refer to these bytes by
        // offset; after object creation, their name pointers refer here directly.
        uint8_t byte = 0;
        if (!reader.readU8(byte)) {
            logMessage(NUT_LOG_LITERAL("Could not read string table."));
            return false;
        }
#ifdef ARDUINO
        workspace.stringTable[i] = byte;
        workspace.stringTableSize = static_cast<uint16_t>(i + 1);
#else
        stringTable[i] = byte;
#endif
    }
    NUT_TRACE_LOAD("loader-core: string table ok");

#ifndef ARDUINO
    MeshRecordBuffer meshRecords;
    meshRecords.resize(header.meshCount);
    for (MeshRecord& record : meshRecords) {
#else
    for (uint16_t meshIndex = 0; meshIndex < header.meshCount; ++meshIndex) {
        MeshRecord& record = workspace.meshRecords[meshIndex];
#endif
        if (!reader.readU16(record.vertexCount)
            || !reader.readU16(record.edgeCount)) {
            logMessage(NUT_LOG_LITERAL("Could not read mesh records."));
            return false;
        }
#ifdef ARDUINO
        workspace.meshRecordCount = static_cast<uint16_t>(meshIndex + 1);
#endif
    }
    NUT_TRACE_LOAD("loader-core: mesh records ok");

    const size_t meshGeometryOffset = reader.position();
    // Skip over the raw mesh-geometry payload for now.
    // We will either materialize it later (desktop) or keep offsets into it (Nano).
    if (!reader.skip(header.meshGeometrySize)) {
        logMessage(NUT_LOG_LITERAL("Scene binary mesh geometry is truncated."));
        return false;
    }
    NUT_TRACE_LOAD("loader-core: mesh geometry span ok");

#ifndef ARDUINO
    ObjectRecordBuffer objectRecords;
    objectRecords.resize(header.objectCount);
    for (ObjectRecord& record : objectRecords) {
#else
    for (uint16_t objectIndex = 0; objectIndex < header.objectCount; ++objectIndex) {
        ObjectRecord& record = workspace.objectRecords[objectIndex];
#endif
        if (!reader.readU16(record.nameOffset)
            || !reader.readI16(record.parentIndex)
            || !reader.readI16(record.meshIndex)
            || !reader.readU16(record.firstScriptIndex)
            || !reader.readU16(record.scriptCount)) {
            logMessage(NUT_LOG_LITERAL("Could not read object records."));
            return false;
        }

        for (float& value : record.position) {
            if (!reader.readF32(value)) {
                return false;
            }
        }

        for (float& value : record.rotation) {
            if (!reader.readF32(value)) {
                return false;
            }
        }

        for (float& value : record.scale) {
            if (!reader.readF32(value)) {
                return false;
            }
        }
#ifdef ARDUINO
        workspace.objectRecordCount = static_cast<uint16_t>(objectIndex + 1);
#endif
    }
    NUT_TRACE_LOAD("loader-core: object records ok");

#ifndef ARDUINO
    ScriptRecordBuffer scriptRecords;
    scriptRecords.resize(header.scriptCount);
    for (ScriptRecord& record : scriptRecords) {
#else
    for (uint16_t scriptIndex = 0; scriptIndex < header.scriptCount; ++scriptIndex) {
        ScriptRecord& record = workspace.scriptRecords[scriptIndex];
#endif
        if (!reader.readU16(record.scriptId)
            || !reader.readU16(record.configOffset)
            || !reader.readU16(record.configSize)) {
            logMessage(NUT_LOG_LITERAL("Could not read script records."));
            return false;
        }
#ifdef ARDUINO
        workspace.scriptRecordCount = static_cast<uint16_t>(scriptIndex + 1);
#endif
    }
    NUT_TRACE_LOAD("loader-core: script records ok");

    const size_t scriptConfigOffset = reader.position();
    // Skip over the packed script-config payload for now and come back to it
    // once objects and script records are known.
    if (!reader.skip(header.scriptConfigSize)) {
        logMessage(NUT_LOG_LITERAL("Scene binary script config is truncated."));
        return false;
    }
    NUT_TRACE_LOAD("loader-core: script config span ok");

    outScene.clearLoadedData();
    NUT_TRACE_LOAD("loader-core: scene cleared");

    // Camera state is scene-global, so it is restored directly from the header.
    outScene.camera().transform.position = math::Vec3(
        header.cameraPosition[0],
        header.cameraPosition[1],
        header.cameraPosition[2]
    );
    outScene.camera().transform.rotation = math::Vec3(
        header.cameraRotation[0],
        header.cameraRotation[1],
        header.cameraRotation[2]
    );
    outScene.camera().setFovDegrees(header.cameraFovDegrees);

#ifdef ARDUINO
    // On Nano, keep the original blob and store only compact mesh offsets/counts.
    outScene.setBinarySceneData(data, static_cast<uint16_t>(meshGeometryOffset));
    uint16_t runningMeshOffset = 0;
    for (uint16_t meshIndex = 0; meshIndex < workspace.meshRecordCount; ++meshIndex) {
        const MeshRecord& record = workspace.meshRecords[meshIndex];
        if (!outScene.addBinaryMeshInfo(runningMeshOffset, record.vertexCount, record.edgeCount)) {
            logMessage(NUT_LOG_LITERAL("Could not store binary mesh info."));
            return false;
        }

        runningMeshOffset = static_cast<uint16_t>(
            runningMeshOffset
            + static_cast<uint16_t>(record.vertexCount * sizeof(float) * 3)
            + static_cast<uint16_t>(record.edgeCount * sizeof(uint16_t) * 2)
        );
    }
    NUT_TRACE_LOAD("loader-core: mesh metadata stored");
#else
    // On desktop, build fully materialized Mesh objects from the binary payload.
    MeshPointerBuffer meshes;
    meshes.reserve(meshRecords.size());

    ByteReader meshReader(data + meshGeometryOffset, header.meshGeometrySize);
    for (const MeshRecord& record : meshRecords) {
        Mesh* mesh = outScene.createMesh();
        if (!mesh) {
            logMessage(NUT_LOG_LITERAL("Could not allocate mesh."));
            return false;
        }
        mesh->vertices.reserve(record.vertexCount);
        mesh->edges.reserve(record.edgeCount);

        for (uint16_t i = 0; i < record.vertexCount; ++i) {
            float x = 0.0f;
            float y = 0.0f;
            float z = 0.0f;
            if (!meshReader.readF32(x) || !meshReader.readF32(y) || !meshReader.readF32(z)) {
                logMessage(NUT_LOG_LITERAL("Could not read mesh vertices."));
                return false;
            }

            mesh->vertices.push_back(math::Vec3(x, y, z));
        }

        for (uint16_t i = 0; i < record.edgeCount; ++i) {
            uint16_t a = 0;
            uint16_t b = 0;
            if (!meshReader.readU16(a) || !meshReader.readU16(b)) {
                logMessage(NUT_LOG_LITERAL("Could not read mesh edges."));
                return false;
            }

            mesh->edges.push_back(Edge(static_cast<int>(a), static_cast<int>(b)));
        }

        meshes.push_back(mesh);
    }
    NUT_TRACE_LOAD("loader-core: meshes built");
#endif

    #ifndef ARDUINO
    ObjectPointerBuffer objects;
    objects.reserve(objectRecords.size());
    #endif

#ifdef ARDUINO
    // Build runtime objects from the decoded object records.
    for (uint16_t objectBuildIndex = 0; objectBuildIndex < workspace.objectRecordCount; ++objectBuildIndex) {
        const ObjectRecord& record = workspace.objectRecords[objectBuildIndex];
        GameObject* object = outScene.createObject(readStringFromTable(workspace, record.nameOffset));
#else
    for (size_t objectBuildIndex = 0; objectBuildIndex < objectRecords.size(); ++objectBuildIndex) {
        const ObjectRecord& record = objectRecords[objectBuildIndex];
        GameObject* object = outScene.createObject(readStringFromTable(stringTable, record.nameOffset));
#endif
        if (!object) {
            logMessage(NUT_LOG_LITERAL("Could not allocate object."));
            return false;
        }
        object->transform.position = math::Vec3(record.position[0], record.position[1], record.position[2]);
        object->transform.rotation = math::Vec3(record.rotation[0], record.rotation[1], record.rotation[2]);
        object->transform.scale = math::Vec3(record.scale[0], record.scale[1], record.scale[2]);

#ifdef ARDUINO
        object->setMeshIndex(record.meshIndex);
#else
        if (record.meshIndex >= 0 && static_cast<size_t>(record.meshIndex) < meshes.size()) {
            object->mesh = meshes[record.meshIndex];
        }
#endif

#ifdef ARDUINO
        workspace.objects[objectBuildIndex] = object;
        workspace.objectCount = static_cast<uint16_t>(objectBuildIndex + 1);
#else
        objects.push_back(object);
#endif
    }
    NUT_TRACE_LOAD("loader-core: objects built");

#ifdef ARDUINO
    // Decode compact script instances and attach them by object index.
    for (uint16_t objectIndex = 0; objectIndex < workspace.objectRecordCount; ++objectIndex) {
        const ObjectRecord& record = workspace.objectRecords[objectIndex];
#else
    for (size_t objectIndex = 0; objectIndex < objectRecords.size(); ++objectIndex) {
        const ObjectRecord& record = objectRecords[objectIndex];
#endif
        for (uint16_t i = 0; i < record.scriptCount; ++i) {
            const size_t scriptIndex = static_cast<size_t>(record.firstScriptIndex + i);
#ifdef ARDUINO
            if (scriptIndex >= workspace.scriptRecordCount) {
#else
            if (scriptIndex >= scriptRecords.size()) {
#endif
                logMessage(NUT_LOG_LITERAL("Script record index is out of range."));
                return false;
            }

#ifdef ARDUINO
            const ScriptRecord& scriptRecord = workspace.scriptRecords[scriptIndex];
#else
            const ScriptRecord& scriptRecord = scriptRecords[scriptIndex];
#endif
            if (scriptConfigOffset + scriptRecord.configOffset + scriptRecord.configSize > size) {
                logMessage(NUT_LOG_LITERAL("Script config range is out of bounds."));
                return false;
            }
            CompiledScriptInstance instance;
            if (!decodeScriptConfig(
                    data + scriptConfigOffset,
                    header.scriptConfigSize,
                    scriptRecord,
                    static_cast<uint16_t>(objectIndex),
                    instance
                )) {
                logMessageValue(NUT_LOG_LITERAL("Invalid script config for id: "), scriptRecord.scriptId);
                return false;
            }

            if (!outScene.addScriptInstance(instance)) {
                logMessage(NUT_LOG_LITERAL("Could not store compiled script instance."));
                return false;
            }
        }
    }
    NUT_TRACE_LOAD("loader-core: scripts attached");

#ifdef ARDUINO
    // Rebuild the scene graph hierarchy after every object exists.
    for (uint16_t objectIndex = 0; objectIndex < workspace.objectRecordCount; ++objectIndex) {
        const ObjectRecord& record = workspace.objectRecords[objectIndex];
        if (record.parentIndex >= 0) {
            if (static_cast<uint16_t>(record.parentIndex) >= workspace.objectCount) {
                logMessage(NUT_LOG_LITERAL("Parent index is out of range."));
                return false;
            }

            workspace.objects[record.parentIndex]->addChild(workspace.objects[objectIndex]);
        } else {
            outScene.addObject(workspace.objects[objectIndex]);
        }
    }
#else
    for (size_t objectIndex = 0; objectIndex < objectRecords.size(); ++objectIndex) {
        const ObjectRecord& record = objectRecords[objectIndex];
        if (record.parentIndex >= 0) {
            if (static_cast<size_t>(record.parentIndex) >= objects.size()) {
                logMessage(NUT_LOG_LITERAL("Parent index is out of range."));
                return false;
            }

            objects[record.parentIndex]->addChild(objects[objectIndex]);
        } else {
            outScene.addObject(objects[objectIndex]);
        }
    }
#endif
    NUT_TRACE_LOAD("loader-core: hierarchy built");

    return true;
}

} // namespace nut
