#include "EditorBuildSettings.h"

#include "EditorSceneCatalog.h"
#include "EditorUtils.h"

#include "../../../src/scene/Json.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <sstream>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif

namespace {

std::string toAbsoluteProjectPath(const std::string& relativePath) {
    return std::string(NUT_PROJECT_ROOT) + "/" + relativePath;
}

float readFloat(const nut::Json& value, float defaultValue) {
    return static_cast<float>(value.asNumber(defaultValue));
}

void readVec3(const nut::Json& value, float outValues[3], const float defaults[3]) {
    outValues[0] = defaults[0];
    outValues[1] = defaults[1];
    outValues[2] = defaults[2];

    if (!value.isArray()) {
        return;
    }

    for (size_t i = 0; i < 3 && i < value.size(); ++i) {
        outValues[i] = static_cast<float>(value.at(i).asNumber(defaults[i]));
    }
}

std::string normalizePathSlashes(const std::string& path) {
    std::string normalized = path;
    std::replace(normalized.begin(), normalized.end(), '\\', '/');
    return normalized;
}

std::string getEnvVarCopy(const char* name) {
    const char* value = std::getenv(name);
    return value != nullptr ? std::string(value) : std::string();
}

std::string joinWindowsPath(const std::string& left, const std::string& right) {
    if (left.empty()) {
        return normalizePathSlashes(right);
    }

    std::string joined = left;
    if (!joined.empty() && joined.back() != '/' && joined.back() != '\\') {
        joined.push_back('/');
    }
    joined += right;
    return normalizePathSlashes(joined);
}

std::string detectPlatformIoExecutable() {
#ifdef _WIN32
    std::vector<std::string> candidates;

    const std::string userProfile = getEnvVarCopy("USERPROFILE");
    if (!userProfile.empty()) {
        candidates.push_back(joinWindowsPath(userProfile, ".platformio/penv/Scripts/platformio.exe"));
    }

    const std::string homeDrive = getEnvVarCopy("HOMEDRIVE");
    const std::string homePath = getEnvVarCopy("HOMEPATH");
    if (!homeDrive.empty() && !homePath.empty()) {
        candidates.push_back(joinWindowsPath(homeDrive + homePath, ".platformio/penv/Scripts/platformio.exe"));
    }

    for (const std::string& candidate : candidates) {
        if (fileExistsOnDisk(candidate)) {
            return normalizePathSlashes(candidate);
        }
    }

    char resolvedPath[MAX_PATH] = {};
    const DWORD resolvedLength = SearchPathA(nullptr, "platformio.exe", nullptr, MAX_PATH, resolvedPath, nullptr);
    if (resolvedLength > 0 && resolvedLength < MAX_PATH) {
        return normalizePathSlashes(resolvedPath);
    }
#endif

    return std::string();
}

std::string defaultUploadPort() {
    std::string platformIoIni;
    if (readTextFile(toAbsoluteProjectPath("targets/nano/platformio.ini"), platformIoIni)) {
        std::istringstream stream(platformIoIni);
        std::string line;
        while (std::getline(stream, line)) {
            const std::string trimmed = trimAsciiWhitespace(line);
            if (trimmed.rfind("upload_port", 0) != 0) {
                continue;
            }

            const size_t equals = trimmed.find('=');
            if (equals == std::string::npos) {
                continue;
            }

            const std::string value = trimAsciiWhitespace(trimmed.substr(equals + 1));
            if (!value.empty()) {
                return value;
            }
        }
    }

    return "COM5";
}

}

bool isValidUploadPort(const std::string& port) {
    if (port.empty()) {
        return false;
    }

    for (char c : port) {
        if ((c >= 'a' && c <= 'z')
            || (c >= 'A' && c <= 'Z')
            || (c >= '0' && c <= '9')
            || c == '_' || c == '-' || c == '.') {
            continue;
        }
        return false;
    }

    return true;
}

std::string buildSettingsConfigPath() {
    return std::string(NUT_PROJECT_ROOT) + "/tools/scene_editor/editor_user_settings.json";
}

EditorBuildSettings makeDefaultBuildSettings() {
    EditorBuildSettings settings;
    settings.platformioPath = detectPlatformIoExecutable();
    settings.uploadPort = defaultUploadPort();
    settings.selectedScenePath = defaultEditorScenePath();
    return settings;
}

