#include <raylib.h>
#include <rlImGui.h>
#include <imgui.h>
#include <imgui_internal.h>

#include <cstdint>
#include <TextEditor.h>

#include "../../src/scene/Json.h"
#include "../scene_compiler/ScenePipeline.h"

#include <algorithm>
#include <atomic>
#include <cstring>
#include <cstdlib>
#include <fstream>
#include <map>
#include <mutex>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <thread>
#include <utility>
#include <vector>
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef NOGDI
#define NOGDI
#endif
#ifndef NOUSER
#define NOUSER
#endif
#include <windows.h>
#endif

namespace {

namespace motif {
constexpr ImU32 DarkDesktop = IM_COL32(72, 86, 92, 255);
constexpr ImU32 Panel = IM_COL32(184, 184, 174, 255);
constexpr ImU32 PanelLight = IM_COL32(238, 238, 226, 255);
constexpr ImU32 PanelShadow = IM_COL32(86, 86, 80, 255);
constexpr ImU32 PanelDarkShadow = IM_COL32(46, 50, 48, 255);
constexpr ImU32 TitleTeal = IM_COL32(88, 139, 145, 255);
constexpr ImU32 ViewportBg = IM_COL32(25, 28, 30, 255);
constexpr ImU32 ViewportGrid = IM_COL32(50, 57, 61, 255);
constexpr ImU32 ViewportWire = IM_COL32(220, 226, 234, 255);
constexpr ImU32 AccentYellow = IM_COL32(246, 218, 102, 255);
} // namespace motif

struct EditorLayout {
    ImVec2 hierarchyPos;
    ImVec2 hierarchySize;
    ImVec2 viewportPos;
    ImVec2 viewportSize;
    ImVec2 inspectorPos;
    ImVec2 inspectorSize;
    ImVec2 assetsPos;
    ImVec2 assetsSize;
};

struct EditorScriptData {
    nut::Json scriptJson;
    std::string type;
    bool expanded = true;
};

struct EditorObjectData {
    nut::Json sourceJson;
    std::string name;
    std::string path;
    std::string meshPath;
    float position[3] = {0.0f, 0.0f, 0.0f};
    float rotation[3] = {0.0f, 0.0f, 0.0f};
    float scale[3] = {1.0f, 1.0f, 1.0f};
    std::vector<EditorScriptData> scripts;
    std::vector<std::unique_ptr<EditorObjectData>> children;
};

struct EditorCameraData {
    float position[3] = {0.0f, 0.0f, -5.0f};
    float rotation[3] = {0.0f, 0.0f, 0.0f};
    float fov = 60.0f;
};

enum class EditorSelectionKind {
    None,
    SceneCamera,
    Object
};

struct EditorSceneData {
    nut::Json rootJson;
    std::string scenePath;
    std::string sceneName = "Untitled";
    EditorCameraData camera;
    std::vector<std::unique_ptr<EditorObjectData>> roots;
    std::vector<std::string> assetPaths;
    std::vector<std::string> consoleLines;
    std::vector<std::string> buildOutputLines;
    EditorSelectionKind selectionKind = EditorSelectionKind::None;
    EditorObjectData* selectedObject = nullptr;
    bool loaded = false;
    bool dirty = false;
    bool hasSceneAnalysis = false;
    nut::tooling::SceneAnalysis sceneAnalysis;
    std::string lastBuildRamSummary;
    std::string lastBuildFlashSummary;
};

struct InspectorState {
    EditorObjectData* syncedObject = nullptr;
    EditorSelectionKind syncedSelectionKind = EditorSelectionKind::None;
    char nameBuffer[256] = {};
    char positionBuffers[3][32] = {};
    char rotationBuffers[3][32] = {};
    char scaleBuffers[3][32] = {};
    char fovBuffer[32] = {};
};

struct EditorUiActions {
    bool resetLayout = false;
    bool saveScene = false;
    bool openBuildModal = false;
    bool compileScene = false;
    bool buildNano = false;
    bool uploadNano = false;
    bool buildAndUploadNano = false;
    bool runDesktop = false;
    std::string requestedScenePath;
};

enum class EditorTaskKind {
    CompileScene,
    BuildNano,
    UploadNano,
    BuildAndUploadNano,
    RunDesktop
};

struct EditorBuildSettings {
    std::string platformioPath;
    std::string uploadPort = "COM5";
    std::string selectedScenePath;
    char platformioPathBuffer[512] = {};
    char uploadPortBuffer[64] = {};
};

struct EditorBackgroundTaskState {
    std::atomic<bool> running {false};
    std::atomic<bool> finished {false};
    std::mutex mutex;
    std::vector<std::string> pendingConsoleLines;
    bool success = false;
    bool updatedSceneAnalysis = false;
    bool limitsChanged = false;
    nut::tooling::SceneAnalysis sceneAnalysis;
    std::string ramSummary;
    std::string flashSummary;
    std::thread worker;
};

struct EditorBuildModalState {
    bool openRequested = false;
};

struct EditorTaskInput {
    nut::Json rootJson;
    std::string scenePath;
};

std::map<std::string, bool> g_scriptExpansionStates;
EditorSceneData* g_scriptExpansionSceneData = nullptr;
std::map<std::string, std::unique_ptr<TextEditor>> g_sourceEditors;

void drawScriptSourceRow(
    const std::string& sourceDisplay,
    const std::string& popupId,
    bool hasSource,
    float labelWidth
);
void drawScriptSourceModal(const EditorScriptData& script, const std::string& sourcePath);
std::string toAbsoluteProjectPath(const std::string& relativePath);
void appendConsoleLine(EditorSceneData& sceneData, const std::string& line);
void appendBuildOutputLine(EditorSceneData& sceneData, const std::string& line);
std::string escapeJsonString(const std::string& value);

bool updateSceneAnalysis(EditorSceneData& sceneData, bool logSummary);

const std::vector<std::pair<const char*, const char*>>& availableEditorScenes() {
    static const std::vector<std::pair<const char*, const char*>> scenes = {
        {"Demo", "assets/scenes/demo.nutscene"},
        {"Tunnel Run", "assets/scenes/tunnel_run.nutscene"}
    };
    return scenes;
}

std::string sceneDisplayNameForPath(const std::string& scenePath) {
    for (const auto& entry : availableEditorScenes()) {
        if (scenePath == entry.second) {
            return entry.first;
        }
    }
    return scenePath;
}

bool isKnownEditorScenePath(const std::string& scenePath) {
    for (const auto& entry : availableEditorScenes()) {
        if (scenePath == entry.second) {
            return true;
        }
    }
    return false;
}

std::string buildObjectPath(const std::string& parentPath, const std::string& objectName, size_t siblingIndex) {
    std::ostringstream stream;
    if (!parentPath.empty()) {
        stream << parentPath << "/";
    }
    stream << siblingIndex << ":" << objectName;
    return stream.str();
}

std::string buildScriptStateKey(const EditorObjectData& object, size_t scriptIndex, const EditorScriptData& script) {
    std::ostringstream stream;
    stream << object.path << "#script:" << scriptIndex << ":" << script.type;
    return stream.str();
}

void* scriptExpansionSettingsReadOpen(ImGuiContext*, ImGuiSettingsHandler*, const char* name) {
    if (std::strcmp(name, "ScriptExpansion") == 0) {
        return reinterpret_cast<void*>(1);
    }
    return nullptr;
}

void scriptExpansionSettingsReadLine(ImGuiContext*, ImGuiSettingsHandler*, void*, const char* line) {
    const char* separator = std::strchr(line, '=');
    if (!separator) {
        return;
    }

    std::string key(line, static_cast<size_t>(separator - line));
    std::string value(separator + 1);
    g_scriptExpansionStates[key] = value == "1";
}

EditorLayout calculateResponsiveLayout(float contentTop) {
    ImGuiViewport* viewport = ImGui::GetMainViewport();

    const float outerGap = 16.0f;
    const float panelGap = 20.0f;
    const float bottomHeight = 180.0f;
    const float contentBottom = viewport->Pos.y + viewport->Size.y;

    ImVec2 origin(viewport->Pos.x + outerGap, contentTop + outerGap);
    float availableWidth = viewport->Size.x - outerGap * 2.0f;
    float availableHeight = contentBottom - contentTop - outerGap * 2.0f;

    float leftWidth = availableWidth * 0.19f;
    if (leftWidth < 300.0f) leftWidth = 300.0f;
    if (leftWidth > 380.0f) leftWidth = 380.0f;

    float rightWidth = availableWidth * 0.23f;
    if (rightWidth < 340.0f) rightWidth = 340.0f;
    if (rightWidth > 430.0f) rightWidth = 430.0f;

    float topHeight = availableHeight - bottomHeight - panelGap;
    if (topHeight < 360.0f) topHeight = availableHeight * 0.68f;

    float centerWidth = availableWidth - leftWidth - rightWidth - panelGap * 2.0f;
    if (centerWidth < 420.0f) centerWidth = 420.0f;

    EditorLayout layout;
    layout.hierarchyPos = origin;
    layout.hierarchySize = ImVec2(leftWidth, topHeight);
    layout.viewportPos = ImVec2(origin.x + leftWidth + panelGap, origin.y);
    layout.viewportSize = ImVec2(centerWidth, topHeight);
    layout.inspectorPos = ImVec2(layout.viewportPos.x + centerWidth + panelGap, origin.y);
    layout.inspectorSize = ImVec2(rightWidth, topHeight);
    layout.assetsPos = ImVec2(origin.x, origin.y + topHeight + panelGap);
    layout.assetsSize = ImVec2(availableWidth, bottomHeight);
    return layout;
}

void applyMotifStyle() {
    ImGuiStyle& style = ImGui::GetStyle();

    style.WindowRounding = 0.0f;
    style.ChildRounding = 0.0f;
    style.FrameRounding = 0.0f;
    style.PopupRounding = 0.0f;
    style.ScrollbarRounding = 0.0f;
    style.GrabRounding = 0.0f;
    style.TabRounding = 0.0f;

    style.WindowBorderSize = 2.0f;
    style.ChildBorderSize = 1.0f;
    style.FrameBorderSize = 2.0f;
    style.PopupBorderSize = 1.0f;
    style.TabBorderSize = 1.0f;

    style.WindowPadding = ImVec2(9.0f, 9.0f);
    style.FramePadding = ImVec2(9.0f, 4.0f);
    style.ItemSpacing = ImVec2(9.0f, 7.0f);
    style.ItemInnerSpacing = ImVec2(6.0f, 5.0f);
    style.WindowTitleAlign = ImVec2(0.02f, 0.5f);
    style.ScrollbarSize = 16.0f;
    style.GrabMinSize = 12.0f;

    ImVec4* colors = style.Colors;
    colors[ImGuiCol_Text] = ImVec4(0.02f, 0.02f, 0.02f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.34f, 0.34f, 0.31f, 1.00f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.72f, 0.72f, 0.67f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.72f, 0.72f, 0.67f, 1.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.72f, 0.72f, 0.67f, 1.00f);
    colors[ImGuiCol_Border] = ImVec4(0.18f, 0.19f, 0.18f, 1.00f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.93f, 0.93f, 0.88f, 1.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.86f, 0.86f, 0.80f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.95f, 0.95f, 0.88f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.62f, 0.62f, 0.58f, 1.00f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.58f, 0.60f, 0.56f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.35f, 0.55f, 0.57f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.58f, 0.60f, 0.56f, 1.00f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.73f, 0.73f, 0.68f, 1.00f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.67f, 0.67f, 0.63f, 1.00f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.48f, 0.48f, 0.45f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.40f, 0.40f, 0.38f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.30f, 0.30f, 0.28f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.08f, 0.18f, 0.34f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.48f, 0.48f, 0.45f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.22f, 0.31f, 0.48f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.73f, 0.73f, 0.68f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.82f, 0.82f, 0.76f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.55f, 0.55f, 0.52f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.46f, 0.66f, 0.68f, 1.00f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.53f, 0.72f, 0.74f, 1.00f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.38f, 0.58f, 0.61f, 1.00f);
    colors[ImGuiCol_Separator] = ImVec4(0.38f, 0.38f, 0.35f, 1.00f);
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.20f, 0.35f, 0.55f, 1.00f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.10f, 0.25f, 0.45f, 1.00f);
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.45f, 0.45f, 0.42f, 0.80f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.22f, 0.35f, 0.55f, 0.90f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.10f, 0.25f, 0.45f, 1.00f);
    colors[ImGuiCol_Tab] = ImVec4(0.67f, 0.67f, 0.63f, 1.00f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.78f, 0.78f, 0.72f, 1.00f);
    colors[ImGuiCol_TabActive] = ImVec4(0.72f, 0.72f, 0.68f, 1.00f);
}

