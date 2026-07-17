#include "ScenePipeline.h"

#include "../../assets/scripts/ScriptCatalog.h"
#include "../../src/core/ObjLoader.h"
#include "../../src/core/CompiledScriptInstance.h"

#include <cstdint>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace nut::tooling {
namespace {

struct CompiledObject {
    std::string name;
    int parentIndex = -1;
    int meshIndex = -1;
    bool enabled = true;
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
    std::uint8_t enabled = 1;
    float position[3] {0.0f, 0.0f, 0.0f};
    float rotation[3] {0.0f, 0.0f, 0.0f};
    float scale[3] {1.0f, 1.0f, 1.0f};
    std::uint16_t firstScriptIndex = 0;
    std::uint16_t scriptCount = 0;
};

struct SceneBuildState {
    std::vector<CompiledObject> compiledObjects;
    std::vector<std::string> meshPaths;
    std::map<std::string, int> meshIndices;
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

bool fileExists(const fs::path& path) {
    std::error_code error;
    return fs::exists(path, error) && fs::is_regular_file(path, error);
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

fs::path resolveReferencedPath(const fs::path& scenePath, const std::string& referencedPath) {
    const fs::path candidatePath = fs::path(referencedPath);
    if (candidatePath.is_absolute() && fileExists(candidatePath)) {
        return candidatePath;
    }

    if (fileExists(candidatePath)) {
        return candidatePath;
    }

    fs::path base = scenePath.parent_path();
    while (!base.empty()) {
        const fs::path resolved = base / candidatePath;
        if (fileExists(resolved)) {
            return resolved;
        }

        const fs::path parent = base.parent_path();
        if (parent == base) {
            break;
        }
        base = parent;
    }

    return candidatePath;
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

bool flattenObjectTree(
    const nut::Json& objectJson,
    int parentIndex,
    int depth,
    SceneBuildState& state,
    SceneRequirements& requirements,
    std::string& error
) {
    if (depth > 1) {
        error = "Nano scenes support only root objects and one child level.";
        return false;
    }

    CompiledObject object;
    object.name = objectJson.get("name").asString("GameObject");
    object.parentIndex = parentIndex;
    object.enabled = objectJson.get("enabled").asBool(true);
    object.position = readVec3(objectJson.get("position"), {0.0, 0.0, 0.0});
    object.rotation = readVec3(objectJson.get("rotation"), {0.0, 0.0, 0.0});
    object.scale = readVec3(objectJson.get("scale"), {1.0, 1.0, 1.0});
    object.meshIndex = meshIndexForPath(objectJson.get("mesh").asString(""), state.meshIndices, state.meshPaths);

    const nut::Json& scripts = objectJson.get("scripts");
    if (scripts.isArray()) {
        object.scripts = scripts.arrayValue;
    }

    const nut::Json& children = objectJson.get("children");
    const std::uint16_t childCount = children.isArray() ? static_cast<std::uint16_t>(children.size()) : 0;
    const std::uint16_t scriptCount = static_cast<std::uint16_t>(object.scripts.size());

    if (childCount > requirements.maxChildrenPerObject) {
        requirements.maxChildrenPerObject = childCount;
    }
    if (scriptCount > requirements.maxScriptsPerObject) {
        requirements.maxScriptsPerObject = scriptCount;
    }
    requirements.scripts = static_cast<std::uint16_t>(requirements.scripts + scriptCount);

    state.compiledObjects.push_back(object);
    requirements.objects = static_cast<std::uint16_t>(state.compiledObjects.size());
    const int objectIndex = static_cast<int>(state.compiledObjects.size()) - 1;

    if (!children.isArray()) {
        return true;
    }

    for (size_t i = 0; i < children.size(); ++i) {
        if (!flattenObjectTree(children.at(i), objectIndex, depth + 1, state, requirements, error)) {
            return false;
        }
    }

    return true;
}

std::vector<std::uint8_t> buildStringTable(std::map<std::string, int>& stringOffsets) {
    // Pack every distinct object name into one byte array, separated by '\0'.
    // Example: {"Cube", "Sphere"} becomes:
    //   ['C','u','b','e',0,'S','p','h','e','r','e',0]
    // The map value is replaced with the name's starting byte offset, so object
    // records store small integers (Cube=0, Sphere=5) instead of whole strings.
    // std::map also deduplicates equal names before this function is called.
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
    case nut::game::SpinScript::kScriptId: {
        const std::vector<double> rotationSpeed = readVec3(script.get("rotationSpeed"), {0.0, 0.0, 0.0});
        writeF32(out, static_cast<float>(rotationSpeed[0]));
        writeF32(out, static_cast<float>(rotationSpeed[1]));
        writeF32(out, static_cast<float>(rotationSpeed[2]));
        return true;
    }
    case nut::game::AutoTranslateScript::kScriptId: {
        const std::vector<double> unitsPerSecond = readVec3(script.get("unitsPerSecond"), {0.0, 0.0, 0.0});
        writeF32(out, static_cast<float>(unitsPerSecond[0]));
        writeF32(out, static_cast<float>(unitsPerSecond[1]));
        writeF32(out, static_cast<float>(unitsPerSecond[2]));
        return true;
    }
    case nut::game::AutoTranslateWrapScript::kScriptId: {
        float axis = 2.0f;
        const nut::Json& axisValue = script.get("axis");
        if (axisValue.isString()) {
            const std::string axisName = axisValue.asString("z");
            if (axisName == "x" || axisName == "X") {
                axis = 0.0f;
            } else if (axisName == "y" || axisName == "Y") {
                axis = 1.0f;
            }
        } else {
            axis = static_cast<float>(axisValue.asNumber(2.0));
        }

        const std::vector<double> unitsPerSecond = readVec3(script.get("unitsPerSecond"), {0.0, 0.0, 0.0});
        float speed = static_cast<float>(unitsPerSecond[2]);
        if (axis <= 0.5f) {
            speed = static_cast<float>(unitsPerSecond[0]);
        } else if (axis <= 1.5f) {
            speed = static_cast<float>(unitsPerSecond[1]);
        }

        const float minValue = static_cast<float>(script.get("minValue").asNumber(-8.0));
        const float maxValue = static_cast<float>(script.get("maxValue").asNumber(8.0));
        const int16_t packedMin = static_cast<int16_t>(minValue * 100.0f);
        const int16_t packedMax = static_cast<int16_t>(maxValue * 100.0f);

        writeU8(out, static_cast<std::uint8_t>(axis));
        writeF32(out, speed);
        writeI16(out, packedMin);
        writeI16(out, packedMax);
        return true;
    }
    case nut::game::GameControllerScript::kScriptId:
        return true;
    case nut::game::PlayerMoveScript::kScriptId: {
        const std::vector<double> unitsPerSecond = readVec3(script.get("unitsPerSecond"), {1.0, 1.0, 0.0});
        writeF32(out, static_cast<float>(unitsPerSecond[0]));
        writeF32(out, static_cast<float>(unitsPerSecond[1]));
        writeF32(out, static_cast<float>(unitsPerSecond[2]));
        return true;
    }
    case nut::game::ClampPositionScript::kScriptId: {
        float axis = 0.0f;
        const nut::Json& axisValue = script.get("axis");
        if (axisValue.isString()) {
            const std::string axisName = axisValue.asString("x");
            if (axisName == "y" || axisName == "Y") {
                axis = 1.0f;
            } else if (axisName == "z" || axisName == "Z") {
                axis = 2.0f;
            }
        } else {
            axis = static_cast<float>(axisValue.asNumber(0.0));
        }
        writeF32(out, axis);
        writeF32(out, static_cast<float>(script.get("minValue").asNumber(-4.0)));
        writeF32(out, static_cast<float>(script.get("maxValue").asNumber(4.0)));
        return true;
    }
    case nut::game::PulseScaleScript::kScriptId:
        writeF32(out, static_cast<float>(script.get("speed").asNumber(2.0)));
        writeF32(out, static_cast<float>(script.get("minScale").asNumber(0.8)));
        writeF32(out, static_cast<float>(script.get("maxScale").asNumber(1.2)));
        return true;
    case nut::game::TunnelRunScript::kScriptId:
        writeF32(out, static_cast<float>(script.get("baseSpeed").asNumber(2.4)));
        writeF32(out, static_cast<float>(script.get("speedStep").asNumber(0.12)));
        writeF32(out, static_cast<float>(script.get("collisionRadius").asNumber(1.25)));
        return true;
    case nut::game::WrapPositionScript::kScriptId: {
        float axis = 2.0f;
        const nut::Json& axisValue = script.get("axis");
        if (axisValue.isString()) {
            const std::string axisName = axisValue.asString("z");
            if (axisName == "x" || axisName == "X") {
                axis = 0.0f;
            } else if (axisName == "y" || axisName == "Y") {
                axis = 1.0f;
            }
        } else {
            axis = static_cast<float>(axisValue.asNumber(2.0));
        }
        writeF32(out, axis);
        writeF32(out, static_cast<float>(script.get("minValue").asNumber(-8.0)));
        writeF32(out, static_cast<float>(script.get("maxValue").asNumber(8.0)));
        return true;
    }
    default:
        return false;
    }
}

bool loadSceneBuildState(const nut::Json& root, SceneBuildState& outState, SceneAnalysis& outAnalysis, std::string& error) {
    if (!root.isObject()) {
        error = "Scene root must be a JSON object.";
        return false;
    }

    const nut::Json& objects = root.get("objects");
    if (!objects.isArray()) {
        error = "Scene file must contain an objects array.";
        return false;
    }

    outState = SceneBuildState();
    outAnalysis = SceneAnalysis();
    outAnalysis.sceneName = root.get("name").asString("Unnamed");
    outAnalysis.requirements.rootObjects = static_cast<std::uint16_t>(objects.size());

    for (size_t i = 0; i < objects.size(); ++i) {
        if (!flattenObjectTree(objects.at(i), -1, 0, outState, outAnalysis.requirements, error)) {
            return false;
        }
    }

    outAnalysis.requirements.meshes = static_cast<std::uint16_t>(outState.meshPaths.size());

    std::map<std::string, int> stringOffsets;
    for (const CompiledObject& object : outState.compiledObjects) {
        stringOffsets.emplace(object.name, -1);
    }
    const std::vector<std::uint8_t> stringTable = buildStringTable(stringOffsets);
    outAnalysis.requirements.stringTableBytes = static_cast<std::uint16_t>(stringTable.size());
    return true;
}

} // namespace

bool readSceneFile(const fs::path& inputPath, nut::Json& outRoot, std::string& outError) {
    std::string text;
    if (!readTextFile(inputPath, text)) {
        outError = "Could not read input scene: " + inputPath.string();
        return false;
    }

    if (!nut::Json::parse(text, outRoot, &outError)) {
        outError = "Could not parse input scene: " + outError;
        return false;
    }

    if (!outRoot.isObject()) {
        outError = "Scene root must be a JSON object.";
        return false;
    }

    return true;
}

bool analyzeScene(const fs::path& inputPath, const nut::Json& root, SceneAnalysis& outAnalysis, std::string& outError) {
    SceneBuildState state;
    if (!loadSceneBuildState(root, state, outAnalysis, outError)) {
        return false;
    }

    for (const std::string& meshPath : state.meshPaths) {
        nut::Mesh mesh;
        const fs::path resolvedMeshPath = resolveReferencedPath(inputPath, meshPath);
        if (!nut::ObjLoader::load(resolvedMeshPath.string(), mesh)) {
            outError = "Could not load mesh referenced by scene: " + meshPath;
            return false;
        }

        const std::uint16_t vertexCount = static_cast<std::uint16_t>(mesh.vertices.size());
        const std::uint16_t edgeCount = static_cast<std::uint16_t>(mesh.edges.size());
        if (vertexCount > outAnalysis.requirements.maxVerticesPerMesh) {
            outAnalysis.requirements.maxVerticesPerMesh = vertexCount;
        }
        if (edgeCount > outAnalysis.requirements.maxEdgesPerMesh) {
            outAnalysis.requirements.maxEdgesPerMesh = edgeCount;
        }

        outAnalysis.meshGeometryBytes += mesh.vertices.size() * sizeof(float) * 3;
        outAnalysis.meshGeometryBytes += mesh.edges.size() * sizeof(std::uint16_t) * 2;
    }

    for (const CompiledObject& object : state.compiledObjects) {
        for (const nut::Json& script : object.scripts) {
            const std::uint16_t scriptId = nut::game::scriptIdFromName(script.get("type").asString(""));
            if (scriptId == 0) {
                outError = "Unknown script type referenced by scene: " + script.get("type").asString("");
                return false;
            }

            std::vector<std::uint8_t> scriptConfigBytes;
            if (!encodeBinaryScriptConfig(script, scriptId, scriptConfigBytes)) {
                outError = "Could not encode binary config for script id: " + std::to_string(scriptId);
                return false;
            }

            outAnalysis.scriptConfigBytes += scriptConfigBytes.size();
        }
    }

    return true;
}

bool compileScene(const fs::path& inputPath, const nut::Json& root, CompiledSceneData& outCompiled, std::string& outError) {
    SceneBuildState state;
    SceneAnalysis analysis;
    if (!loadSceneBuildState(root, state, analysis, outError)) {
        return false;
    }

    std::map<std::string, int> stringOffsets;
    for (const CompiledObject& object : state.compiledObjects) {
        stringOffsets.emplace(object.name, -1);
    }
    const std::vector<std::uint8_t> stringTable = buildStringTable(stringOffsets);

    std::vector<MeshRecord> meshRecords;
    std::vector<std::uint8_t> meshGeometryBytes;

    for (const std::string& meshPath : state.meshPaths) {
        nut::Mesh mesh;
        const fs::path resolvedMeshPath = resolveReferencedPath(inputPath, meshPath);
        if (!nut::ObjLoader::load(resolvedMeshPath.string(), mesh)) {
            outError = "Could not load mesh referenced by scene: " + meshPath;
            return false;
        }

        MeshRecord record;
        record.vertexCount = static_cast<std::uint16_t>(mesh.vertices.size());
        record.edgeCount = static_cast<std::uint16_t>(mesh.edges.size());
        meshRecords.push_back(record);

        if (record.vertexCount > analysis.requirements.maxVerticesPerMesh) {
            analysis.requirements.maxVerticesPerMesh = record.vertexCount;
        }
        if (record.edgeCount > analysis.requirements.maxEdgesPerMesh) {
            analysis.requirements.maxEdgesPerMesh = record.edgeCount;
        }

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
    analysis.meshGeometryBytes = meshGeometryBytes.size();

    std::vector<ScriptRecord> scriptRecords;
    std::vector<std::uint8_t> scriptConfigBytes;
    std::vector<ObjectRecord> objectRecords;

    for (const CompiledObject& object : state.compiledObjects) {
        ObjectRecord record;
        record.nameOffset = stringOffsets.at(object.name);
        record.parentIndex = object.parentIndex;
        record.meshIndex = object.meshIndex;
        record.enabled = object.enabled ? 1 : 0;
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
            const std::uint16_t scriptId = nut::game::scriptIdFromName(script.get("type").asString(""));
            if (scriptId == 0) {
                outError = "Unknown script type referenced by scene: " + script.get("type").asString("");
                return false;
            }

            scriptRecord.scriptId = scriptId;
            scriptRecord.configOffset = static_cast<std::uint16_t>(scriptConfigBytes.size());
            const std::size_t configStart = scriptConfigBytes.size();
            if (!encodeBinaryScriptConfig(script, scriptRecord.scriptId, scriptConfigBytes)) {
                outError = "Could not encode binary config for script id: " + std::to_string(scriptRecord.scriptId);
                return false;
            }
            scriptRecord.configSize = static_cast<std::uint16_t>(scriptConfigBytes.size() - configStart);
            scriptRecords.push_back(scriptRecord);
        }

        objectRecords.push_back(record);
    }
    analysis.scriptConfigBytes = scriptConfigBytes.size();

    const nut::Json& camera = root.get("camera");
    const std::vector<double> cameraPosition = readVec3(camera.get("position"), {0.0, 0.0, -5.0});
    const std::vector<double> cameraRotation = readVec3(camera.get("rotation"), {0.0, 0.0, 0.0});
    const float cameraFov = static_cast<float>(camera.get("fov").asNumber(60.0));

    std::vector<std::uint8_t> blob;
    blob.push_back('N');
    blob.push_back('U');
    blob.push_back('T');
    blob.push_back('0');
    writeU16(blob, 3);
    writeU16(blob, static_cast<std::uint16_t>(stringTable.size()));
    writeU16(blob, static_cast<std::uint16_t>(state.meshPaths.size()));
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
        writeU8(blob, record.enabled);
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

    outCompiled.analysis = analysis;
    outCompiled.blob = std::move(blob);
    return true;
}

bool writeSceneHeader(
    const fs::path& inputPath,
    const fs::path& outputPath,
    const CompiledSceneData& compiled,
    std::string& outError
) {
    fs::create_directories(outputPath.parent_path());

    std::ofstream out(outputPath);
    if (!out.is_open()) {
        outError = "Could not write output file: " + outputPath.string();
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
    out << "// Scene name: " << compiled.analysis.sceneName << "\n";
    out << "// Objects: " << compiled.analysis.requirements.objects
        << ", Meshes: " << compiled.analysis.requirements.meshes
        << ", Scripts: " << compiled.analysis.requirements.scripts << "\n";
    out << "// Mesh bytes: " << compiled.analysis.meshGeometryBytes
        << ", Script config bytes: " << compiled.analysis.scriptConfigBytes << "\n\n";
    out << "static const uint8_t " << arrayName << "[] NUT_SCENE_STORAGE = {\n";

    out << std::hex << std::setfill('0');
    for (size_t i = 0; i < compiled.blob.size(); ++i) {
        if (i % 12 == 0) {
            out << "    ";
        }

        out << "0x" << std::setw(2) << static_cast<int>(compiled.blob[i]);
        if (i + 1 < compiled.blob.size()) {
            out << ", ";
        }

        if (i % 12 == 11 || i + 1 == compiled.blob.size()) {
            out << "\n";
        }
    }

    out << "};\n\n";
    out << "static const size_t " << sizeName << " = sizeof(" << arrayName << ");\n";
    return true;
}

bool writeSceneLimitsHeader(
    const fs::path& inputPath,
    const fs::path& outputPath,
    const SceneAnalysis& analysis,
    std::string& outError
) {
    fs::create_directories(outputPath.parent_path());

    std::ofstream out(outputPath);
    if (!out.is_open()) {
        outError = "Could not write output file: " + outputPath.string();
        return false;
    }

    const SceneRequirements& r = analysis.requirements;

    out << "#pragma once\n\n";
    out << "// Generated from: " << inputPath.generic_string() << "\n";
    out << "// Scene name: " << analysis.sceneName << "\n";
    out << "// Derived Nano runtime limits for the active scene.\n\n";
    out << "#define NUT_CFG_MAX_ROOT_OBJECTS " << r.rootObjects << "\n";
    out << "#define NUT_CFG_MAX_CHILDREN_PER_OBJECT " << r.maxChildrenPerObject << "\n";
    out << "#define NUT_CFG_MAX_SCRIPTS_PER_OBJECT " << r.maxScriptsPerObject << "\n";
    out << "#define NUT_CFG_MAX_OWNED_OBJECTS " << r.objects << "\n";
    out << "#define NUT_CFG_MAX_OWNED_MESHES " << r.meshes << "\n";
    out << "#define NUT_CFG_MAX_OWNED_SCRIPTS " << r.scripts << "\n";
    out << "#define NUT_CFG_MAX_MESHES " << r.meshes << "\n";
    out << "#define NUT_CFG_MAX_OBJECTS " << r.objects << "\n";
    out << "#define NUT_CFG_MAX_SCRIPT_RECORDS " << r.scripts << "\n";
    out << "#define NUT_CFG_MAX_VERTICES_PER_MESH " << r.maxVerticesPerMesh << "\n";
    out << "#define NUT_CFG_MAX_EDGES_PER_MESH " << r.maxEdgesPerMesh << "\n";
    out << "#define NUT_CFG_MAX_STRING_TABLE_BYTES " << r.stringTableBytes << "\n";
    return true;
}

} // namespace nut::tooling
