#pragma once

#include "../../src/scene/Json.h"

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace nut::tooling {

struct SceneRequirements {
    std::uint16_t rootObjects = 0;
    std::uint16_t objects = 0;
    std::uint16_t maxChildrenPerObject = 0;
    std::uint16_t scripts = 0;
    std::uint16_t maxScriptsPerObject = 0;
    std::uint16_t meshes = 0;
    std::uint16_t stringTableBytes = 0;
    std::uint16_t maxVerticesPerMesh = 0;
    std::uint16_t maxEdgesPerMesh = 0;
};

struct SceneAnalysis {
    SceneRequirements requirements;
    std::string sceneName;
    std::size_t meshGeometryBytes = 0;
    std::size_t scriptConfigBytes = 0;
};

struct CompiledSceneData {
    SceneAnalysis analysis;
    std::vector<std::uint8_t> blob;
};

bool readSceneFile(const std::filesystem::path& inputPath, nut::Json& outRoot, std::string& outError);
bool analyzeScene(const std::filesystem::path& inputPath, const nut::Json& root, SceneAnalysis& outAnalysis, std::string& outError);
bool compileScene(const std::filesystem::path& inputPath, const nut::Json& root, CompiledSceneData& outCompiled, std::string& outError);
bool writeSceneHeader(
    const std::filesystem::path& inputPath,
    const std::filesystem::path& outputPath,
    const CompiledSceneData& compiled,
    std::string& outError
);
bool writeSceneLimitsHeader(
    const std::filesystem::path& inputPath,
    const std::filesystem::path& outputPath,
    const SceneAnalysis& analysis,
    std::string& outError
);

} // namespace nut::tooling
