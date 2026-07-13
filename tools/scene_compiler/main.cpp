#include "ScenePipeline.h"

#include <filesystem>
#include <iostream>
#include <string>

namespace fs = std::filesystem;

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "Usage: NutSceneCompiler <input.nutscene> <output_scene.h>\n";
        return 1;
    }

    const fs::path inputPath = fs::path(argv[1]);
    const fs::path outputPath = fs::path(argv[2]);
    const fs::path limitsPath = outputPath.parent_path() / (outputPath.stem().string() + "_limits.h");

    nut::Json root;
    std::string error;
    if (!nut::tooling::readSceneFile(inputPath, root, error)) {
        std::cerr << error << "\n";
        return 1;
    }

    nut::tooling::CompiledSceneData compiled;
    if (!nut::tooling::compileScene(inputPath, root, compiled, error)) {
        std::cerr << error << "\n";
        return 1;
    }

    if (!nut::tooling::writeSceneHeader(inputPath, outputPath, compiled, error)) {
        std::cerr << error << "\n";
        return 1;
    }

    if (!nut::tooling::writeSceneLimitsHeader(inputPath, limitsPath, compiled.analysis, error)) {
        std::cerr << error << "\n";
        return 1;
    }

    std::cout << "Wrote compiled scene header: " << outputPath << "\n";
    std::cout << "Wrote scene limits header: " << limitsPath << "\n";
    return 0;
}