void drawRaisedBevel(ImDrawList* drawList, ImVec2 min, ImVec2 max) {
    drawList->AddLine(min, ImVec2(max.x - 1.0f, min.y), motif::PanelLight);
    drawList->AddLine(min, ImVec2(min.x, max.y - 1.0f), motif::PanelLight);
    drawList->AddLine(ImVec2(min.x + 1.0f, max.y - 1.0f), max, motif::PanelShadow);
    drawList->AddLine(ImVec2(max.x - 1.0f, min.y + 1.0f), max, motif::PanelShadow);
    drawList->AddRect(ImVec2(min.x + 1.0f, min.y + 1.0f), ImVec2(max.x - 1.0f, max.y - 1.0f), motif::PanelDarkShadow);
}

void drawSunkenBevel(ImDrawList* drawList, ImVec2 min, ImVec2 max) {
    drawList->AddLine(min, ImVec2(max.x - 1.0f, min.y), motif::PanelShadow);
    drawList->AddLine(min, ImVec2(min.x, max.y - 1.0f), motif::PanelShadow);
    drawList->AddLine(ImVec2(min.x + 1.0f, max.y - 1.0f), max, motif::PanelLight);
    drawList->AddLine(ImVec2(max.x - 1.0f, min.y + 1.0f), max, motif::PanelLight);
}

void drawSunkenPanelFrame() {
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 min = ImGui::GetItemRectMin();
    ImVec2 max = ImGui::GetItemRectMax();
    drawSunkenBevel(drawList, min, max);
}

void drawDesktopPattern() {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImDrawList* drawList = ImGui::GetBackgroundDrawList();
    ImVec2 min = viewport->Pos;
    ImVec2 max(min.x + viewport->Size.x, min.y + viewport->Size.y);

    drawList->AddRectFilled(min, max, motif::DarkDesktop);

    ImU32 lightLine = IM_COL32(88, 104, 108, 42);
    ImU32 darkLine = IM_COL32(54, 66, 70, 38);
    for (float y = min.y; y < max.y; y += 24.0f) {
        drawList->AddLine(ImVec2(min.x, y), ImVec2(max.x, y), lightLine);
    }
    for (float x = min.x; x < max.x; x += 32.0f) {
        drawList->AddLine(ImVec2(x, min.y), ImVec2(x, max.y), darkLine);
    }
}

void applyPanelLayout(ImVec2 pos, ImVec2 size, bool forceLayout) {
    ImGuiCond condition = forceLayout ? ImGuiCond_Always : ImGuiCond_FirstUseEver;
    ImGui::SetNextWindowPos(pos, condition);
    ImGui::SetNextWindowSize(size, condition);
}

bool readTextFile(const std::string& path, std::string& outText) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return false;
    }

    std::ostringstream stream;
    stream << file.rdbuf();
    outText = stream.str();
    return true;
}

bool writeTextFile(const std::string& path, const std::string& text) {
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file.is_open()) {
        return false;
    }

    file.write(text.data(), static_cast<std::streamsize>(text.size()));
    return file.good();
}

