cmake_minimum_required(VERSION 3.14)
project(NutSceneCompiler)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(NutSceneCompiler
    main.cpp
    ../../src/scene/Json.cpp
    ../../src/core/ObjLoader.cpp
)

target_include_directories(NutSceneCompiler PRIVATE
    ../../src
)
