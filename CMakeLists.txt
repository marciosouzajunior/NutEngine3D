cmake_minimum_required(VERSION 3.14)
project(NutEngine3D)

# Resolve problemas com CMake 4.x tentando baixar bibliotecas antigas
set(CMAKE_POLICY_VERSION_MINIMUM "3.5" CACHE STRING "" FORCE)
if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

set(CMAKE_CXX_STANDARD 17)
option(NUT_BUILD_SCENE_EDITOR "Build the NutSceneEditor desktop tool" OFF)

# Fetch Raylib
include(FetchContent)
FetchContent_Declare(
    raylib
    URL https://github.com/raysan5/raylib/archive/refs/tags/6.0.tar.gz
)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(raylib)

# Shared runtime code used by the desktop simulator and other native tools.
add_library(NutEngineRuntime STATIC
    src/core/ObjLoader.cpp
    src/core/Scene.cpp
    src/core/SceneManager.cpp
    src/core/Script.cpp
    assets/scripts/AutoTranslateScript.cpp
    assets/scripts/AutoTranslateWrapScript.cpp
    assets/scripts/ClampPositionScript.cpp
    assets/scripts/GameControllerScript.cpp
    assets/scripts/PulseScaleScript.cpp
    assets/scripts/SpinScript.cpp
    assets/scripts/PlayerMoveScript.cpp
    assets/scripts/TunnelRunScript.cpp
    assets/scripts/WrapPositionScript.cpp
    assets/scripts/GameScriptDispatcher.cpp
    src/scene/RuntimeScene.cpp
    src/scene/SceneBinaryLoader.cpp
)
target_include_directories(NutEngineRuntime PUBLIC src assets ${CMAKE_SOURCE_DIR})

# Desktop simulator target
add_executable(NutEngine3D
    targets/desktop/main.cpp
)

target_include_directories(NutEngine3D PRIVATE targets/desktop)
target_link_libraries(NutEngine3D PRIVATE NutEngineRuntime raylib)

if(NUT_BUILD_SCENE_EDITOR)
    # Dear ImGui + rlImGui are only needed for the scene editor UI.
    FetchContent_Declare(
        imgui
        URL https://github.com/ocornut/imgui/archive/refs/heads/master.zip
    )
    FetchContent_Declare(
        rlimgui
        URL https://github.com/raylib-extras/rlImGui/archive/refs/heads/main.zip
    )
    FetchContent_Declare(
        imgui_color_text_edit
        URL https://github.com/BalazsJako/ImGuiColorTextEdit/archive/refs/heads/master.zip
    )
    FetchContent_MakeAvailable(imgui rlimgui imgui_color_text_edit)

    # Keep the script preview usable inside the scene editor:
    # allow horizontal scroll only when needed instead of forcing it visible.
    set(NUT_TEXT_EDITOR_SOURCE "${imgui_color_text_edit_SOURCE_DIR}/TextEditor.cpp")
    file(READ "${NUT_TEXT_EDITOR_SOURCE}" NUT_TEXT_EDITOR_CPP)
    string(REPLACE
        "ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_AlwaysHorizontalScrollbar | ImGuiWindowFlags_NoMove"
        "ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoMove"
        NUT_TEXT_EDITOR_CPP
        "${NUT_TEXT_EDITOR_CPP}"
    )
    file(WRITE "${NUT_TEXT_EDITOR_SOURCE}" "${NUT_TEXT_EDITOR_CPP}")

    add_library(imgui STATIC
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    )
    target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR})

    add_library(rlimgui STATIC
        ${rlimgui_SOURCE_DIR}/rlImGui.cpp
    )
    target_include_directories(rlimgui PUBLIC ${rlimgui_SOURCE_DIR})
    target_link_libraries(rlimgui PUBLIC imgui raylib)

    add_library(imgui_color_text_edit STATIC
        ${imgui_color_text_edit_SOURCE_DIR}/TextEditor.cpp
    )
    target_include_directories(imgui_color_text_edit PUBLIC
        ${imgui_color_text_edit_SOURCE_DIR}
        ${imgui_SOURCE_DIR}
    )
    target_link_libraries(imgui_color_text_edit PUBLIC imgui)

    add_executable(NutSceneEditor
        tools/scene_editor/src/main.cpp
        tools/scene_editor/src/EditorBuildSettings.cpp
        tools/scene_editor/src/EditorSceneCatalog.cpp
        tools/scene_editor/src/EditorUtils.cpp
        tools/scene_compiler/ScenePipeline.cpp
        src/scene/Json.cpp
        src/core/ObjLoader.cpp
    )
    target_include_directories(NutSceneEditor PRIVATE src ${CMAKE_SOURCE_DIR})
    target_link_libraries(NutSceneEditor PRIVATE raylib imgui rlimgui imgui_color_text_edit)
    target_compile_definitions(NutSceneEditor PRIVATE NUT_PROJECT_ROOT="${CMAKE_SOURCE_DIR}")
endif()