bool fileExistsOnDisk(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    return file.good();
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

std::string makeJsonKeyValue(const std::string& key, const std::string& value) {
    std::ostringstream out;
    out << "  \"" << escapeJsonString(key) << "\": \"" << escapeJsonString(value) << "\"";
    return out.str();
}

#ifdef _WIN32
std::string quoteWindowsArg(const std::string& value) {
    std::string quoted = "\"";
    for (char c : value) {
        if (c == '"') {
            quoted += "\\\"";
        } else {
            quoted.push_back(c);
        }
    }
    quoted += "\"";
    return quoted;
}

bool runProcessCapture(
    const std::string& executablePath,
    const std::vector<std::string>& args,
    const std::string& workingDirectory,
    std::vector<std::string>& outLines,
    int& outExitCode
) {
    SECURITY_ATTRIBUTES securityAttributes {};
    securityAttributes.nLength = sizeof(securityAttributes);
    securityAttributes.bInheritHandle = TRUE;

    HANDLE readHandle = nullptr;
    HANDLE writeHandle = nullptr;
    if (!CreatePipe(&readHandle, &writeHandle, &securityAttributes, 0)) {
        outLines.push_back("> Failed to create process output pipe.");
        return false;
    }

    SetHandleInformation(readHandle, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOA startupInfo {};
    startupInfo.cb = sizeof(startupInfo);
    startupInfo.dwFlags = STARTF_USESTDHANDLES;
    startupInfo.hStdOutput = writeHandle;
    startupInfo.hStdError = writeHandle;
    startupInfo.hStdInput = GetStdHandle(STD_INPUT_HANDLE);

    PROCESS_INFORMATION processInfo {};
    std::string commandLine = quoteWindowsArg(executablePath);
    for (const std::string& arg : args) {
        commandLine += " ";
        commandLine += quoteWindowsArg(arg);
    }
    std::vector<char> mutableCommandLine(commandLine.begin(), commandLine.end());
    mutableCommandLine.push_back('\0');

    const BOOL created = CreateProcessA(
        executablePath.c_str(),
        mutableCommandLine.data(),
        nullptr,
        nullptr,
        TRUE,
        CREATE_NO_WINDOW,
        nullptr,
        workingDirectory.empty() ? nullptr : workingDirectory.c_str(),
        &startupInfo,
        &processInfo
    );

    CloseHandle(writeHandle);
    writeHandle = nullptr;

    if (!created) {
        outLines.push_back("> Failed to start process: " + executablePath);
        CloseHandle(readHandle);
        return false;
    }

    std::string buffer;
    char chunk[256];
    DWORD bytesRead = 0;
    while (ReadFile(readHandle, chunk, sizeof(chunk), &bytesRead, nullptr) && bytesRead > 0) {
        buffer.append(chunk, chunk + bytesRead);
        size_t lineEnd = std::string::npos;
        while ((lineEnd = buffer.find('\n')) != std::string::npos) {
            std::string line = buffer.substr(0, lineEnd);
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            outLines.push_back(line);
            buffer.erase(0, lineEnd + 1);
        }
    }

    if (!buffer.empty()) {
        if (!buffer.empty() && buffer.back() == '\r') {
            buffer.pop_back();
        }
        outLines.push_back(buffer);
    }

    WaitForSingleObject(processInfo.hProcess, INFINITE);
    DWORD exitCode = 1;
    GetExitCodeProcess(processInfo.hProcess, &exitCode);
    outExitCode = static_cast<int>(exitCode);

    CloseHandle(processInfo.hThread);
    CloseHandle(processInfo.hProcess);
    CloseHandle(readHandle);
    return true;
}
#endif

std::string getEnvVarCopy(const char* name) {
    const char* value = std::getenv(name);
    return value ? std::string(value) : std::string();
}

std::string joinWindowsPath(const std::string& base, const std::string& suffix) {
    if (base.empty()) {
        return suffix;
    }
    if (base.back() == '/' || base.back() == '\\') {
        return base + suffix;
    }
    return base + "/" + suffix;
}

std::string normalizePathSlashes(std::string path) {
    std::replace(path.begin(), path.end(), '\\', '/');
    return path;
}

std::string resolveExecutableFromPath(const std::string& executableName) {
#ifdef _WIN32
    if (executableName.empty()) {
        return std::string();
    }

    if (fileExistsOnDisk(executableName)) {
        return normalizePathSlashes(executableName);
    }

    char resolvedPath[MAX_PATH] = {};
    const DWORD resolvedLength = SearchPathA(nullptr, executableName.c_str(), nullptr, MAX_PATH, resolvedPath, nullptr);
    if (resolvedLength > 0 && resolvedLength < MAX_PATH) {
        return normalizePathSlashes(resolvedPath);
    }
#endif

    return executableName;
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

std::string trimAsciiWhitespace(const std::string& text) {
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

std::string buildSettingsConfigPath() {
    return std::string(NUT_PROJECT_ROOT) + "/tools/scene_editor/editor_user_settings.json";
}

std::string desktopExecutablePath() {
    return std::string(NUT_PROJECT_ROOT) + "/build/NutEngine3D.exe";
}

EditorBuildSettings makeDefaultBuildSettings() {
    EditorBuildSettings settings;
    settings.platformioPath = detectPlatformIoExecutable();
    settings.uploadPort = defaultUploadPort();
    settings.selectedScenePath = availableEditorScenes().front().second;
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
    if (!configuredPath.empty()) {
        settings.platformioPath = configuredPath;
    }
    if (!configuredPort.empty()) {
        settings.uploadPort = configuredPort;
    }
    if (isKnownEditorScenePath(configuredScenePath)) {
        settings.selectedScenePath = configuredScenePath;
    }

    outMessage = "> Build settings: loaded local user configuration.";
    return true;
}

bool saveBuildSettings(const EditorBuildSettings& settings) {
    std::ostringstream out;
    out << "{\n";
    out << makeJsonKeyValue("platformioPath", settings.platformioPath) << ",\n";
    out << makeJsonKeyValue("uploadPort", settings.uploadPort) << ",\n";
    out << makeJsonKeyValue("selectedScenePath", settings.selectedScenePath) << "\n";
    out << "}\n";
    return writeTextFile(buildSettingsConfigPath(), out.str());
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

std::string formatJsonValue(const nut::Json& value) {
    if (value.isString()) {
        return value.asString();
    }

    if (value.isNumber()) {
        std::ostringstream stream;
        stream << value.asNumber();
        return stream.str();
    }

    if (value.isBool()) {
        return value.asBool() ? "true" : "false";
    }

    if (value.isArray()) {
        std::ostringstream stream;
        stream << "[";
        for (size_t i = 0; i < value.size(); ++i) {
            if (i > 0) {
                stream << ", ";
            }
            stream << formatJsonValue(value.at(i));
        }
        stream << "]";
        return stream.str();
    }

    if (value.isObject()) {
        return "{...}";
    }

    return "null";
}

nut::Json makeJsonNumber(double value) {
    nut::Json json;
    json.type = nut::Json::Type::Number;
    json.numberValue = value;
    return json;
}

nut::Json makeJsonString(const std::string& value) {
    nut::Json json;
    json.type = nut::Json::Type::String;
    json.stringValue = value;
    return json;
}

nut::Json makeJsonVec3(const float values[3]) {
    nut::Json json;
    json.type = nut::Json::Type::Array;
    json.arrayValue.push_back(makeJsonNumber(values[0]));
    json.arrayValue.push_back(makeJsonNumber(values[1]));
    json.arrayValue.push_back(makeJsonNumber(values[2]));
    return json;
}

void syncScriptJson(EditorScriptData& script) {
    if (!script.scriptJson.isObject()) {
        script.scriptJson.type = nut::Json::Type::Object;
        script.scriptJson.objectValue.clear();
    }
    script.scriptJson.objectValue["type"] = makeJsonString(script.type);
}

nut::Json buildObjectJson(const EditorObjectData& object) {
    nut::Json objectJson = object.sourceJson;
    objectJson.type = nut::Json::Type::Object;
    objectJson.objectValue["name"] = makeJsonString(object.name);

    if (object.meshPath.empty()) {
        objectJson.objectValue.erase("mesh");
    } else {
        objectJson.objectValue["mesh"] = makeJsonString(object.meshPath);
    }

    objectJson.objectValue["position"] = makeJsonVec3(object.position);
    objectJson.objectValue["rotation"] = makeJsonVec3(object.rotation);
    objectJson.objectValue["scale"] = makeJsonVec3(object.scale);

    nut::Json scriptsJson;
    scriptsJson.type = nut::Json::Type::Array;
    for (const EditorScriptData& script : object.scripts) {
        nut::Json scriptJson = script.scriptJson;
        scriptJson.type = nut::Json::Type::Object;
        scriptJson.objectValue["type"] = makeJsonString(script.type);
        scriptsJson.arrayValue.push_back(scriptJson);
    }
    objectJson.objectValue["scripts"] = scriptsJson;

    nut::Json childrenJson;
    childrenJson.type = nut::Json::Type::Array;
    for (const std::unique_ptr<EditorObjectData>& child : object.children) {
        childrenJson.arrayValue.push_back(buildObjectJson(*child));
    }
    objectJson.objectValue["children"] = childrenJson;

    return objectJson;
}

nut::Json buildSceneJson(const EditorSceneData& sceneData) {
    nut::Json rootJson = sceneData.rootJson;
    rootJson.type = nut::Json::Type::Object;
    rootJson.objectValue["name"] = makeJsonString(sceneData.sceneName);

    nut::Json cameraJson;
    cameraJson.type = nut::Json::Type::Object;
    cameraJson.objectValue["position"] = makeJsonVec3(sceneData.camera.position);
    cameraJson.objectValue["rotation"] = makeJsonVec3(sceneData.camera.rotation);
    cameraJson.objectValue["fov"] = makeJsonNumber(sceneData.camera.fov);
    rootJson.objectValue["camera"] = cameraJson;

    nut::Json objectsJson;
    objectsJson.type = nut::Json::Type::Array;
    for (const std::unique_ptr<EditorObjectData>& root : sceneData.roots) {
        objectsJson.arrayValue.push_back(buildObjectJson(*root));
    }
    rootJson.objectValue["objects"] = objectsJson;

    return rootJson;
}

void commitObjectJson(EditorObjectData& object) {
    for (EditorScriptData& script : object.scripts) {
        syncScriptJson(script);
    }

    for (std::unique_ptr<EditorObjectData>& child : object.children) {
        commitObjectJson(*child);
    }

    object.sourceJson = buildObjectJson(object);
}

void commitSceneJson(EditorSceneData& sceneData) {
    for (std::unique_ptr<EditorObjectData>& root : sceneData.roots) {
        commitObjectJson(*root);
    }
    sceneData.rootJson = buildSceneJson(sceneData);
}

std::string escapeJsonString(const std::string& value) {
    std::string escaped;
    escaped.reserve(value.size() + 8);

    for (char c : value) {
        switch (c) {
        case '\\': escaped += "\\\\"; break;
        case '"': escaped += "\\\""; break;
        case '\n': escaped += "\\n"; break;
        case '\r': escaped += "\\r"; break;
        case '\t': escaped += "\\t"; break;
        default: escaped.push_back(c); break;
        }
    }

    return escaped;
}

void appendJsonIndent(std::ostringstream& out, int indentLevel) {
    for (int i = 0; i < indentLevel; ++i) {
        out << "  ";
    }
}

void appendJsonValue(std::ostringstream& out, const nut::Json& value, int indentLevel);

void appendJsonArray(std::ostringstream& out, const nut::Json& value, int indentLevel) {
    if (value.arrayValue.empty()) {
        out << "[]";
        return;
    }

    out << "[\n";
    for (size_t i = 0; i < value.arrayValue.size(); ++i) {
        appendJsonIndent(out, indentLevel + 1);
        appendJsonValue(out, value.arrayValue[i], indentLevel + 1);
        if (i + 1 < value.arrayValue.size()) {
            out << ",";
        }
        out << "\n";
    }
    appendJsonIndent(out, indentLevel);
    out << "]";
}

void appendJsonObject(std::ostringstream& out, const nut::Json& value, int indentLevel) {
    if (value.objectValue.empty()) {
        out << "{}";
        return;
    }

    out << "{\n";
    size_t index = 0;
    for (const auto& entry : value.objectValue) {
        appendJsonIndent(out, indentLevel + 1);
        out << "\"" << escapeJsonString(entry.first) << "\": ";
        appendJsonValue(out, entry.second, indentLevel + 1);
        if (index + 1 < value.objectValue.size()) {
            out << ",";
        }
        out << "\n";
        ++index;
    }
    appendJsonIndent(out, indentLevel);
    out << "}";
}

void appendJsonValue(std::ostringstream& out, const nut::Json& value, int indentLevel) {
    switch (value.type) {
    case nut::Json::Type::Null:
        out << "null";
        return;
    case nut::Json::Type::Bool:
        out << (value.boolValue ? "true" : "false");
        return;
    case nut::Json::Type::Number:
        out << value.numberValue;
        return;
    case nut::Json::Type::String:
        out << "\"" << escapeJsonString(value.stringValue) << "\"";
        return;
    case nut::Json::Type::Array:
        appendJsonArray(out, value, indentLevel);
        return;
    case nut::Json::Type::Object:
        appendJsonObject(out, value, indentLevel);
        return;
    }
}

std::string serializeJson(const nut::Json& value) {
    std::ostringstream out;
    appendJsonValue(out, value, 0);
    out << "\n";
    return out.str();
}

bool saveSceneToDisk(EditorSceneData& sceneData) {
    if (!sceneData.loaded) {
        appendConsoleLine(sceneData, "> Save failed: no scene is currently loaded.");
        return false;
    }

    if (sceneData.scenePath.empty()) {
        appendConsoleLine(sceneData, "> Save failed: the current scene has no path.");
        return false;
    }

    commitSceneJson(sceneData);
    const std::string outputPath = toAbsoluteProjectPath(sceneData.scenePath);
    const std::string serialized = serializeJson(sceneData.rootJson);
    if (!writeTextFile(outputPath, serialized)) {
        appendConsoleLine(sceneData, "> Save failed: could not write scene file.");
        return false;
    }

    sceneData.dirty = false;
    updateSceneAnalysis(sceneData, false);
    appendConsoleLine(sceneData, "> Saved scene: " + sceneData.scenePath);
    return true;
}

EditorScriptData parseScript(const nut::Json& scriptJson) {
    EditorScriptData script;
    script.scriptJson = scriptJson;
    script.type = scriptJson.get("type").asString("UnknownScript");
    syncScriptJson(script);
    return script;
}

std::unique_ptr<EditorObjectData> parseObject(
    const nut::Json& objectJson,
    std::set<std::string>& assets,
    const std::string& parentPath,
    size_t siblingIndex,
    size_t& objectCount,
    size_t& scriptCount
) {
    auto object = std::make_unique<EditorObjectData>();
    object->sourceJson = objectJson;
    object->name = objectJson.get("name").asString("GameObject");
    object->path = buildObjectPath(parentPath, object->name, siblingIndex);
    object->meshPath = objectJson.get("mesh").asString("");

    static const float defaultPosition[3] = {0.0f, 0.0f, 0.0f};
    static const float defaultRotation[3] = {0.0f, 0.0f, 0.0f};
    static const float defaultScale[3] = {1.0f, 1.0f, 1.0f};
    readVec3(objectJson.get("position"), object->position, defaultPosition);
    readVec3(objectJson.get("rotation"), object->rotation, defaultRotation);
    readVec3(objectJson.get("scale"), object->scale, defaultScale);

    if (!object->meshPath.empty()) {
        assets.insert(object->meshPath);
    }

    const nut::Json& scripts = objectJson.get("scripts");
    if (scripts.isArray()) {
        for (size_t i = 0; i < scripts.size(); ++i) {
            object->scripts.push_back(parseScript(scripts.at(i)));
            ++scriptCount;
        }
    }

    const nut::Json& children = objectJson.get("children");
    if (children.isArray()) {
        for (size_t i = 0; i < children.size(); ++i) {
            object->children.push_back(parseObject(children.at(i), assets, object->path, i, objectCount, scriptCount));
        }
    }

    ++objectCount;
    return object;
}

EditorObjectData* findFirstObject(std::vector<std::unique_ptr<EditorObjectData>>& roots) {
    if (roots.empty()) {
        return nullptr;
    }
    return roots.front().get();
}

void appendConsoleLine(EditorSceneData& sceneData, const std::string& line) {
    sceneData.consoleLines.push_back(line);
}

void appendBuildOutputLine(EditorSceneData& sceneData, const std::string& line) {
    sceneData.buildOutputLines.push_back(line);
}

std::string trimCopy(const std::string& text) {
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

bool updateSceneAnalysis(EditorSceneData& sceneData, bool logSummary) {
    std::string error;
    nut::tooling::SceneAnalysis analysis;
    if (!nut::tooling::analyzeScene(toAbsoluteProjectPath(sceneData.scenePath), sceneData.rootJson, analysis, error)) {
        sceneData.hasSceneAnalysis = false;
        if (logSummary) {
            appendConsoleLine(sceneData, "> Scene analysis failed: " + error);
        }
        return false;
    }

    sceneData.sceneAnalysis = analysis;
    sceneData.hasSceneAnalysis = true;

    if (logSummary) {
        std::ostringstream requirementsSummary;
        requirementsSummary
            << "> Nano requirements: roots=" << analysis.requirements.rootObjects
            << ", objects=" << analysis.requirements.objects
            << ", max children=" << analysis.requirements.maxChildrenPerObject
            << ", meshes=" << analysis.requirements.meshes
            << ", scripts=" << analysis.requirements.scripts
            << ", max scripts/object=" << analysis.requirements.maxScriptsPerObject
            << ", max verts/mesh=" << analysis.requirements.maxVerticesPerMesh
            << ", max edges/mesh=" << analysis.requirements.maxEdgesPerMesh
            << ", string table=" << analysis.requirements.stringTableBytes << " bytes.";
        appendConsoleLine(sceneData, requirementsSummary.str());
    }

    return true;
}

void formatFloatToBuffer(float value, char* buffer, size_t bufferSize) {
    std::snprintf(buffer, bufferSize, "%.3f", value);
}

bool loadSceneData(EditorSceneData& sceneData, const std::string& scenePathOnDisk, const std::string& scenePathDisplay) {
    sceneData = EditorSceneData();
    sceneData.scenePath = scenePathDisplay;
    sceneData.assetPaths.push_back(scenePathDisplay);

    std::string text;
    if (!readTextFile(scenePathOnDisk, text)) {
        appendConsoleLine(sceneData, "> Failed to read scene: " + scenePathDisplay);
        return false;
    }

    nut::Json root;
    std::string error;
    if (!nut::Json::parse(text, root, &error)) {
        appendConsoleLine(sceneData, "> Failed to parse scene JSON: " + error);
        return false;
    }

    if (!root.isObject()) {
        appendConsoleLine(sceneData, "> Invalid .nutscene: root must be a JSON object.");
        return false;
    }

    sceneData.sceneName = root.get("name").asString("Untitled");
    sceneData.rootJson = root;

    const nut::Json& camera = root.get("camera");
    static const float defaultCameraPosition[3] = {0.0f, 0.0f, -5.0f};
    static const float defaultCameraRotation[3] = {0.0f, 0.0f, 0.0f};
    if (camera.isObject()) {
        readVec3(camera.get("position"), sceneData.camera.position, defaultCameraPosition);
        readVec3(camera.get("rotation"), sceneData.camera.rotation, defaultCameraRotation);
        sceneData.camera.fov = readFloat(camera.get("fov"), 60.0f);
    }

    std::set<std::string> uniqueAssets;
    uniqueAssets.insert(scenePathDisplay);

    size_t objectCount = 0;
    size_t scriptCount = 0;
    const nut::Json& objects = root.get("objects");
    if (objects.isArray()) {
        for (size_t i = 0; i < objects.size(); ++i) {
            sceneData.roots.push_back(parseObject(objects.at(i), uniqueAssets, "", i, objectCount, scriptCount));
        }
    } else {
        appendConsoleLine(sceneData, "> Invalid .nutscene: missing objects array.");
        return false;
    }

    sceneData.assetPaths.assign(uniqueAssets.begin(), uniqueAssets.end());
    sceneData.selectedObject = findFirstObject(sceneData.roots);
    sceneData.selectionKind = sceneData.selectedObject ? EditorSelectionKind::Object : EditorSelectionKind::SceneCamera;
    sceneData.loaded = true;

    appendConsoleLine(sceneData, "> Loaded scene: " + scenePathDisplay);
    appendConsoleLine(sceneData, "> Scene name: " + sceneData.sceneName);

    std::ostringstream summary;
    summary << "> Parsed " << objectCount << " object(s), "
            << scriptCount << " script instance(s), "
            << sceneData.assetPaths.size() << " asset reference(s).";
    appendConsoleLine(sceneData, summary.str());

    updateSceneAnalysis(sceneData, true);

    appendConsoleLine(sceneData, "> Scene is ready for in-editor save.");
    return true;
}

void collectScriptExpansionStates(
    const EditorObjectData& object,
    std::map<std::string, bool>& states
) {
    for (size_t i = 0; i < object.scripts.size(); ++i) {
        const EditorScriptData& script = object.scripts[i];
        states[buildScriptStateKey(object, i, script)] = script.expanded;
    }

    for (const std::unique_ptr<EditorObjectData>& child : object.children) {
        collectScriptExpansionStates(*child, states);
    }
}

void applyScriptExpansionStates(
    EditorObjectData& object,
    const std::map<std::string, bool>& states
) {
    for (size_t i = 0; i < object.scripts.size(); ++i) {
        EditorScriptData& script = object.scripts[i];
        const std::string key = buildScriptStateKey(object, i, script);
        auto it = states.find(key);
        if (it != states.end()) {
            script.expanded = it->second;
        }
    }

    for (std::unique_ptr<EditorObjectData>& child : object.children) {
        applyScriptExpansionStates(*child, states);
    }
}

void applyScriptExpansionStates(
    std::vector<std::unique_ptr<EditorObjectData>>& roots,
    const std::map<std::string, bool>& states
) {
    for (std::unique_ptr<EditorObjectData>& root : roots) {
        applyScriptExpansionStates(*root, states);
    }
}

void scriptExpansionSettingsWriteAll(ImGuiContext*, ImGuiSettingsHandler* handler, ImGuiTextBuffer* outBuf) {
    if (!g_scriptExpansionSceneData) {
        return;
    }

    g_scriptExpansionStates.clear();
    for (const std::unique_ptr<EditorObjectData>& root : g_scriptExpansionSceneData->roots) {
        collectScriptExpansionStates(*root, g_scriptExpansionStates);
    }

    if (g_scriptExpansionStates.empty()) {
        return;
    }

    outBuf->appendf("[%s][ScriptExpansion]\n", handler->TypeName);
    for (const auto& entry : g_scriptExpansionStates) {
        outBuf->appendf("%s=%d\n", entry.first.c_str(), entry.second ? 1 : 0);
    }
    outBuf->append("\n");
}

EditorUiActions drawMainMenu(const EditorSceneData& sceneData) {
    EditorUiActions actions;
    const char* scenePath = sceneData.scenePath.c_str();

    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::BeginMenu("Open Scene")) {
                for (const auto& entry : availableEditorScenes()) {
                    const bool selected = sceneData.scenePath == entry.second;
                    if (ImGui::MenuItem(entry.first, nullptr, selected)) {
                        actions.requestedScenePath = entry.second;
                    }
                }
                ImGui::EndMenu();
            }
            if (ImGui::MenuItem("Save", "Ctrl+S", false, sceneData.loaded)) {
                actions.saveScene = true;
            }
            ImGui::MenuItem("Save As...", nullptr, false, false);
            ImGui::Separator();
            ImGui::MenuItem("Exit");
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Edit")) {
            ImGui::MenuItem("Undo", nullptr, false, false);
            ImGui::MenuItem("Redo", nullptr, false, false);
            ImGui::Separator();
            ImGui::MenuItem("Duplicate Object", nullptr, false, false);
            ImGui::MenuItem("Delete Object", nullptr, false, false);
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View")) {
            if (ImGui::MenuItem("Reset Layout")) {
                actions.resetLayout = true;
            }
            ImGui::MenuItem("Fullscreen", "F11");
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Scene")) {
            if (ImGui::MenuItem("Build...", nullptr, false, sceneData.loaded)) {
                actions.openBuildModal = true;
            }
            ImGui::Separator();
            ImGui::MenuItem("Add Empty Object", nullptr, false, false);
            ImGui::MenuItem("Add Mesh Object", nullptr, false, false);
            ImGui::MenuItem("Validate Scene", nullptr, false, false);
            ImGui::EndMenu();
        }

        ImVec2 textSize = ImGui::CalcTextSize(scenePath);
        float centeredX = (ImGui::GetWindowWidth() - textSize.x) * 0.5f;
        if (centeredX > ImGui::GetCursorPosX()) {
            ImGui::SameLine(centeredX);
        } else {
            ImGui::SameLine();
        }
        ImGui::TextDisabled("%s", scenePath);

        ImGui::EndMainMenuBar();
    }

    return actions;
}

EditorUiActions drawToolbar(float menuHeight, const EditorSceneData& sceneData, float& outContentTop) {
    EditorUiActions actions;
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    const float toolbarHeight = 44.0f;

    ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x, viewport->Pos.y + menuHeight));
    ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, toolbarHeight));

    ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoSavedSettings;

    ImGui::Begin("Toolbar", nullptr, flags);
    ImVec2 toolbarMin = ImGui::GetWindowPos();
    ImVec2 toolbarMax(toolbarMin.x + ImGui::GetWindowWidth(), toolbarMin.y + ImGui::GetWindowHeight());
    drawRaisedBevel(ImGui::GetWindowDrawList(), toolbarMin, toolbarMax);

    ImGui::BeginDisabled();
    ImGui::Button("Open", ImVec2(72, 0));
    ImGui::EndDisabled();
    ImGui::SameLine();

    const std::string currentSceneLabel = sceneDisplayNameForPath(sceneData.scenePath);
    ImGui::SetNextItemWidth(150.0f);
    if (ImGui::BeginCombo("##SceneSelector", currentSceneLabel.c_str())) {
        for (const auto& entry : availableEditorScenes()) {
            const bool selected = sceneData.scenePath == entry.second;
            if (ImGui::Selectable(entry.first, selected)) {
                actions.requestedScenePath = entry.second;
            }
            if (selected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    ImGui::SameLine();

    if (sceneData.loaded) {
        if (ImGui::Button("Save", ImVec2(72, 0))) {
            actions.saveScene = true;
        }
    } else {
        ImGui::BeginDisabled();
        ImGui::Button("Save", ImVec2(72, 0));
        ImGui::EndDisabled();
    }

    ImGui::SameLine();
    ImGui::BeginDisabled();
    ImGui::Button("Add Object", ImVec2(112, 0));
    ImGui::SameLine();
    ImGui::Button("Add Script", ImVec2(104, 0));
    ImGui::EndDisabled();
    ImGui::SameLine();

    if (sceneData.loaded) {
        if (ImGui::Button("Build", ImVec2(120, 0))) {
            actions.openBuildModal = true;
        }
    } else {
        ImGui::BeginDisabled();
        ImGui::Button("Build", ImVec2(120, 0));
        ImGui::EndDisabled();
    }

    std::string status;
    if (!sceneData.loaded) {
        status = "Scene load failed - showing diagnostics in Console";
    } else if (sceneData.dirty) {
        status = "Unsaved scene changes";
    } else {
        status = "Scene saved to JSON";
    }
    ImVec2 statusSize = ImGui::CalcTextSize(status.c_str());
    float statusX = ImGui::GetWindowWidth() - statusSize.x - 16.0f;
    if (statusX > ImGui::GetCursorPosX()) {
        ImGui::SameLine(statusX);
        ImGui::TextDisabled("%s", status.c_str());
    }
    ImGui::End();

    outContentTop = viewport->Pos.y + menuHeight + toolbarHeight;
    return actions;
}

void drawHierarchyNode(EditorObjectData& object, EditorSceneData& sceneData) {
    const bool isSelected =
        sceneData.selectionKind == EditorSelectionKind::Object &&
        sceneData.selectedObject == &object;
    ImGuiTreeNodeFlags flags =
        ImGuiTreeNodeFlags_OpenOnArrow |
        ImGuiTreeNodeFlags_OpenOnDoubleClick |
        ImGuiTreeNodeFlags_DefaultOpen;
    if (object.children.empty()) {
        flags |= ImGuiTreeNodeFlags_Leaf;
    }
    if (isSelected) {
        flags |= ImGuiTreeNodeFlags_Selected;
    }

    bool isOpen = ImGui::TreeNodeEx(static_cast<void*>(&object), flags, "%s", object.name.c_str());
    if (ImGui::IsItemClicked()) {
        sceneData.selectionKind = EditorSelectionKind::Object;
        sceneData.selectedObject = &object;
    }

    if (isOpen) {
        for (std::unique_ptr<EditorObjectData>& child : object.children) {
            drawHierarchyNode(*child, sceneData);
        }
        ImGui::TreePop();
    }
}

void drawHierarchy(const EditorLayout& layout, bool forceLayout, EditorSceneData& sceneData) {
    applyPanelLayout(layout.hierarchyPos, layout.hierarchySize, forceLayout);

    ImGui::Begin("Scene Hierarchy");
    ImGui::TextDisabled("Scene: %s", sceneData.sceneName.c_str());
    ImGui::Separator();

    ImGuiTreeNodeFlags cameraFlags =
        ImGuiTreeNodeFlags_Leaf |
        ImGuiTreeNodeFlags_NoTreePushOnOpen;
    if (sceneData.selectionKind == EditorSelectionKind::SceneCamera) {
        cameraFlags |= ImGuiTreeNodeFlags_Selected;
    }
    ImGui::TreeNodeEx("SceneCamera", cameraFlags, "Scene Camera");
    if (ImGui::IsItemClicked()) {
        sceneData.selectionKind = EditorSelectionKind::SceneCamera;
        sceneData.selectedObject = nullptr;
    }

    if (!sceneData.loaded) {
        ImGui::TextDisabled("No scene data available.");
    } else if (sceneData.roots.empty()) {
        ImGui::TextDisabled("Scene has no root objects.");
    } else {
        for (std::unique_ptr<EditorObjectData>& root : sceneData.roots) {
            drawHierarchyNode(*root, sceneData);
        }
    }

    ImGui::End();
}

void drawViewport(const EditorLayout& layout, bool forceLayout, const EditorSceneData& sceneData) {
    applyPanelLayout(layout.viewportPos, layout.viewportSize, forceLayout);

    ImGui::Begin("Viewport");
    if (sceneData.selectionKind == EditorSelectionKind::SceneCamera) {
        ImGui::TextDisabled("Preview placeholder for: Scene Camera");
    } else if (sceneData.selectedObject) {
        ImGui::TextDisabled("Preview placeholder for: %s", sceneData.selectedObject->name.c_str());
    } else {
        ImGui::TextDisabled("Wireframe preview placeholder");
    }
    ImGui::Separator();

    ImVec2 canvasPos = ImGui::GetCursorScreenPos();
    ImVec2 canvasSize = ImGui::GetContentRegionAvail();
    if (canvasSize.x < 50.0f) canvasSize.x = 50.0f;
    if (canvasSize.y < 50.0f) canvasSize.y = 50.0f;

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImU32 bg = motif::ViewportBg;
    ImU32 grid = motif::ViewportGrid;
    ImU32 wire = motif::ViewportWire;
    ImU32 sphere = motif::AccentYellow;

    drawList->AddRectFilled(canvasPos, ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y), bg);
    drawSunkenBevel(drawList, canvasPos, ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y));

    for (float x = canvasPos.x; x < canvasPos.x + canvasSize.x; x += 28.0f) {
        drawList->AddLine(ImVec2(x, canvasPos.y), ImVec2(x, canvasPos.y + canvasSize.y), grid);
    }
    for (float y = canvasPos.y; y < canvasPos.y + canvasSize.y; y += 28.0f) {
        drawList->AddLine(ImVec2(canvasPos.x, y), ImVec2(canvasPos.x + canvasSize.x, y), grid);
    }

    ImVec2 c(canvasPos.x + canvasSize.x * 0.5f, canvasPos.y + canvasSize.y * 0.52f);
    drawList->AddLine(ImVec2(c.x - 60, c.y - 28), ImVec2(c.x + 58, c.y - 28), wire);
    drawList->AddLine(ImVec2(c.x + 58, c.y - 28), ImVec2(c.x + 78, c.y + 34), wire);
    drawList->AddLine(ImVec2(c.x + 78, c.y + 34), ImVec2(c.x - 38, c.y + 34), wire);
    drawList->AddLine(ImVec2(c.x - 38, c.y + 34), ImVec2(c.x - 60, c.y - 28), wire);
    drawList->AddLine(ImVec2(c.x - 60, c.y - 28), ImVec2(c.x - 24, c.y - 70), wire);
    drawList->AddLine(ImVec2(c.x + 58, c.y - 28), ImVec2(c.x + 24, c.y - 70), wire);
    drawList->AddLine(ImVec2(c.x - 24, c.y - 70), ImVec2(c.x + 24, c.y - 70), wire);
    drawList->AddLine(ImVec2(c.x + 24, c.y - 70), ImVec2(c.x + 78, c.y + 34), wire);
    drawList->AddLine(ImVec2(c.x - 24, c.y - 70), ImVec2(c.x - 38, c.y + 34), wire);
    drawList->AddCircle(ImVec2(c.x + 132, c.y - 16), 26.0f, sphere, 24);

    ImGui::Dummy(canvasSize);
    ImGui::End();
}

