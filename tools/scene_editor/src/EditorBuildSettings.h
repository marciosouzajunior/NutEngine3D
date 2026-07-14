#pragma once

#include <map>
#include <string>

struct EditorBuildSettings {
    struct SceneViewportCameraSettings {
        bool cameraSaved = false;
        float orbitTarget[3] = {0.0f, 0.0f, 0.0f};
        float orbitYaw = 0.0f;
        float orbitPitch = 0.0f;
        float orbitDistance = 5.0f;
        float fov = 60.0f;
    };

    std::string platformioPath;
    std::string uploadPort = "COM5";
    std::string selectedScenePath;
    bool viewportCameraSaved = false;
    float viewportOrbitTarget[3] = {0.0f, 0.0f, 0.0f};
    float viewportOrbitYaw = 0.0f;
    float viewportOrbitPitch = 0.0f;
    float viewportOrbitDistance = 5.0f;
    float viewportFov = 60.0f;
    bool viewportShowGroundGrid = true;
    bool viewportShowSceneCameraPreview = false;
    std::map<std::string, SceneViewportCameraSettings> sceneViewportCameras;
    char platformioPathBuffer[512] = {};
    char uploadPortBuffer[64] = {};
};

bool isValidUploadPort(const std::string& port);
std::string buildSettingsConfigPath();
EditorBuildSettings makeDefaultBuildSettings();
bool loadBuildSettings(EditorBuildSettings& settings, std::string& outMessage);
bool saveBuildSettings(const EditorBuildSettings& settings);
void syncBuildSettingsBuffers(EditorBuildSettings& settings);
void applyBuildSettingsBuffers(EditorBuildSettings& settings);
