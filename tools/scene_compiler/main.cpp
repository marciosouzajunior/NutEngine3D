#include "../../src/scene/Json.h"
#include "../../src/core/ObjLoader.h"
#include "../../assets/scripts/ScriptCatalog.h"

#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace {

struct CompiledObject {
    std::string name;
    int parentIndex = -1;
    int meshIndex = -1;
    std::vector<double> position {0.0, 0.0, 0.0};
    std::vector<double> rotation {0.0, 0.0, 0.0};
    std::vector<double> scale {1.0, 1.0, 1.0};
    std::vector<nut::Json> scripts;
};

struct ScriptRecord {
    std::uint16_t scriptId = 0;
    std::uint16_t configOffset = 0;
    std::uint16_t configSize = 0;
};

struct MeshRecord {
    std::uint16_t vertexCount = 0;
    std::uint16_t edgeCount = 0;
};

struct ObjectRecord {
    int nameOffset = 0;
    int parentIndex = -1;
    int meshIndex = -1;
    float position[3] {0.0f, 0.0f, 0.0f};
    float rotation[3] {0.0f, 0.0f, 0.0f};
    float scale[3] {1.0f, 1.0f, 1.0f};
    std::uint16_t firstScriptIndex = 0;
    std::uint16_t scriptCount = 0;
};

bool readTextFile(const fs::path& path, std::string& outText) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return false;
    }

    std::ostringstream stream;
    stream << file.rdbuf();
    outText = stream.str();
    return true;
}

std::string sanitizeSymbol(const std::string& text) {
    std::string symbol;
    symbol.reserve(text.size() + 8);

    for (char c : text) {
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')) {
            symbol.push_back(c);
        } else {
            symbol.push_back('_');
        }
    }

    if (symbol.empty() || (symbol[0] >= '0' && symbol[0] <= '9')) {
        symbol.insert(symbol.begin(), '_');
    }

    return symbol;
}

std::vector<double> readVec3(const nut::Json& value, const std::vector<double>& defaultValue) {
    if (!value.isArray() || value.size() < 3) {
        return defaultValue;
    }

    return {
        value.at(0).asNumber(defaultValue[0]),
        value.at(1).asNumber(defaultValue[1]),
        value.at(2).asNumber(defaultValue[2])
    };
}

int meshIndexForPath(
    const std::string& meshPath,
    std::map<std::string, int>& meshIndices,
    std::vector<std::string>& meshPaths
) {
    if (meshPath.empty()) {
        return -1;
    }

    auto it = meshIndices.find(meshPath);
    if (it != meshIndices.end()) {
        return it->second;
    }

    const int index = static_cast<int>(meshPaths.size());
    meshIndices[meshPath] = index;
    meshPaths.push_back(meshPath);
    return index;
}

void flattenObjectTree(
    const nut::Json& objectJson,
    int parentIndex,
    std::map<std::string, int>& meshIndices,
    std::vector<std::string>& meshPaths,
    std::vector<CompiledObject>& compiledObjects
) {
    CompiledObject object;
    object.name = objectJson.get("name").asString("GameObject");
    object.parentIndex = parentIndex;
    object.position = readVec3(objectJson.get("position"), {0.0, 0.0, 0.0});
    object.rotation = readVec3(objectJson.get("rotation"), {0.0, 0.0, 0.0});
    object.scale = readVec3(objectJson.get("scale"), {1.0, 1.0, 1.0});
    object.meshIndex = meshIndexForPath(objectJson.get("mesh").asString(""), meshIndices, meshPaths);

    const nut::Json& scripts = objectJson.get("scripts");
    if (scripts.isArray()) {
        object.scripts = scripts.arrayValue;
    }

    compiledObjects.push_back(object);
    const int objectIndex = static_cast<int>(compiledObjects.size()) - 1;

    const nut::Json& children = objectJson.get("children");
    if (!children.isArray()) {
        return;
    }

    for (size_t i = 0; i < children.size(); ++i) {
        flattenObjectTree(children.at(i), objectIndex, meshIndices, meshPaths, compiledObjects);
    }
}

