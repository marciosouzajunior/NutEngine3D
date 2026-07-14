#include "EditorSceneCatalog.h"

#include <algorithm>
#include <cctype>
#include <filesystem>

namespace {

std::string toAbsoluteProjectPath(const std::string& relativePath) {
    return std::string(NUT_PROJECT_ROOT) + "/" + relativePath;
}

std::string trimSceneCatalogText(const std::string& text) {
    size_t start = 0;
    while (start < text.size() && std::isspace(static_cast<unsigned char>(text[start]))) {
        ++start;
    }

    size_t end = text.size();
    while (end > start && std::isspace(static_cast<unsigned char>(text[end - 1]))) {
        --end;
    }

    return text.substr(start, end - start);
}

}

std::vector<std::pair<std::string, std::string>> availableEditorScenes() {
    namespace fs = std::filesystem;

    std::vector<std::pair<std::string, std::string>> scenes;
    const fs::path scenesDir = fs::path(toAbsoluteProjectPath("assets/scenes"));
    if (!fs::exists(scenesDir) || !fs::is_directory(scenesDir)) {
        return scenes;
    }

    for (const fs::directory_entry& entry : fs::directory_iterator(scenesDir)) {
        if (!entry.is_regular_file() || entry.path().extension() != ".nutscene") {
            continue;
        }

        const std::string filename = entry.path().filename().string();
        std::string label = entry.path().stem().string();
        if (!label.empty()) {
            label[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(label[0])));
        }
        scenes.push_back({label, "assets/scenes/" + filename});
    }

    std::sort(
        scenes.begin(),
        scenes.end(),
        [](const auto& left, const auto& right) {
            return left.second < right.second;
        }
    );
    return scenes;
}

std::string defaultEditorScenePath() {
    const std::vector<std::pair<std::string, std::string>> scenes = availableEditorScenes();
    if (!scenes.empty()) {
        return scenes.front().second;
    }
    return "assets/scenes/demo.nutscene";
}

bool isProtectedScenePath(const std::string& scenePath) {
    return scenePath == "assets/scenes/demo.nutscene";
}

std::string sceneDisplayNameForPath(const std::string& scenePath) {
    for (const auto& entry : availableEditorScenes()) {
        if (scenePath == entry.second) {
            return entry.first;
        }
    }
    return scenePath;
}

std::vector<std::string> availableEditorMeshPaths() {
    namespace fs = std::filesystem;

    std::vector<std::string> meshPaths;
    const fs::path modelsDir = fs::path(toAbsoluteProjectPath("assets/models"));
    if (!fs::exists(modelsDir) || !fs::is_directory(modelsDir)) {
        return meshPaths;
    }

    for (const fs::directory_entry& entry : fs::directory_iterator(modelsDir)) {
        if (!entry.is_regular_file()) {
            continue;
        }
        if (entry.path().extension() != ".obj") {
            continue;
        }
        meshPaths.push_back("assets/models/" + entry.path().filename().string());
    }

    std::sort(meshPaths.begin(), meshPaths.end());
    return meshPaths;
}

bool isKnownEditorScenePath(const std::string& scenePath) {
    for (const auto& entry : availableEditorScenes()) {
        if (scenePath == entry.second) {
            return true;
        }
    }
    return false;
}

bool isValidSceneRelativePath(const std::string& scenePath) {
    if (scenePath.empty() || scenePath.find("..") != std::string::npos) {
        return false;
    }
    if (scenePath.rfind("assets/scenes/", 0) != 0) {
        return false;
    }
    return std::filesystem::path(scenePath).extension() == ".nutscene";
}

std::string normalizeSceneRelativePath(const std::string& rawPath) {
    std::string trimmed = trimSceneCatalogText(rawPath);
    std::replace(trimmed.begin(), trimmed.end(), '\\', '/');
    if (trimmed.empty()) {
        return "";
    }
    if (trimmed.rfind("assets/scenes/", 0) != 0) {
        trimmed = "assets/scenes/" + trimmed;
    }
    if (std::filesystem::path(trimmed).extension() != ".nutscene") {
        trimmed += ".nutscene";
    }
    return trimmed;
}

std::string slugifySceneName(const std::string& sceneName) {
    std::string slug;
    slug.reserve(sceneName.size());
    bool previousUnderscore = false;
    for (char ch : sceneName) {
        const unsigned char uch = static_cast<unsigned char>(ch);
        if (std::isalnum(uch)) {
            slug.push_back(static_cast<char>(std::tolower(uch)));
            previousUnderscore = false;
        } else if (!previousUnderscore) {
            slug.push_back('_');
            previousUnderscore = true;
        }
    }

    while (!slug.empty() && slug.front() == '_') {
        slug.erase(slug.begin());
    }
    while (!slug.empty() && slug.back() == '_') {
        slug.pop_back();
    }

    if (slug.empty()) {
        slug = "new_scene";
    }
    return "assets/scenes/" + slug + ".nutscene";
}