void drawReadOnlyInputText(const char* label, const std::string& value) {
    std::vector<char> buffer(value.begin(), value.end());
    buffer.push_back('\0');
    ImGui::InputText(label, buffer.data(), buffer.size(), ImGuiInputTextFlags_ReadOnly);
}

void drawLabeledReadOnlyInputText(const char* label, const std::string& value, float labelWidth) {
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(label);
    ImGui::SameLine();
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + std::max(0.0f, labelWidth - ImGui::CalcTextSize(label).x));
    ImGui::SameLine();

    std::vector<char> buffer(value.begin(), value.end());
    buffer.push_back('\0');
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    std::string inputId = std::string("##") + label;
    ImGui::InputText(inputId.c_str(), buffer.data(), buffer.size(), ImGuiInputTextFlags_ReadOnly);
}

std::string getScriptSourcePath(const std::string& typeName) {
    return "assets/scripts/" + typeName + ".cpp";
}

std::string toAbsoluteProjectPath(const std::string& relativePath) {
    return std::string(NUT_PROJECT_ROOT) + "/" + relativePath;
}

bool projectFileExists(const std::string& relativePath) {
    return FileExists(toAbsoluteProjectPath(relativePath).c_str());
}

TextEditor::Palette buildMotifSourcePalette() {
    TextEditor::Palette palette = TextEditor::GetLightPalette();
    palette[static_cast<size_t>(TextEditor::PaletteIndex::Background)] = motif::Panel;
    palette[static_cast<size_t>(TextEditor::PaletteIndex::Default)] = IM_COL32(28, 28, 24, 255);
    palette[static_cast<size_t>(TextEditor::PaletteIndex::Keyword)] = IM_COL32(32, 58, 122, 255);
    palette[static_cast<size_t>(TextEditor::PaletteIndex::Number)] = IM_COL32(104, 42, 118, 255);
    palette[static_cast<size_t>(TextEditor::PaletteIndex::String)] = IM_COL32(138, 66, 28, 255);
    palette[static_cast<size_t>(TextEditor::PaletteIndex::CharLiteral)] = IM_COL32(138, 66, 28, 255);
    palette[static_cast<size_t>(TextEditor::PaletteIndex::Preprocessor)] = IM_COL32(126, 88, 22, 255);
    palette[static_cast<size_t>(TextEditor::PaletteIndex::Comment)] = IM_COL32(64, 102, 58, 255);
    palette[static_cast<size_t>(TextEditor::PaletteIndex::MultiLineComment)] = IM_COL32(64, 102, 58, 255);
    palette[static_cast<size_t>(TextEditor::PaletteIndex::LineNumber)] = IM_COL32(104, 104, 96, 255);
    palette[static_cast<size_t>(TextEditor::PaletteIndex::CurrentLineFill)] = IM_COL32(172, 176, 168, 255);
    palette[static_cast<size_t>(TextEditor::PaletteIndex::CurrentLineFillInactive)] = IM_COL32(180, 184, 174, 255);
    palette[static_cast<size_t>(TextEditor::PaletteIndex::Selection)] = IM_COL32(88, 139, 145, 96);
    palette[static_cast<size_t>(TextEditor::PaletteIndex::Cursor)] = motif::PanelDarkShadow;
    return palette;
}