std::vector<std::uint8_t> buildStringTable(std::map<std::string, int>& stringOffsets) {
    std::vector<std::uint8_t> bytes;

    for (auto& entry : stringOffsets) {
        entry.second = static_cast<int>(bytes.size());
        bytes.insert(bytes.end(), entry.first.begin(), entry.first.end());
        bytes.push_back(0);
    }

    return bytes;
}

void writeU8(std::vector<std::uint8_t>& out, std::uint8_t value) {
    out.push_back(value);
}

void writeU16(std::vector<std::uint8_t>& out, std::uint16_t value) {
    out.push_back(static_cast<std::uint8_t>(value & 0xFF));
    out.push_back(static_cast<std::uint8_t>((value >> 8) & 0xFF));
}

void writeI16(std::vector<std::uint8_t>& out, std::int16_t value) {
    writeU16(out, static_cast<std::uint16_t>(value));
}

void writeF32(std::vector<std::uint8_t>& out, float value) {
    std::uint32_t bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));
    out.push_back(static_cast<std::uint8_t>(bits & 0xFF));
    out.push_back(static_cast<std::uint8_t>((bits >> 8) & 0xFF));
    out.push_back(static_cast<std::uint8_t>((bits >> 16) & 0xFF));
    out.push_back(static_cast<std::uint8_t>((bits >> 24) & 0xFF));
}

bool encodeBinaryScriptConfig(
    const nut::Json& script,
    std::uint16_t scriptId,
    std::vector<std::uint8_t>& out
) {
    switch (scriptId) {
    case SpinScript::kScriptId: {
        const std::vector<double> rotationSpeed = readVec3(script.get("rotationSpeed"), {0.0, 0.0, 0.0});
        writeF32(out, static_cast<float>(rotationSpeed[0]));
        writeF32(out, static_cast<float>(rotationSpeed[1]));
        writeF32(out, static_cast<float>(rotationSpeed[2]));
        return true;
    }
    case DummyScript::kScriptId:
        writeU8(out, script.get("enabled").asBool(true) ? 1 : 0);
        return true;
    default:
        return false;
    }
}

