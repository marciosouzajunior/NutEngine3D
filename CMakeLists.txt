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
    URL https://github.com/raysan5/raylib/archive/refs/tags/5.0.tar.gz
)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(raylib)

# Source files
file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.h")

# Create executable
add_executable(NutEngine3D ${SOURCES})

# Link Raylib
target_link_libraries(NutEngine3D PRIVATE raylib)
target_include_directories(NutEngine3D PRIVATE src)