TextEditor& getSourceEditor(const std::string& sourcePath) {
    auto it = g_sourceEditors.find(sourcePath);
    if (it != g_sourceEditors.end()) {
        return *it->second;
    }

    auto editor = std::make_unique<TextEditor>();
    editor->SetLanguageDefinition(TextEditor::LanguageDefinition::CPlusPlus());
    editor->SetPalette(buildMotifSourcePalette());
    editor->SetShowWhitespaces(false);
    editor->SetReadOnly(true);

    std::string sourceText;
    const std::string absolutePath = toAbsoluteProjectPath(sourcePath);
    if (!readTextFile(absolutePath, sourceText)) {
        sourceText = "// Could not read source file:\n// " + sourcePath;
    }
    editor->SetText(sourceText);

    TextEditor& editorRef = *editor;
    g_sourceEditors[sourcePath] = std::move(editor);
    return editorRef;
}

void drawScriptPropertyRow(const char* label, const std::string& value, float labelWidth) {
    const float lineStartX = ImGui::GetCursorPosX();
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(label);
    ImGui::SameLine(lineStartX + labelWidth);

    std::vector<char> buffer(value.begin(), value.end());
    buffer.push_back('\0');
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    std::string inputId = std::string("##Prop") + label;
    ImGui::InputText(inputId.c_str(), buffer.data(), buffer.size(), ImGuiInputTextFlags_ReadOnly);
}

size_t countVisibleScriptProperties(const EditorScriptData& script) {
    if (!script.scriptJson.isObject()) {
        return 0;
    }

    size_t count = 0;
    for (const auto& entry : script.scriptJson.objectValue) {
        if (entry.first != "type") {
            ++count;
        }
    }
    return count;
}

void drawScriptCard(
    EditorScriptData& script,
    const std::string& sourcePath,
    float labelWidth
) {
    const bool hasSource = projectFileExists(sourcePath);
    const std::string sourceDisplay = hasSource ? sourcePath : "(not found)";
    const std::string popupId = script.type + "##SourceModal";

    const ImGuiStyle& style = ImGui::GetStyle();
    const float lineHeight = ImGui::GetFrameHeightWithSpacing();
    float propertyLabelWidth = 0.0f;
    if (script.scriptJson.isObject()) {
        for (const auto& entry : script.scriptJson.objectValue) {
            if (entry.first == "type") {
                continue;
            }
            propertyLabelWidth = std::max(propertyLabelWidth, ImGui::CalcTextSize(entry.first.c_str()).x);
        }
    }
    propertyLabelWidth += 16.0f;

    float cardHeight = style.WindowPadding.y * 2.0f + ImGui::GetFrameHeight();
    if (script.expanded) {
        const size_t propertyCount = countVisibleScriptProperties(script);
        const float propertyRows = propertyCount == 0 ? 1.0f : static_cast<float>(propertyCount);
        cardHeight += 8.0f;
        cardHeight += lineHeight * (2.0f + propertyRows);
    } else {
        cardHeight += 4.0f;
    }

    ImGui::BeginChild(
        (script.type + "##Card").c_str(),
        ImVec2(0.0f, cardHeight),
        true
    );

    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(script.type.c_str());
    ImGui::SameLine();
    const float buttonWidth = 18.0f;
    const float buttonX = ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - buttonWidth;
    if (buttonX > ImGui::GetCursorPosX()) {
        ImGui::SetCursorPosX(buttonX);
    }
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2.0f, 1.0f));
    if (ImGui::SmallButton(script.expanded ? "-" : "+")) {
        script.expanded = !script.expanded;
    }
    ImGui::PopStyleVar();

    if (script.expanded) {
        ImGui::Separator();
        drawScriptSourceRow(sourceDisplay, popupId, hasSource, labelWidth);
        if (hasSource) {
            drawScriptSourceModal(script, sourcePath);
        }

        ImGui::Spacing();
        ImGui::TextDisabled("Properties");
        const bool hasProperties = countVisibleScriptProperties(script) > 0;
        if (!hasProperties) {
            ImGui::TextDisabled("No properties.");
        } else {
            for (const auto& entry : script.scriptJson.objectValue) {
                if (entry.first == "type") {
                    continue;
                }
                drawScriptPropertyRow(entry.first.c_str(), formatJsonValue(entry.second), propertyLabelWidth);
            }
        }
    }

    ImGui::EndChild();
    drawSunkenPanelFrame();
}

void drawScriptSourceRow(
    const std::string& sourceDisplay,
    const std::string& popupId,
    bool hasSource,
    float labelWidth
) {
    const float lineStartX = ImGui::GetCursorPosX();
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("Source");
    ImGui::SameLine(lineStartX + labelWidth);

    const float buttonWidth = 48.0f;
    const ImGuiStyle& style = ImGui::GetStyle();
    float fieldWidth = ImGui::GetContentRegionAvail().x - buttonWidth - style.ItemSpacing.x;
    if (fieldWidth < 80.0f) {
        fieldWidth = 80.0f;
    }

    std::vector<char> buffer(sourceDisplay.begin(), sourceDisplay.end());
    buffer.push_back('\0');
    ImGui::SetNextItemWidth(fieldWidth);
    ImGui::InputText("##Source", buffer.data(), buffer.size(), ImGuiInputTextFlags_ReadOnly);
    ImGui::SameLine();

    if (hasSource) {
        if (ImGui::Button("View", ImVec2(buttonWidth, 0.0f))) {
            ImGui::OpenPopup(popupId.c_str());
        }
    } else {
        ImGui::BeginDisabled();
        ImGui::Button("View", ImVec2(buttonWidth, 0.0f));
        ImGui::EndDisabled();
    }
}

void drawScriptSourcePreview(const std::string& sourcePath) {
    TextEditor& editor = getSourceEditor(sourcePath);
    editor.Render("##SourcePreviewEditor", ImVec2(-FLT_MIN, -FLT_MIN), false);
}