bool loadBuildSettings(EditorBuildSettings& settings, std::string& outMessage) {
    settings = makeDefaultBuildSettings();

    const std::string configPath = buildSettingsConfigPath();
    std::string text;
    if (!readTextFile(configPath, text)) {
        if (settings.platformioPath.empty()) {
            outMessage = "> Build settings: PlatformIO was not auto-detected. Configure the executable path in Build.";
        } else {
            outMessage = "> Build settings: using auto-detected PlatformIO path.";
        }
        return false;
    }

    nut::Json root;
    std::string error;
    if (!nut::Json::parse(text, root, &error) || !root.isObject()) {
        outMessage = "> Build settings: local config is invalid. Using detected defaults.";
        return false;
    }

    const std::string configuredPath = root.get("platformioPath").asString("");
    const std::string configuredPort = root.get("uploadPort").asString("");
    const std::string configuredScenePath = root.get("selectedScenePath").asString("");
    const nut::Json& viewport = root.get("viewport");
    const nut::Json& sceneViewports = root.get("sceneViewports");
    if (!configuredPath.empty()) {
        settings.platformioPath = configuredPath;
    }
    if (!configuredPort.empty()) {
        settings.uploadPort = configuredPort;
    }
    if (isKnownEditorScenePath(configuredScenePath)) {
        settings.selectedScenePath = configuredScenePath;
    }
    if (viewport.isObject()) {
        settings.viewportCameraSaved = viewport.get("cameraSaved").asBool(false);
        readVec3(
            viewport.get("orbitTarget"),
            settings.viewportOrbitTarget,
            settings.viewportOrbitTarget
        );
        settings.viewportOrbitYaw = readFloat(viewport.get("orbitYaw"), settings.viewportOrbitYaw);
        settings.viewportOrbitPitch = readFloat(viewport.get("orbitPitch"), settings.viewportOrbitPitch);
        settings.viewportOrbitDistance = std::max(0.05f, readFloat(viewport.get("orbitDistance"), settings.viewportOrbitDistance));
        settings.viewportFov = readFloat(viewport.get("fov"), settings.viewportFov);
        settings.viewportShowGroundGrid = viewport.get("showGroundGrid").asBool(settings.viewportShowGroundGrid);
        settings.viewportShowSceneCameraPreview =
            viewport.get("showSceneCameraPreview").asBool(settings.viewportShowSceneCameraPreview);
    }
    if (sceneViewports.isObject()) {
        for (const auto& entry : sceneViewports.objectValue) {
            const nut::Json& sceneViewport = entry.second;
            if (!sceneViewport.isObject()) {
                continue;
            }
            EditorBuildSettings::SceneViewportCameraSettings sceneSettings;
            sceneSettings.cameraSaved = sceneViewport.get("cameraSaved").asBool(false);
            readVec3(sceneViewport.get("orbitTarget"), sceneSettings.orbitTarget, sceneSettings.orbitTarget);
            sceneSettings.orbitYaw = readFloat(sceneViewport.get("orbitYaw"), sceneSettings.orbitYaw);
            sceneSettings.orbitPitch = readFloat(sceneViewport.get("orbitPitch"), sceneSettings.orbitPitch);
            sceneSettings.orbitDistance = std::max(0.05f, readFloat(sceneViewport.get("orbitDistance"), sceneSettings.orbitDistance));
            sceneSettings.fov = readFloat(sceneViewport.get("fov"), sceneSettings.fov);
            settings.sceneViewportCameras[entry.first] = sceneSettings;
        }
    }

    outMessage = "> Build settings: loaded local user configuration.";
    return true;
}

bool saveBuildSettings(const EditorBuildSettings& settings) {
    std::ostringstream out;
    out << "{\n";
    out << makeJsonKeyValue("platformioPath", settings.platformioPath) << ",\n";
    out << makeJsonKeyValue("uploadPort", settings.uploadPort) << ",\n";
    out << makeJsonKeyValue("selectedScenePath", settings.selectedScenePath) << ",\n";
    out << "  \"viewport\": {\n";
    out << "    \"cameraSaved\": " << (settings.viewportCameraSaved ? "true" : "false") << ",\n";
    out << "    \"orbitTarget\": ["
        << settings.viewportOrbitTarget[0] << ", "
        << settings.viewportOrbitTarget[1] << ", "
        << settings.viewportOrbitTarget[2] << "],\n";
    out << "    \"orbitYaw\": " << settings.viewportOrbitYaw << ",\n";
    out << "    \"orbitPitch\": " << settings.viewportOrbitPitch << ",\n";
    out << "    \"orbitDistance\": " << settings.viewportOrbitDistance << ",\n";
    out << "    \"fov\": " << settings.viewportFov << ",\n";
    out << "    \"showGroundGrid\": " << (settings.viewportShowGroundGrid ? "true" : "false") << ",\n";
    out << "    \"showSceneCameraPreview\": " << (settings.viewportShowSceneCameraPreview ? "true" : "false") << "\n";
    out << "  },\n";
    out << "  \"sceneViewports\": {\n";
    bool firstSceneViewport = true;
    for (const auto& entry : settings.sceneViewportCameras) {
        if (!firstSceneViewport) {
            out << ",\n";
        }
        firstSceneViewport = false;
        out << "    \"" << escapeJsonString(entry.first) << "\": {\n";
        out << "      \"cameraSaved\": " << (entry.second.cameraSaved ? "true" : "false") << ",\n";
        out << "      \"orbitTarget\": ["
            << entry.second.orbitTarget[0] << ", "
            << entry.second.orbitTarget[1] << ", "
            << entry.second.orbitTarget[2] << "],\n";
        out << "      \"orbitYaw\": " << entry.second.orbitYaw << ",\n";
        out << "      \"orbitPitch\": " << entry.second.orbitPitch << ",\n";
        out << "      \"orbitDistance\": " << entry.second.orbitDistance << ",\n";
        out << "      \"fov\": " << entry.second.fov << "\n";
        out << "    }";
    }
    out << "\n  }\n";
    out << "}\n";
    return writeTextFile(buildSettingsConfigPath(), out.str());
}

void syncBuildSettingsBuffers(EditorBuildSettings& settings) {
    std::snprintf(settings.platformioPathBuffer, sizeof(settings.platformioPathBuffer), "%s", settings.platformioPath.c_str());
    std::snprintf(settings.uploadPortBuffer, sizeof(settings.uploadPortBuffer), "%s", settings.uploadPort.c_str());
}

void applyBuildSettingsBuffers(EditorBuildSettings& settings) {
    settings.platformioPath = settings.platformioPathBuffer;
    settings.uploadPort = settings.uploadPortBuffer;
}
