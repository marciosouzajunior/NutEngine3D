#include <raylib.h>
#include <rlImGui.h>
#include <imgui.h>
#include <imgui_internal.h>

#include <cstdint>
#include <TextEditor.h>

#include "../../src/scene/Json.h"

#include <algorithm>
#include <cstring>
#include <cstdlib>
#include <fstream>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

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
    std::string type;
    std::vector<std::pair<std::string, std::string>> properties;
    bool expanded = true;
};

struct EditorObjectData {
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
    std::string scenePath;
    std::string sceneName = "Untitled";
    EditorCameraData camera;
    std::vector<std::unique_ptr<EditorObjectData>> roots;
    std::vector<std::string> assetPaths;
    std::vector<std::string> consoleLines;
    EditorSelectionKind selectionKind = EditorSelectionKind::None;
    EditorObjectData* selectedObject = nullptr;
    bool loaded = false;
    bool dirty = false;
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

EditorScriptData parseScript(const nut::Json& scriptJson) {
    EditorScriptData script;
    script.type = scriptJson.get("type").asString("UnknownScript");

    if (!scriptJson.isObject()) {
        return script;
    }

    for (const auto& entry : scriptJson.objectValue) {
        if (entry.first == "type") {
            continue;
        }
        script.properties.push_back({entry.first, formatJsonValue(entry.second)});
    }

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
    appendConsoleLine(sceneData, "> Inspector edits are local only; Save is not wired yet.");
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

bool drawMainMenu(const EditorSceneData& sceneData) {
    bool resetLayout = false;
    const char* scenePath = sceneData.scenePath.c_str();

    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            ImGui::MenuItem("Open Scene...", nullptr, false, false);
            ImGui::MenuItem("Save", nullptr, false, false);
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
                resetLayout = true;
            }
            ImGui::MenuItem("Fullscreen", "F11");
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Scene")) {
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

    return resetLayout;
}

float drawToolbar(float menuHeight, const EditorSceneData& sceneData) {
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
    ImGui::SameLine();
    ImGui::Button("Save", ImVec2(72, 0));
    ImGui::SameLine();
    ImGui::Button("Add Object", ImVec2(112, 0));
    ImGui::SameLine();
    ImGui::Button("Add Script", ImVec2(104, 0));
    ImGui::EndDisabled();

    std::string status;
    if (!sceneData.loaded) {
        status = "Scene load failed - showing diagnostics in Console";
    } else if (sceneData.dirty) {
        status = "Unsaved local changes in memory";
    } else {
        status = "Scene loaded from JSON";
    }
    ImVec2 statusSize = ImGui::CalcTextSize(status.c_str());
    float statusX = ImGui::GetWindowWidth() - statusSize.x - 16.0f;
    if (statusX > ImGui::GetCursorPosX()) {
        ImGui::SameLine(statusX);
        ImGui::TextDisabled("%s", status.c_str());
    }
    ImGui::End();

    return viewport->Pos.y + menuHeight + toolbarHeight;
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
    for (const auto& property : script.properties) {
        propertyLabelWidth = std::max(propertyLabelWidth, ImGui::CalcTextSize(property.first.c_str()).x);
    }
    propertyLabelWidth += 16.0f;

    float cardHeight = style.WindowPadding.y * 2.0f + ImGui::GetFrameHeight();
    if (script.expanded) {
        const float propertyRows = script.properties.empty() ? 1.0f : static_cast<float>(script.properties.size());
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
        if (script.properties.empty()) {
            ImGui::TextDisabled("No properties.");
        } else {
            for (const auto& property : script.properties) {
                drawScriptPropertyRow(property.first.c_str(), property.second, propertyLabelWidth);
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
        appendConsoleLine(sceneData, "> Scene modified in memory. Save is not implemented yet.");
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
            ImGui::TextDisabled("Local changes pending. Save is not implemented yet.");
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
        ImGui::TextDisabled("Local changes pending. Save is not implemented yet.");
    }

    ImGui::End();
}

void drawAssetsConsole(const EditorLayout& layout, bool forceLayout, const EditorSceneData& sceneData) {
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
    const std::string scenePath = "assets/scenes/demo.nutscene";
    const std::string scenePathOnDisk = std::string(NUT_PROJECT_ROOT) + "/" + scenePath;
    loadSceneData(sceneData, scenePathOnDisk, scenePath);
    applyScriptExpansionStates(sceneData.roots, g_scriptExpansionStates);
    g_scriptExpansionSceneData = &sceneData;

    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_F11)) {
            ToggleFullscreen();
            forceLayout = true;
        }

        BeginDrawing();
        ClearBackground({72, 86, 92, 255});

        rlImGuiBegin();
        drawDesktopPattern();
        float menuHeight = ImGui::GetFrameHeight();
        if (drawMainMenu(sceneData)) {
            forceLayout = true;
        }

        float contentTop = drawToolbar(menuHeight, sceneData);
        EditorLayout layout = calculateResponsiveLayout(contentTop);
        drawHierarchy(layout, forceLayout, sceneData);
        drawViewport(layout, forceLayout, sceneData);
        drawInspector(layout, forceLayout, sceneData, inspectorState);
        drawAssetsConsole(layout, forceLayout, sceneData);
        forceLayout = false;
        rlImGuiEnd();

        EndDrawing();
    }

    ImGui::SaveIniSettingsToDisk(editorIniPath.c_str());
    rlImGuiShutdown();
    CloseWindow();
    return 0;
}