void drawScriptSourceModal(const EditorScriptData& script, const std::string& sourcePath) {
    const std::string popupId = script.type + "##SourceModal";
    ImGui::SetNextWindowSize(ImVec2(760.0f, 520.0f), ImGuiCond_Appearing);
    bool keepOpen = true;
    if (ImGui::BeginPopupModal(popupId.c_str(), &keepOpen)) {
        ImGui::TextDisabled("%s", sourcePath.c_str());
        ImGui::Spacing();
        ImGui::BeginChild("SourceModalBody", ImVec2(0.0f, 0.0f), false);
        drawScriptSourcePreview(sourcePath);
        ImGui::EndChild();
        if (!keepOpen) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void syncInspectorState(InspectorState& inspectorState, const EditorSceneData& sceneData) {
    if (inspectorState.syncedSelectionKind == sceneData.selectionKind &&
        inspectorState.syncedObject == sceneData.selectedObject) {
        return;
    }

    inspectorState.syncedSelectionKind = sceneData.selectionKind;
    inspectorState.syncedObject = sceneData.selectedObject;
    inspectorState.nameBuffer[0] = '\0';

    if (sceneData.selectionKind == EditorSelectionKind::SceneCamera) {
        for (int i = 0; i < 3; ++i) {
            formatFloatToBuffer(sceneData.camera.position[i], inspectorState.positionBuffers[i], sizeof(inspectorState.positionBuffers[i]));
            formatFloatToBuffer(sceneData.camera.rotation[i], inspectorState.rotationBuffers[i], sizeof(inspectorState.rotationBuffers[i]));
        }
        formatFloatToBuffer(sceneData.camera.fov, inspectorState.fovBuffer, sizeof(inspectorState.fovBuffer));
        return;
    }

    EditorObjectData* selectedObject = sceneData.selectedObject;
    if (!selectedObject) {
        return;
    }

    const size_t maxLength = sizeof(inspectorState.nameBuffer) - 1;
    const size_t copyLength = std::min(selectedObject->name.size(), maxLength);
    selectedObject->name.copy(inspectorState.nameBuffer, copyLength);
    inspectorState.nameBuffer[copyLength] = '\0';

    for (int i = 0; i < 3; ++i) {
        formatFloatToBuffer(selectedObject->position[i], inspectorState.positionBuffers[i], sizeof(inspectorState.positionBuffers[i]));
        formatFloatToBuffer(selectedObject->rotation[i], inspectorState.rotationBuffers[i], sizeof(inspectorState.rotationBuffers[i]));
        formatFloatToBuffer(selectedObject->scale[i], inspectorState.scaleBuffers[i], sizeof(inspectorState.scaleBuffers[i]));
    }
}

void markSceneDirty(EditorSceneData& sceneData) {
    if (!sceneData.dirty) {
        sceneData.dirty = true;
        appendConsoleLine(sceneData, "> Scene modified in memory. Save to write changes to disk.");
    }
}

void syncBuildSettingsBuffers(EditorBuildSettings& settings) {
    std::snprintf(settings.platformioPathBuffer, sizeof(settings.platformioPathBuffer), "%s", settings.platformioPath.c_str());
    std::snprintf(settings.uploadPortBuffer, sizeof(settings.uploadPortBuffer), "%s", settings.uploadPort.c_str());
}

void applyBuildSettingsBuffers(EditorBuildSettings& settings) {
    settings.platformioPath = settings.platformioPathBuffer;
    settings.uploadPort = settings.uploadPortBuffer;
}

std::string activeTaskLabel(EditorTaskKind taskKind) {
    switch (taskKind) {
    case EditorTaskKind::CompileScene:
        return "Compile Scene";
    case EditorTaskKind::BuildNano:
        return "Build Nano";
    case EditorTaskKind::UploadNano:
        return "Upload Nano";
    case EditorTaskKind::BuildAndUploadNano:
        return "Build & Upload";
    case EditorTaskKind::RunDesktop:
        return "Run Desktop";
    }
    return "Task";
}

bool compileSceneForNano(
    const EditorTaskInput& taskInput,
    nut::tooling::SceneAnalysis& outAnalysis,
    bool& outLimitsChanged,
    std::vector<std::string>& outLines
) {
    std::string error;
    const std::string inputPath = toAbsoluteProjectPath(taskInput.scenePath);
    const std::string outputHeaderPath = toAbsoluteProjectPath("build/assets/nano/demo_scene.h");
    const std::string outputLimitsPath = toAbsoluteProjectPath("build/assets/nano/demo_scene_limits.h");
    std::string previousLimitsText;
    const bool hadPreviousLimits = readTextFile(outputLimitsPath, previousLimitsText);

    nut::tooling::CompiledSceneData compiled;
    if (!nut::tooling::compileScene(inputPath, taskInput.rootJson, compiled, error)) {
        outLines.push_back("> Scene compilation failed: " + error);
        return false;
    }

    if (!nut::tooling::writeSceneHeader(inputPath, outputHeaderPath, compiled, error)) {
        outLines.push_back("> Scene header write failed: " + error);
        return false;
    }

    if (!nut::tooling::writeSceneLimitsHeader(inputPath, outputLimitsPath, compiled.analysis, error)) {
        outLines.push_back("> Scene limits write failed: " + error);
        return false;
    }

    std::string newLimitsText;
    if (!readTextFile(outputLimitsPath, newLimitsText)) {
        outLines.push_back("> Scene limits verification failed: could not re-read generated limits file.");
        return false;
    }

    outLimitsChanged = !hadPreviousLimits || previousLimitsText != newLimitsText;
    outAnalysis = compiled.analysis;
    outLines.push_back("> Scene compiled for Nano.");
    outLines.push_back("> Output header: build/assets/nano/demo_scene.h");
    outLines.push_back("> Output limits: build/assets/nano/demo_scene_limits.h");
    outLines.push_back(outLimitsChanged
        ? "> Scene limits changed: Nano build will clean before recompiling."
        : "> Scene limits unchanged.");
    return true;
}

void collectPlatformIoSummaries(
    const std::vector<std::string>& lines,
    std::string& outRamSummary,
    std::string& outFlashSummary
) {
    for (const std::string& line : lines) {
        const std::string trimmed = trimCopy(line);
        if (trimmed.rfind("RAM:", 0) == 0 || trimmed.rfind("DATA:", 0) == 0) {
            outRamSummary = trimmed;
        } else if (trimmed.rfind("Flash:", 0) == 0 || trimmed.rfind("PROGRAM:", 0) == 0) {
            outFlashSummary = trimmed;
        }
    }
}

void addUploadFailureHintIfNeeded(const std::vector<std::string>& lines, std::vector<std::string>& outLines) {
    for (const std::string& line : lines) {
        if (line.find("Access is denied") != std::string::npos
            || line.find("PermissionError") != std::string::npos
            || line.find("could not open port") != std::string::npos
            || line.find("Resource busy") != std::string::npos) {
            outLines.push_back("> Upload hint: the serial port may be busy. Close the serial monitor or any tool holding the configured port and try again.");
            return;
        }
    }
}

bool runPlatformIoCommand(
    const EditorBuildSettings& settings,
    const std::vector<std::string>& extraArgs,
    std::vector<std::string>& outLines,
    std::string& outRamSummary,
    std::string& outFlashSummary
) {
#ifdef _WIN32
    std::vector<std::string> args {
        "run",
        "-d", toAbsoluteProjectPath("targets/nano"),
        "-e", "nano"
    };
    args.insert(args.end(), extraArgs.begin(), extraArgs.end());

    int exitCode = 1;
    if (!runProcessCapture(settings.platformioPath, args, toAbsoluteProjectPath("targets/nano"), outLines, exitCode)) {
        return false;
    }

    collectPlatformIoSummaries(outLines, outRamSummary, outFlashSummary);
    return exitCode == 0;
#else
    (void)settings;
    (void)extraArgs;
    (void)outRamSummary;
    (void)outFlashSummary;
    outLines.push_back("> PlatformIO integration is currently implemented for Windows in this editor build.");
    return false;
#endif
}

bool runPlatformIoClean(const EditorBuildSettings& settings, std::vector<std::string>& outLines) {
#ifdef _WIN32
    std::string ramSummary;
    std::string flashSummary;
    return runPlatformIoCommand(settings, {"-t", "clean"}, outLines, ramSummary, flashSummary);
#else
    (void)settings;
    outLines.push_back("> PlatformIO clean is currently implemented for Windows in this editor build.");
    return false;
#endif
}

bool buildDesktopExecutable(std::vector<std::string>& outLines) {
#ifdef _WIN32
    const std::string cmakeExecutable = resolveExecutableFromPath("cmake.exe");
    std::vector<std::string> args {
        "--build",
        toAbsoluteProjectPath("build"),
        "--target",
        "NutEngine3D"
    };

    int exitCode = 1;
    if (!runProcessCapture(cmakeExecutable, args, toAbsoluteProjectPath("."), outLines, exitCode)) {
        return false;
    }

    return exitCode == 0;
#else
    outLines.push_back("> Desktop build is currently implemented for Windows in this editor build.");
    return false;
#endif
}

bool launchDesktopExecutable(std::vector<std::string>& outLines) {
#ifdef _WIN32
    const std::string executablePath = desktopExecutablePath();
    if (!fileExistsOnDisk(executablePath)) {
        outLines.push_back("> Desktop launch failed: build/NutEngine3D.exe was not found.");
        return false;
    }

    STARTUPINFOA startupInfo {};
    startupInfo.cb = sizeof(startupInfo);
    PROCESS_INFORMATION processInfo {};
    std::string commandLine = quoteWindowsArg(executablePath);
    std::vector<char> mutableCommandLine(commandLine.begin(), commandLine.end());
    mutableCommandLine.push_back('\0');

    const BOOL created = CreateProcessA(
        executablePath.c_str(),
        mutableCommandLine.data(),
        nullptr,
        nullptr,
        FALSE,
        0,
        nullptr,
        toAbsoluteProjectPath("build").c_str(),
        &startupInfo,
        &processInfo
    );

    if (!created) {
        outLines.push_back("> Desktop launch failed: could not start build/NutEngine3D.exe.");
        return false;
    }

    CloseHandle(processInfo.hThread);
    CloseHandle(processInfo.hProcess);
    outLines.push_back("> Desktop simulator launched: build/NutEngine3D.exe");
    outLines.push_back("> Runtime output for the simulator appears in its own process window.");
    return true;
#else
    outLines.push_back("> Desktop launch is currently implemented for Windows in this editor build.");
    return false;
#endif
}

void startEditorTask(
    EditorBackgroundTaskState& taskState,
    const EditorSceneData& sceneData,
    const EditorBuildSettings& settings,
    EditorTaskKind taskKind
) {
    if (taskState.running.load()) {
        return;
    }

    {
        std::lock_guard<std::mutex> lock(taskState.mutex);
        taskState.pendingConsoleLines.clear();
        taskState.finished = false;
        taskState.success = false;
        taskState.updatedSceneAnalysis = false;
        taskState.limitsChanged = false;
        taskState.ramSummary.clear();
        taskState.flashSummary.clear();
    }

    EditorTaskInput taskInput;
    taskInput.rootJson = sceneData.rootJson;
    taskInput.scenePath = sceneData.scenePath;
    const EditorBuildSettings settingsSnapshot = settings;
    taskState.running = true;
    taskState.worker = std::thread([&taskState, taskInput, settingsSnapshot, taskKind]() {
        std::vector<std::string> lines;
        lines.push_back("> " + activeTaskLabel(taskKind) + " started.");

        bool success = true;
        nut::tooling::SceneAnalysis analysis;
        bool updatedSceneAnalysis = false;
        bool limitsChanged = false;
        std::string ramSummary;
        std::string flashSummary;

        if (taskKind == EditorTaskKind::CompileScene
            || taskKind == EditorTaskKind::BuildNano
            || taskKind == EditorTaskKind::UploadNano
            || taskKind == EditorTaskKind::BuildAndUploadNano
            || taskKind == EditorTaskKind::RunDesktop) {
            success = compileSceneForNano(taskInput, analysis, limitsChanged, lines);
            updatedSceneAnalysis = success;
        }

        if (success
            && limitsChanged
            && (taskKind == EditorTaskKind::BuildNano
                || taskKind == EditorTaskKind::UploadNano
                || taskKind == EditorTaskKind::BuildAndUploadNano)) {
            std::vector<std::string> cleanLines;
            cleanLines.push_back("> Running PlatformIO clean because scene limits changed...");
            const bool cleanOk = runPlatformIoClean(settingsSnapshot, cleanLines);
            lines.insert(lines.end(), cleanLines.begin(), cleanLines.end());
            success = cleanOk;
        }

        if (success && (taskKind == EditorTaskKind::BuildNano || taskKind == EditorTaskKind::BuildAndUploadNano)) {
            std::vector<std::string> buildLines;
            buildLines.push_back("> Running PlatformIO build...");
            const bool buildOk = runPlatformIoCommand(settingsSnapshot, {}, buildLines, ramSummary, flashSummary);
            lines.insert(lines.end(), buildLines.begin(), buildLines.end());
            success = buildOk;
        }

        if (success && (taskKind == EditorTaskKind::UploadNano || taskKind == EditorTaskKind::BuildAndUploadNano)) {
            std::vector<std::string> uploadArgs {"-t", "upload"};
            if (isValidUploadPort(settingsSnapshot.uploadPort)) {
                uploadArgs.push_back("--upload-port");
                uploadArgs.push_back(settingsSnapshot.uploadPort);
            }

            std::vector<std::string> uploadLines;
            uploadLines.push_back("> Running PlatformIO upload...");
            const bool uploadOk = runPlatformIoCommand(settingsSnapshot, uploadArgs, uploadLines, ramSummary, flashSummary);
            if (!uploadOk) {
                std::vector<std::string> uploadHints;
                addUploadFailureHintIfNeeded(uploadLines, uploadHints);
                uploadLines.insert(uploadLines.end(), uploadHints.begin(), uploadHints.end());
            }
            lines.insert(lines.end(), uploadLines.begin(), uploadLines.end());
            success = uploadOk;
        }

        if (success && taskKind == EditorTaskKind::RunDesktop) {
            std::vector<std::string> desktopBuildLines;
            desktopBuildLines.push_back("> Building desktop simulator...");
            const bool buildDesktopOk = buildDesktopExecutable(desktopBuildLines);
            lines.insert(lines.end(), desktopBuildLines.begin(), desktopBuildLines.end());
            success = buildDesktopOk;
        }

        if (success && taskKind == EditorTaskKind::RunDesktop) {
            std::vector<std::string> launchLines;
            launchLines.push_back("> Launching desktop simulator...");
            const bool launchOk = launchDesktopExecutable(launchLines);
            lines.insert(lines.end(), launchLines.begin(), launchLines.end());
            success = launchOk;
        }

        lines.push_back(success
            ? "> " + activeTaskLabel(taskKind) + " finished successfully."
            : "> " + activeTaskLabel(taskKind) + " failed.");

        {
            std::lock_guard<std::mutex> lock(taskState.mutex);
            taskState.pendingConsoleLines = std::move(lines);
            taskState.finished = true;
            taskState.success = success;
            taskState.updatedSceneAnalysis = updatedSceneAnalysis;
            taskState.limitsChanged = limitsChanged;
            taskState.sceneAnalysis = analysis;
            taskState.ramSummary = ramSummary;
            taskState.flashSummary = flashSummary;
        }

        taskState.running = false;
    });
}

void flushEditorTask(EditorBackgroundTaskState& taskState, EditorSceneData& sceneData) {
    if (!taskState.finished.load()) {
        return;
    }

    {
        std::lock_guard<std::mutex> lock(taskState.mutex);
        for (const std::string& line : taskState.pendingConsoleLines) {
            appendConsoleLine(sceneData, line);
            appendBuildOutputLine(sceneData, line);
        }
        taskState.pendingConsoleLines.clear();

        if (taskState.updatedSceneAnalysis) {
            sceneData.sceneAnalysis = taskState.sceneAnalysis;
            sceneData.hasSceneAnalysis = true;
        }
        if (!taskState.ramSummary.empty()) {
            sceneData.lastBuildRamSummary = taskState.ramSummary;
        }
        if (!taskState.flashSummary.empty()) {
            sceneData.lastBuildFlashSummary = taskState.flashSummary;
        }

        taskState.finished = false;
    }

    if (taskState.worker.joinable()) {
        taskState.worker.join();
    }
}

bool tryParseFloatBuffer(const char* buffer, float& outValue) {
    char* end = nullptr;
    const float parsedValue = std::strtof(buffer, &end);
    if (end == buffer) {
        return false;
    }

    while (*end != '\0') {
        if (!std::isspace(static_cast<unsigned char>(*end))) {
            return false;
        }
        ++end;
    }

    outValue = parsedValue;
    return true;
}

bool drawVec3TextFields(const char* label, float values[3], char buffers[3][32]) {
    bool changed = false;
    ImGui::PushID(label);
    const ImGuiStyle& style = ImGui::GetStyle();
    const float lineStartX = ImGui::GetCursorPosX();
    const float totalWidth = ImGui::GetContentRegionAvail().x;
    const float labelWidth = 72.0f;
    const float slotGap = style.ItemSpacing.x;
    const float axisGap = style.ItemInnerSpacing.x;
    const float axisLabelWidth = ImGui::CalcTextSize("X").x;
    const float slotsWidth = std::max(120.0f, totalWidth - labelWidth - style.ItemSpacing.x);
    const float slotWidth = std::max(36.0f, (slotsWidth - slotGap * 2.0f) / 3.0f);
    const float fieldWidth = std::max(24.0f, slotWidth - axisLabelWidth - axisGap - 8.0f);

    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(label);
    ImGui::SameLine(lineStartX + labelWidth);

    const char* axisLabels[3] = {"X", "Y", "Z"};
    for (int i = 0; i < 3; ++i) {
        if (i > 0) {
            ImGui::SameLine(0.0f, slotGap);
        }

        ImGui::BeginGroup();
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(axisLabels[i]);
        ImGui::SameLine(0.0f, axisGap);
        ImGui::SetNextItemWidth(fieldWidth);
        std::string inputId = std::string("##") + axisLabels[i];
        if (ImGui::InputText(inputId.c_str(), buffers[i], sizeof(buffers[i]), ImGuiInputTextFlags_EnterReturnsTrue)) {
            float parsedValue = values[i];
            if (tryParseFloatBuffer(buffers[i], parsedValue) && parsedValue != values[i]) {
                values[i] = parsedValue;
                changed = true;
            }
            formatFloatToBuffer(values[i], buffers[i], sizeof(buffers[i]));
        }
        if (ImGui::IsItemDeactivatedAfterEdit()) {
            float parsedValue = values[i];
            if (tryParseFloatBuffer(buffers[i], parsedValue) && parsedValue != values[i]) {
                values[i] = parsedValue;
                changed = true;
            }
            formatFloatToBuffer(values[i], buffers[i], sizeof(buffers[i]));
        }
        ImGui::EndGroup();
    }

    ImGui::PopID();
    return changed;
}

bool drawSingleFloatField(const char* label, float& value, char buffer[32], float labelWidth) {
    bool changed = false;
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(label);
    ImGui::SameLine();
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + std::max(0.0f, labelWidth - ImGui::CalcTextSize(label).x));
    ImGui::SameLine();
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    std::string inputId = std::string("##") + label;
    if (ImGui::InputText(inputId.c_str(), buffer, 32, ImGuiInputTextFlags_EnterReturnsTrue)) {
        float parsedValue = value;
        if (tryParseFloatBuffer(buffer, parsedValue) && parsedValue != value) {
            value = parsedValue;
            changed = true;
        }
        formatFloatToBuffer(value, buffer, 32);
    }
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        float parsedValue = value;
        if (tryParseFloatBuffer(buffer, parsedValue) && parsedValue != value) {
            value = parsedValue;
            changed = true;
        }
        formatFloatToBuffer(value, buffer, 32);
    }
    return changed;
}

