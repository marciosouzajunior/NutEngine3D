#include <raylib.h>
#include <rlImGui.h>
#include <imgui.h>

#include <string>

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
    // Motif/CDE windows commonly use muted gray or teal title areas with black text.
    // ImGui shares the normal text color with title text, so keep title bars light enough to read.
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

void drawDesktopPattern() {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImDrawList* drawList = ImGui::GetBackgroundDrawList();
    ImVec2 min = viewport->Pos;
    ImVec2 max(min.x + viewport->Size.x, min.y + viewport->Size.y);

    drawList->AddRectFilled(min, max, motif::DarkDesktop);

    // Keep the desktop texture cheap. Dense speckles can exceed ImGui's 16-bit draw index limit.
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
    // Force only the startup/reset layout. After that, ImGui keeps panels movable and resizable.
    ImGuiCond condition = forceLayout ? ImGuiCond_Always : ImGuiCond_FirstUseEver;
    ImGui::SetNextWindowPos(pos, condition);
    ImGui::SetNextWindowSize(size, condition);
}

bool drawMainMenu() {
    bool resetLayout = false;
    const char* scenePath = "assets/scenes/demo.nutscene";

    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            ImGui::MenuItem("Open Scene...");
            ImGui::MenuItem("Save");
            ImGui::MenuItem("Save As...");
            ImGui::Separator();
            ImGui::MenuItem("Exit");
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Edit")) {
            ImGui::MenuItem("Undo");
            ImGui::MenuItem("Redo");
            ImGui::Separator();
            ImGui::MenuItem("Duplicate Object");
            ImGui::MenuItem("Delete Object");
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
            ImGui::MenuItem("Add Empty Object");
            ImGui::MenuItem("Add Mesh Object");
            ImGui::MenuItem("Validate Scene");
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

float drawToolbar(float menuHeight) {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    const float toolbarHeight = 44.0f;

    // The first frame can report WorkPos before the main menu adjusts it, so reserve menu space explicitly.
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

    ImGui::Button("Open", ImVec2(72, 0));
    ImGui::SameLine();
    ImGui::Button("Save", ImVec2(72, 0));
    ImGui::SameLine();
    ImGui::Button("Add Object", ImVec2(112, 0));
    ImGui::SameLine();
    ImGui::Button("Add Script", ImVec2(104, 0));

    const char* status = "UI mockup only - no scene editing wired yet";
    ImVec2 statusSize = ImGui::CalcTextSize(status);
    float statusX = ImGui::GetWindowWidth() - statusSize.x - 16.0f;
    if (statusX > ImGui::GetCursorPosX()) {
        ImGui::SameLine(statusX);
        ImGui::TextDisabled("%s", status);
    }
    ImGui::End();

    return viewport->Pos.y + menuHeight + toolbarHeight;
}

void drawHierarchy(const EditorLayout& layout, bool forceLayout) {
    applyPanelLayout(layout.hierarchyPos, layout.hierarchySize, forceLayout);

    ImGui::Begin("Scene Hierarchy");
    ImGui::TextDisabled("Scene: Demo");
    ImGui::Separator();

    ImGuiTreeNodeFlags rootFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Selected | ImGuiTreeNodeFlags_OpenOnArrow;
    if (ImGui::TreeNodeEx("OrbitPivot", rootFlags)) {
        ImGui::TreeNodeEx("CenterCube", ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen);
        ImGui::TreeNodeEx("OrbitingSphere", ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen);
        ImGui::TreePop();
    }

    ImGui::End();
}

void drawViewport(const EditorLayout& layout, bool forceLayout) {
    applyPanelLayout(layout.viewportPos, layout.viewportSize, forceLayout);

    ImGui::Begin("Viewport");
    ImGui::TextDisabled("Wireframe preview placeholder");
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

void drawInspector(const EditorLayout& layout, bool forceLayout) {
    applyPanelLayout(layout.inspectorPos, layout.inspectorSize, forceLayout);

    static char name[64] = "OrbitPivot";
    static char mesh[128] = "(empty)";
    static float position[3] = {0.0f, 0.0f, 0.0f};
    static float rotation[3] = {0.0f, 0.0f, 0.0f};
    static float scale[3] = {1.0f, 1.0f, 1.0f};

    ImGui::Begin("Inspector");
    ImGui::TextDisabled("Selected GameObject");
    ImGui::Separator();
    ImGui::BeginDisabled();
    ImGui::InputText("Name", name, sizeof(name), ImGuiInputTextFlags_ReadOnly);
    ImGui::DragFloat3("Position", position, 0.01f, 0.0f, 0.0f, "%.2f");
    ImGui::DragFloat3("Rotation", rotation, 0.01f, 0.0f, 0.0f, "%.2f");
    ImGui::DragFloat3("Scale", scale, 0.01f, 0.0f, 0.0f, "%.2f");
    ImGui::InputText("Mesh", mesh, sizeof(mesh), ImGuiInputTextFlags_ReadOnly);
    ImGui::EndDisabled();

    ImGui::Spacing();
    ImGui::SeparatorText("Scripts");
    ImGui::TextUnformatted("SpinScript");
    ImGui::TextDisabled("rotationSpeed: [0.6, 1.2, 0]");
    ImGui::End();
}

void drawAssetsConsole(const EditorLayout& layout, bool forceLayout) {
    applyPanelLayout(layout.assetsPos, layout.assetsSize, forceLayout);

    ImGui::Begin("Assets / Console");
    if (ImGui::BeginTabBar("AssetsConsoleTabs")) {
        if (ImGui::BeginTabItem("Assets")) {
            ImGui::BulletText("models/cube.obj");
            ImGui::BulletText("models/sphere.obj");
            ImGui::BulletText("scenes/demo.nutscene");
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Console")) {
            ImGui::TextUnformatted("> Loaded scene: assets/scenes/demo.nutscene");
            ImGui::TextUnformatted("> Registered script: SpinScript");
            ImGui::TextUnformatted("> UI containers only; editing is not wired yet.");
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
    static const std::string editorIniPath = std::string(NUT_PROJECT_ROOT) + "/scene_editor/nutscene_editor.ini";
    const bool hasSavedLayout = FileExists(editorIniPath.c_str());

    // ImGui stores window positions and sizes in an .ini file.
    // Use a project-specific file so the scene editor keeps its own layout.
    io.IniFilename = editorIniPath.c_str();
    const std::string editorFontPath = std::string(NUT_PROJECT_ROOT) + "/scene_editor/assets/fonts/ProggyClean.ttf";
    if (FileExists(editorFontPath.c_str())) {
        io.Fonts->AddFontFromFileTTF(editorFontPath.c_str(), 16.0f);
    } else {
        io.Fonts->AddFontDefault();
    }
    applyMotifStyle();
    rlImGuiEndInitImGui();

    bool forceLayout = !hasSavedLayout;

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
        if (drawMainMenu()) {
            forceLayout = true;
        }

        float contentTop = drawToolbar(menuHeight);
        EditorLayout layout = calculateResponsiveLayout(contentTop);
        drawHierarchy(layout, forceLayout);
        drawViewport(layout, forceLayout);
        drawInspector(layout, forceLayout);
        drawAssetsConsole(layout, forceLayout);
        forceLayout = false;
        rlImGuiEnd();

        EndDrawing();
    }

    rlImGuiShutdown();
    CloseWindow();
    return 0;
}
