#pragma once

#include <stddef.h>

namespace nut {

constexpr size_t NUT_MAX_ROOT_OBJECTS = 1;
constexpr size_t NUT_MAX_CHILDREN_PER_OBJECT = 2;
constexpr size_t NUT_MAX_SCRIPTS_PER_OBJECT = 2;
constexpr size_t NUT_MAX_OWNED_OBJECTS = 3;
constexpr size_t NUT_MAX_OWNED_MESHES = 2;
constexpr size_t NUT_MAX_OWNED_SCRIPTS = 3;
constexpr size_t NUT_MAX_MESHES = 2;
constexpr size_t NUT_MAX_OBJECTS = 3;
constexpr size_t NUT_MAX_SCRIPT_RECORDS = 3;
constexpr size_t NUT_MAX_VERTICES_PER_MESH = 64;
constexpr size_t NUT_MAX_EDGES_PER_MESH = 128;
constexpr size_t NUT_MAX_STRING_TABLE_BYTES = 48;
constexpr size_t NUT_SCRIPT_ARENA_BYTES = 64;

} // namespace nut