void drawInspector(const EditorLayout& layout, bool forceLayout, EditorSceneData& sceneData, InspectorState& inspectorState) {
    applyPanelLayout(layout.inspectorPos, layout.inspectorSize, forceLayout);

    ImGui::Begin("Inspector");

    if (sceneData.selectionKind == EditorSelectionKind::None) {
        ImGui::TextDisabled("Nothing selected.");
        ImGui::End();
        return;
    }

    syncInspectorState(inspectorState, sceneData);
    const float inspectorLabelWidth = 72.0f;

    if (sceneData.selectionKind == EditorSelectionKind::SceneCamera) {
        ImGui::BeginDisabled();
        drawLabeledReadOnlyInputText("Type", std::string("Scene Camera"), inspectorLabelWidth);
        ImGui::EndDisabled();

        if (drawVec3TextFields("Position", sceneData.camera.position, inspectorState.positionBuffers)) {
            markSceneDirty(sceneData);
        }
        if (drawVec3TextFields("Rotation", sceneData.camera.rotation, inspectorState.rotationBuffers)) {
            markSceneDirty(sceneData);
        }
        if (drawSingleFloatField("FOV", sceneData.camera.fov, inspectorState.fovBuffer, inspectorLabelWidth)) {
            markSceneDirty(sceneData);
        }

        if (sceneData.dirty) {
            ImGui::Spacing();
            ImGui::TextDisabled("Unsaved scene changes.");
        }

        ImGui::End();
        return;
    }

    if (!sceneData.selectedObject) {
        ImGui::TextDisabled("Nothing selected.");
        ImGui::End();
        return;
    }

    EditorObjectData& object = *sceneData.selectedObject;

    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("Name");
    ImGui::SameLine();
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + std::max(0.0f, inspectorLabelWidth - ImGui::CalcTextSize("Name").x));
    ImGui::SameLine();
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    if (ImGui::InputText("##Name", inspectorState.nameBuffer, sizeof(inspectorState.nameBuffer))) {
        object.name = inspectorState.nameBuffer;
        markSceneDirty(sceneData);
    }

    std::string meshLabel = object.meshPath.empty() ? "(empty)" : object.meshPath;
    ImGui::BeginDisabled();
    drawLabeledReadOnlyInputText("Mesh", meshLabel, inspectorLabelWidth);
    ImGui::EndDisabled();

    if (drawVec3TextFields("Position", object.position, inspectorState.positionBuffers)) {
        markSceneDirty(sceneData);
    }
    if (drawVec3TextFields("Rotation", object.rotation, inspectorState.rotationBuffers)) {
        markSceneDirty(sceneData);
    }
    if (drawVec3TextFields("Scale", object.scale, inspectorState.scaleBuffers)) {
        markSceneDirty(sceneData);
    }

    ImGui::Spacing();
    ImGui::SeparatorText("Scripts");
    if (object.scripts.empty()) {
        ImGui::TextDisabled("No scripts attached.");
    } else {
        for (size_t i = 0; i < object.scripts.size(); ++i) {
            EditorScriptData& script = object.scripts[i];
            const std::string sourcePath = getScriptSourcePath(script.type);
            ImGui::PushID(static_cast<int>(i));
            drawScriptCard(script, sourcePath, inspectorLabelWidth);
            if (i + 1 < object.scripts.size()) {
                ImGui::Spacing();
            }
            ImGui::PopID();
        }
    }

    if (sceneData.dirty) {
        ImGui::Spacing();
        ImGui::TextDisabled("Unsaved scene changes.");
    }

    ImGui::End();
}

