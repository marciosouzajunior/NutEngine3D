cmake_minimum_required(VERSION 3.14)
project(NutEngine3D)

# Resolve problemas com CMake 4.x tentando baixar bibliotecas antigas
set(CMAKE_POLICY_VERSION_MINIMUM "3.5" CACHE STRING "" FORCE)
if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

set(CMAKE_CXX_STANDARD 17)

# Fetch Raylib
include(FetchContent)
FetchContent_Declare(
    raylib
    URL https://github.com/raysan5/raylib/archive/refs/tags/6.0.tar.gz
)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(raylib)

# Dear ImGui + rlImGui for the scene editor UI
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
FetchContent_MakeAvailable(imgui rlimgui)
FetchContent_GetProperties(imgui_color_text_edit)
if(NOT imgui_color_text_edit_POPULATED)
    FetchContent_Populate(imgui_color_text_edit)
endif()

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

# Source files
# CONFIGURE_DEPENDS asks CMake to refresh this list when new source files are added.
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "src/*.cpp" "src/*.h")

# Create executable
add_executable(NutEngine3D ${SOURCES})

# Link Raylib
target_link_libraries(NutEngine3D PRIVATE raylib)
target_include_directories(NutEngine3D PRIVATE src)

# Scene editor prototype
add_executable(NutSceneEditor
    scene_editor/main.cpp
    src/scene/Json.cpp
)
target_link_libraries(NutSceneEditor PRIVATE raylib imgui rlimgui)
target_link_libraries(NutSceneEditor PRIVATE imgui_color_text_edit)
target_compile_definitions(NutSceneEditor PRIVATE NUT_PROJECT_ROOT="${CMAKE_SOURCE_DIR}")