bool writeSceneHeader(
    const fs::path& inputPath,
    const fs::path& outputPath,
    const nut::Json& root
) {
    const nut::Json& objects = root.get("objects");
    if (!objects.isArray()) {
        std::cerr << "Scene file must contain an objects array.\n";
        return false;
    }

    std::vector<CompiledObject> compiledObjects;
    std::vector<std::string> meshPaths;
    std::map<std::string, int> meshIndices;

    for (size_t i = 0; i < objects.size(); ++i) {
        flattenObjectTree(objects.at(i), -1, meshIndices, meshPaths, compiledObjects);
    }

    std::map<std::string, int> stringOffsets;

    for (const CompiledObject& object : compiledObjects) {
        stringOffsets.emplace(object.name, -1);
    }

    const std::vector<std::uint8_t> stringTable = buildStringTable(stringOffsets);

    std::vector<MeshRecord> meshRecords;
    std::vector<std::uint8_t> meshGeometryBytes;

    for (const std::string& meshPath : meshPaths) {
        nut::Mesh mesh;
        if (!nut::ObjLoader::load(meshPath, mesh)) {
            std::cerr << "Could not load mesh referenced by scene: " << meshPath << "\n";
            return false;
        }

        MeshRecord record;
        record.vertexCount = static_cast<std::uint16_t>(mesh.vertices.size());
        record.edgeCount = static_cast<std::uint16_t>(mesh.edges.size());
        meshRecords.push_back(record);

        for (const auto& vertex : mesh.vertices) {
            writeF32(meshGeometryBytes, vertex.x);
            writeF32(meshGeometryBytes, vertex.y);
            writeF32(meshGeometryBytes, vertex.z);
        }

        for (const auto& edge : mesh.edges) {
            writeU16(meshGeometryBytes, static_cast<std::uint16_t>(edge.a));
            writeU16(meshGeometryBytes, static_cast<std::uint16_t>(edge.b));
        }
    }

    std::vector<ScriptRecord> scriptRecords;
    std::vector<std::uint8_t> scriptConfigBytes;
    std::vector<ObjectRecord> objectRecords;

    for (const CompiledObject& object : compiledObjects) {
        ObjectRecord record;
        record.nameOffset = stringOffsets.at(object.name);
        record.parentIndex = object.parentIndex;
        record.meshIndex = object.meshIndex;
        record.position[0] = static_cast<float>(object.position[0]);
        record.position[1] = static_cast<float>(object.position[1]);
        record.position[2] = static_cast<float>(object.position[2]);
        record.rotation[0] = static_cast<float>(object.rotation[0]);
        record.rotation[1] = static_cast<float>(object.rotation[1]);
        record.rotation[2] = static_cast<float>(object.rotation[2]);
        record.scale[0] = static_cast<float>(object.scale[0]);
        record.scale[1] = static_cast<float>(object.scale[1]);
        record.scale[2] = static_cast<float>(object.scale[2]);
        record.firstScriptIndex = static_cast<std::uint16_t>(scriptRecords.size());
        record.scriptCount = static_cast<std::uint16_t>(object.scripts.size());

        for (const nut::Json& script : object.scripts) {
            ScriptRecord scriptRecord;
            const uint16_t scriptId = nut::scripts::scriptIdFromName(script.get("type").asString(""));
            if (scriptId == 0) {
                std::cerr << "Unknown script type referenced by scene: "
                          << script.get("type").asString("") << "\n";
                return false;
            }

            scriptRecord.scriptId = static_cast<std::uint16_t>(scriptId);
            scriptRecord.configOffset = static_cast<std::uint16_t>(scriptConfigBytes.size());

            const std::size_t configStart = scriptConfigBytes.size();
            if (!encodeBinaryScriptConfig(script, scriptRecord.scriptId, scriptConfigBytes)) {
                std::cerr << "Could not encode binary config for script id: "
                          << scriptRecord.scriptId << "\n";
                return false;
            }
            scriptRecord.configSize = static_cast<std::uint16_t>(scriptConfigBytes.size() - configStart);

            scriptRecords.push_back(scriptRecord);
        }

        objectRecords.push_back(record);
    }

    const nut::Json& camera = root.get("camera");
    const std::vector<double> cameraPosition = readVec3(camera.get("position"), {0.0, 0.0, -5.0});
    const std::vector<double> cameraRotation = readVec3(camera.get("rotation"), {0.0, 0.0, 0.0});
    const float cameraFov = static_cast<float>(camera.get("fov").asNumber(60.0));

    std::vector<std::uint8_t> blob;

    blob.push_back('N');
    blob.push_back('U');
    blob.push_back('T');
    blob.push_back('0');
    writeU16(blob, 2);
    writeU16(blob, static_cast<std::uint16_t>(stringTable.size()));
    writeU16(blob, static_cast<std::uint16_t>(meshPaths.size()));
    writeU16(blob, static_cast<std::uint16_t>(objectRecords.size()));
    writeU16(blob, static_cast<std::uint16_t>(scriptRecords.size()));
    writeU16(blob, static_cast<std::uint16_t>(meshGeometryBytes.size()));
    writeU16(blob, static_cast<std::uint16_t>(scriptConfigBytes.size()));

    writeF32(blob, static_cast<float>(cameraPosition[0]));
    writeF32(blob, static_cast<float>(cameraPosition[1]));
    writeF32(blob, static_cast<float>(cameraPosition[2]));
    writeF32(blob, static_cast<float>(cameraRotation[0]));
    writeF32(blob, static_cast<float>(cameraRotation[1]));
    writeF32(blob, static_cast<float>(cameraRotation[2]));
    writeF32(blob, cameraFov);

    blob.insert(blob.end(), stringTable.begin(), stringTable.end());

    for (const MeshRecord& record : meshRecords) {
        writeU16(blob, record.vertexCount);
        writeU16(blob, record.edgeCount);
    }

    blob.insert(blob.end(), meshGeometryBytes.begin(), meshGeometryBytes.end());

    for (const ObjectRecord& record : objectRecords) {
        writeU16(blob, static_cast<std::uint16_t>(record.nameOffset));
        writeI16(blob, static_cast<std::int16_t>(record.parentIndex));
        writeI16(blob, static_cast<std::int16_t>(record.meshIndex));
        writeU16(blob, record.firstScriptIndex);
        writeU16(blob, record.scriptCount);
        for (float value : record.position) {
            writeF32(blob, value);
        }
        for (float value : record.rotation) {
            writeF32(blob, value);
        }
        for (float value : record.scale) {
            writeF32(blob, value);
        }
    }

    for (const ScriptRecord& record : scriptRecords) {
        writeU16(blob, record.scriptId);
        writeU16(blob, record.configOffset);
        writeU16(blob, record.configSize);
    }

    blob.insert(blob.end(), scriptConfigBytes.begin(), scriptConfigBytes.end());

    fs::create_directories(outputPath.parent_path());

    std::ofstream out(outputPath);
    if (!out.is_open()) {
        std::cerr << "Could not write output file: " << outputPath << "\n";
        return false;
    }

    const std::string symbolBase = sanitizeSymbol(outputPath.stem().string());
    const std::string arrayName = symbolBase + "_data";
    const std::string sizeName = symbolBase + "_size";

    out << "#pragma once\n\n";
    out << "#include <stddef.h>\n";
    out << "#include <stdint.h>\n";
    out << "#if defined(__AVR__)\n";
    out << "#include <avr/pgmspace.h>\n";
    out << "#define NUT_SCENE_STORAGE PROGMEM\n";
    out << "#else\n";
    out << "#define NUT_SCENE_STORAGE\n";
    out << "#endif\n\n";
    out << "// Generated from: " << inputPath.generic_string() << "\n";
    out << "// Binary format: NutSceneBinaryV0\n";
    out << "// Scene name: " << root.get("name").asString("Unnamed") << "\n";
    out << "// Objects: " << objectRecords.size() << ", Meshes: " << meshPaths.size() << ", Scripts: " << scriptRecords.size() << "\n";
    out << "// Mesh bytes: " << meshGeometryBytes.size() << ", Script config bytes: " << scriptConfigBytes.size() << "\n\n";
    out << "static const uint8_t " << arrayName << "[] NUT_SCENE_STORAGE = {\n";

    out << std::hex << std::setfill('0');
    for (size_t i = 0; i < blob.size(); ++i) {
        if (i % 12 == 0) {
            out << "    ";
        }

        out << "0x" << std::setw(2) << static_cast<int>(blob[i]);
        if (i + 1 < blob.size()) {
            out << ", ";
        }

        if (i % 12 == 11 || i + 1 == blob.size()) {
            out << "\n";
        }
    }

    out << "};\n\n";
    out << "static const size_t " << sizeName << " = sizeof(" << arrayName << ");\n";

    return true;
}

} // namespace

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "Usage: NutSceneCompiler <input.nutscene> <output_scene.h>\n";
        return 1;
    }

    const fs::path inputPath = fs::path(argv[1]);
    const fs::path outputPath = fs::path(argv[2]);

    std::string text;
    if (!readTextFile(inputPath, text)) {
        std::cerr << "Could not read input scene: " << inputPath << "\n";
        return 1;
    }

    nut::Json root;
    std::string error;
    if (!nut::Json::parse(text, root, &error)) {
        std::cerr << "Could not parse input scene: " << error << "\n";
        return 1;
    }

    if (!root.isObject()) {
        std::cerr << "Scene root must be a JSON object.\n";
        return 1;
    }

    if (!writeSceneHeader(inputPath, outputPath, root)) {
        return 1;
    }

    std::cout << "Wrote compiled scene header: " << outputPath << "\n";
    return 0;
}