EditorUiActions drawAssetsConsole(
    const EditorLayout& layout,
    bool forceLayout,
    const EditorSceneData& sceneData
) {
    EditorUiActions actions;
    applyPanelLayout(layout.assetsPos, layout.assetsSize, forceLayout);

    ImGui::Begin("Assets / Console");
    if (ImGui::BeginTabBar("AssetsConsoleTabs")) {
        if (ImGui::BeginTabItem("Assets")) {
            if (sceneData.assetPaths.empty()) {
                ImGui::TextDisabled("No assets referenced.");
            } else {
                for (const std::string& assetPath : sceneData.assetPaths) {
                    ImGui::BulletText("%s", assetPath.c_str());
                }
            }
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Console")) {
            for (const std::string& line : sceneData.consoleLines) {
                ImGui::TextUnformatted(line.c_str());
            }
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }
    ImGui::End();
    return actions;
}

void drawSceneBudgetSummary(const EditorSceneData& sceneData) {
    if (!sceneData.hasSceneAnalysis) {
        ImGui::TextDisabled("No scene analysis available.");
        return;
    }

    const nut::tooling::SceneRequirements& r = sceneData.sceneAnalysis.requirements;
    if (ImGui::BeginTable("SceneBudgetTable", 2, ImGuiTableFlags_SizingStretchSame)) {
        ImGui::TableNextColumn();
        ImGui::BulletText("Objects: %u", static_cast<unsigned>(r.objects));
        ImGui::BulletText("Root objects: %u", static_cast<unsigned>(r.rootObjects));
        ImGui::BulletText("Maximum children: %u", static_cast<unsigned>(r.maxChildrenPerObject));
        ImGui::BulletText("Meshes: %u", static_cast<unsigned>(r.meshes));
        ImGui::BulletText("Scripts: %u", static_cast<unsigned>(r.scripts));

        ImGui::TableNextColumn();
        ImGui::BulletText("Maximum scripts per object: %u", static_cast<unsigned>(r.maxScriptsPerObject));
        ImGui::BulletText("Vertices per mesh: %u", static_cast<unsigned>(r.maxVerticesPerMesh));
        ImGui::BulletText("Edges per mesh: %u", static_cast<unsigned>(r.maxEdgesPerMesh));
        ImGui::BulletText("String-table bytes: %u", static_cast<unsigned>(r.stringTableBytes));
        ImGui::EndTable();
    }

    const bool exceedsComfort =
        r.objects > 4
        || r.scripts > 3
        || r.maxVerticesPerMesh > 40
        || r.maxEdgesPerMesh > 84;
    const bool nearTestedLimit =
        r.objects > 5
        || r.scripts > 3
        || r.maxVerticesPerMesh > 48
        || r.maxEdgesPerMesh > 120
        || r.maxChildrenPerObject > 4
        || r.maxScriptsPerObject > 3;

    ImGui::Spacing();
    if (nearTestedLimit) {
        ImGui::TextWrapped("Guidance: Near or beyond the upper tested Nano range; validate on hardware.");
    } else if (exceedsComfort) {
        ImGui::TextWrapped("Guidance: Above the tested Nano comfort zone; expect tighter headroom.");
    } else {
        ImGui::TextWrapped("Guidance: Within the tested Nano comfort zone, but still validate on hardware.");
    }
}

void drawFirmwareBuildSummary(const EditorSceneData& sceneData) {
    if (sceneData.lastBuildRamSummary.empty()) {
        ImGui::TextDisabled("RAM: not built yet");
    } else {
        ImGui::TextWrapped("%s", sceneData.lastBuildRamSummary.c_str());
    }

    if (sceneData.lastBuildFlashSummary.empty()) {
        ImGui::TextDisabled("Flash: not built yet");
    } else {
        ImGui::TextWrapped("%s", sceneData.lastBuildFlashSummary.c_str());
    }
}

std::string buildOutputText(const EditorSceneData& sceneData) {
    std::ostringstream out;
    for (size_t i = 0; i < sceneData.buildOutputLines.size(); ++i) {
        out << sceneData.buildOutputLines[i];
        if (i + 1 < sceneData.buildOutputLines.size()) {
            out << '\n';
        }
    }
    return out.str();
}

EditorUiActions drawBuildModal(
    EditorBuildModalState& modalState,
    EditorSceneData& sceneData,
    EditorBuildSettings& buildSettings,
    const EditorBackgroundTaskState& taskState
) {
    EditorUiActions actions;

    if (modalState.openRequested) {
        ImGui::OpenPopup("Build Nano");
        modalState.openRequested = false;
    }

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    const ImVec2 modalSize(
        std::min(viewport->Size.x - 80.0f, 1040.0f),
        std::min(viewport->Size.y - 80.0f, 720.0f)
    );
    ImGui::SetNextWindowSize(modalSize, ImGuiCond_Appearing);
    ImGui::SetNextWindowPos(
        ImVec2(viewport->Pos.x + viewport->Size.x * 0.5f, viewport->Pos.y + viewport->Size.y * 0.5f),
        ImGuiCond_Appearing,
        ImVec2(0.5f, 0.5f)
    );

    bool keepOpen = true;
    if (!ImGui::BeginPopupModal("Build Nano", &keepOpen, ImGuiWindowFlags_NoCollapse)) {
        return actions;
    }

    ImGui::TextDisabled("Toolchain");
    ImGui::SetNextItemWidth(-FLT_MIN);
    if (ImGui::InputText("PlatformIO Path", buildSettings.platformioPathBuffer, sizeof(buildSettings.platformioPathBuffer))) {
        applyBuildSettingsBuffers(buildSettings);
        saveBuildSettings(buildSettings);
    }
    ImGui::SetNextItemWidth(220.0f);
    if (ImGui::InputText("Upload Port", buildSettings.uploadPortBuffer, sizeof(buildSettings.uploadPortBuffer))) {
        applyBuildSettingsBuffers(buildSettings);
        saveBuildSettings(buildSettings);
    }
    ImGui::TextDisabled("Saved locally in tools/scene_editor/editor_user_settings.json");
    if (!isValidUploadPort(buildSettings.uploadPort)) {
        ImGui::TextDisabled("Upload port uses an unsupported format.");
    }

    ImGui::Spacing();
    ImGui::SeparatorText("Scene Budget");
    drawSceneBudgetSummary(sceneData);

    ImGui::Spacing();
    ImGui::SeparatorText("Firmware Build");
    drawFirmwareBuildSummary(sceneData);

    ImGui::Spacing();
    if (taskState.running.load()) {
        ImGui::TextDisabled("Background task running...");
    } else {
        if (ImGui::Button("Compile Scene", ImVec2(140.0f, 0.0f))) {
            actions.compileScene = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("Build Nano", ImVec2(120.0f, 0.0f))) {
            actions.buildNano = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("Upload Nano", ImVec2(128.0f, 0.0f))) {
            actions.uploadNano = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("Build & Upload", ImVec2(140.0f, 0.0f))) {
            actions.buildAndUploadNano = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("Run Desktop", ImVec2(140.0f, 0.0f))) {
            actions.runDesktop = true;
        }
    }

    ImGui::Spacing();
    ImGui::SeparatorText("Output");
    if (ImGui::Button("Clear Output")) {
        sceneData.buildOutputLines.clear();
    }

    const float footerHeight = ImGui::GetFrameHeightWithSpacing() + 8.0f;
    if (ImGui::BeginChild("BuildOutputPanel", ImVec2(0.0f, -footerHeight), false, ImGuiWindowFlags_HorizontalScrollbar)) {
        std::string outputText = buildOutputText(sceneData);
        std::vector<char> outputBuffer(outputText.begin(), outputText.end());
        outputBuffer.push_back('\0');
        ImGui::InputTextMultiline(
            "##BuildOutputText",
            outputBuffer.data(),
            outputBuffer.size(),
            ImVec2(-FLT_MIN, -FLT_MIN),
            ImGuiInputTextFlags_ReadOnly
        );
    }
    ImGui::EndChild();

    ImGui::EndPopup();
    return actions;
}

} // namespace

int main() {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(1280, 800, "NutScene Editor");
    MaximizeWindow();
    SetTargetFPS(60);

    rlImGuiBeginInitImGui();
    ImGuiIO& io = ImGui::GetIO();
    static const std::string editorIniPath = std::string(NUT_PROJECT_ROOT) + "/tools/scene_editor/nutscene_editor.ini";
    const bool hasSavedLayout = FileExists(editorIniPath.c_str());

    io.IniFilename = editorIniPath.c_str();
    ImGuiSettingsHandler scriptExpansionHandler;
    scriptExpansionHandler.TypeName = "NutSceneEditor";
    scriptExpansionHandler.TypeHash = ImHashStr("NutSceneEditor");
    scriptExpansionHandler.ReadOpenFn = scriptExpansionSettingsReadOpen;
    scriptExpansionHandler.ReadLineFn = scriptExpansionSettingsReadLine;
    scriptExpansionHandler.WriteAllFn = scriptExpansionSettingsWriteAll;
    ImGui::GetCurrentContext()->SettingsHandlers.push_back(scriptExpansionHandler);
    ImGui::LoadIniSettingsFromDisk(editorIniPath.c_str());

    const std::string editorFontPath = std::string(NUT_PROJECT_ROOT) + "/tools/scene_editor/assets/fonts/ProggyClean.ttf";
    if (FileExists(editorFontPath.c_str())) {
        io.Fonts->AddFontFromFileTTF(editorFontPath.c_str(), 16.0f);
    } else {
        io.Fonts->AddFontDefault();
    }
    applyMotifStyle();
    rlImGuiEndInitImGui();

    bool forceLayout = !hasSavedLayout;
    EditorSceneData sceneData;
    InspectorState inspectorState;
    EditorBuildSettings buildSettings = makeDefaultBuildSettings();
    syncBuildSettingsBuffers(buildSettings);
    EditorBackgroundTaskState backgroundTask;
    EditorBuildModalState buildModalState;
    std::string buildSettingsMessage;
    loadBuildSettings(buildSettings, buildSettingsMessage);
    syncBuildSettingsBuffers(buildSettings);
    const std::string scenePath = isKnownEditorScenePath(buildSettings.selectedScenePath)
        ? buildSettings.selectedScenePath
        : availableEditorScenes().front().second;
    const std::string scenePathOnDisk = std::string(NUT_PROJECT_ROOT) + "/" + scenePath;
    loadSceneData(sceneData, scenePathOnDisk, scenePath);
    buildSettings.selectedScenePath = scenePath;
    if (!buildSettingsMessage.empty()) {
        appendConsoleLine(sceneData, buildSettingsMessage);
    }
    applyScriptExpansionStates(sceneData.roots, g_scriptExpansionStates);
    g_scriptExpansionSceneData = &sceneData;

    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_F11)) {
            ToggleFullscreen();
            forceLayout = true;
        }

        if (sceneData.loaded && IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_S)) {
            saveSceneToDisk(sceneData);
        }

        flushEditorTask(backgroundTask, sceneData);

        BeginDrawing();
        ClearBackground({72, 86, 92, 255});

        rlImGuiBegin();
        drawDesktopPattern();
        float menuHeight = ImGui::GetFrameHeight();
        const EditorUiActions menuActions = drawMainMenu(sceneData);
        if (menuActions.resetLayout) {
            forceLayout = true;
        }
        if (menuActions.saveScene) {
            saveSceneToDisk(sceneData);
        }
        if (menuActions.openBuildModal) {
            buildModalState.openRequested = true;
        }

        float contentTop = 0.0f;
        const EditorUiActions toolbarActions = drawToolbar(menuHeight, sceneData, contentTop);
        if (toolbarActions.saveScene) {
            saveSceneToDisk(sceneData);
        }
        if (toolbarActions.openBuildModal) {
            buildModalState.openRequested = true;
        }
        const std::string requestedScenePath = !toolbarActions.requestedScenePath.empty()
            ? toolbarActions.requestedScenePath
            : menuActions.requestedScenePath;
        if (!requestedScenePath.empty() && requestedScenePath != sceneData.scenePath) {
            if (backgroundTask.running.load()) {
                appendConsoleLine(sceneData, "> Finish the current background task before switching scenes.");
            } else if (sceneData.dirty && !saveSceneToDisk(sceneData)) {
                appendConsoleLine(sceneData, "> Scene switch stopped because Save Scene failed.");
            } else {
                const std::string requestedScenePathOnDisk = toAbsoluteProjectPath(requestedScenePath);
                loadSceneData(sceneData, requestedScenePathOnDisk, requestedScenePath);
                buildSettings.selectedScenePath = requestedScenePath;
                saveBuildSettings(buildSettings);
                applyScriptExpansionStates(sceneData.roots, g_scriptExpansionStates);
                g_scriptExpansionSceneData = &sceneData;
                forceLayout = true;
            }
        }
        EditorLayout layout = calculateResponsiveLayout(contentTop);
        drawHierarchy(layout, forceLayout, sceneData);
        drawViewport(layout, forceLayout, sceneData);
        drawInspector(layout, forceLayout, sceneData, inspectorState);
        drawAssetsConsole(layout, forceLayout, sceneData);
        const EditorUiActions modalActions = drawBuildModal(buildModalState, sceneData, buildSettings, backgroundTask);
        forceLayout = false;

        auto runRequestedTask = [&](EditorTaskKind taskKind) {
            if (backgroundTask.running.load()) {
                appendConsoleLine(sceneData, "> Another background task is already running.");
                return;
            }
            if (!sceneData.loaded) {
                appendConsoleLine(sceneData, "> No scene is currently loaded.");
                return;
            }
            applyBuildSettingsBuffers(buildSettings);
            if (taskKind != EditorTaskKind::RunDesktop && buildSettings.platformioPath.empty()) {
                appendConsoleLine(sceneData, "> Configure the PlatformIO executable path before building.");
                return;
            }
            if ((taskKind == EditorTaskKind::UploadNano || taskKind == EditorTaskKind::BuildAndUploadNano)
                && !isValidUploadPort(buildSettings.uploadPort)) {
                appendConsoleLine(sceneData, "> Configure a valid upload port before uploading.");
                return;
            }
            if (sceneData.dirty && !saveSceneToDisk(sceneData)) {
                appendConsoleLine(sceneData, "> Build flow stopped because Save Scene failed.");
                return;
            }
            sceneData.buildOutputLines.clear();
            startEditorTask(backgroundTask, sceneData, buildSettings, taskKind);
        };

        if (modalActions.compileScene) {
            runRequestedTask(EditorTaskKind::CompileScene);
        } else if (modalActions.buildNano) {
            runRequestedTask(EditorTaskKind::BuildNano);
        } else if (modalActions.uploadNano) {
            runRequestedTask(EditorTaskKind::UploadNano);
        } else if (modalActions.buildAndUploadNano) {
            runRequestedTask(EditorTaskKind::BuildAndUploadNano);
        } else if (modalActions.runDesktop) {
            runRequestedTask(EditorTaskKind::RunDesktop);
        }

        rlImGuiEnd();

        EndDrawing();
    }

    if (backgroundTask.worker.joinable()) {
        backgroundTask.worker.join();
    }

    saveBuildSettings(buildSettings);
    ImGui::SaveIniSettingsToDisk(editorIniPath.c_str());
    rlImGuiShutdown();
    CloseWindow();
    return 0;
}
