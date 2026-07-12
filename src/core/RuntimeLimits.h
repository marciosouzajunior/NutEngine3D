#pragma once

#include <stddef.h>

namespace nut {

// These limits define the fixed-size runtime budgets used by the engine.
// The storage model stays the same on every platform; only the capacities change.
#ifdef ARDUINO
#ifndef NUT_CFG_MAX_ROOT_OBJECTS
#define NUT_CFG_MAX_ROOT_OBJECTS 1
#endif
#ifndef NUT_CFG_MAX_CHILDREN_PER_OBJECT
#define NUT_CFG_MAX_CHILDREN_PER_OBJECT 2
#endif
#ifndef NUT_CFG_MAX_SCRIPTS_PER_OBJECT
#define NUT_CFG_MAX_SCRIPTS_PER_OBJECT 2
#endif
#ifndef NUT_CFG_MAX_OWNED_OBJECTS
#define NUT_CFG_MAX_OWNED_OBJECTS 4
#endif
#ifndef NUT_CFG_MAX_OWNED_MESHES
#define NUT_CFG_MAX_OWNED_MESHES 2
#endif
#ifndef NUT_CFG_MAX_OWNED_SCRIPTS
#define NUT_CFG_MAX_OWNED_SCRIPTS 3
#endif
#ifndef NUT_CFG_MAX_MESHES
#define NUT_CFG_MAX_MESHES 2
#endif
#ifndef NUT_CFG_MAX_OBJECTS
#define NUT_CFG_MAX_OBJECTS 4
#endif
#ifndef NUT_CFG_MAX_SCRIPT_RECORDS
#define NUT_CFG_MAX_SCRIPT_RECORDS 3
#endif
#ifndef NUT_CFG_MAX_VERTICES_PER_MESH
#define NUT_CFG_MAX_VERTICES_PER_MESH 64
#endif
#ifndef NUT_CFG_MAX_EDGES_PER_MESH
#define NUT_CFG_MAX_EDGES_PER_MESH 128
#endif
#ifndef NUT_CFG_MAX_STRING_TABLE_BYTES
#define NUT_CFG_MAX_STRING_TABLE_BYTES 48
#endif
#ifndef NUT_CFG_SCRIPT_ARENA_BYTES
#define NUT_CFG_SCRIPT_ARENA_BYTES 64
#endif

constexpr size_t NUT_MAX_ROOT_OBJECTS = NUT_CFG_MAX_ROOT_OBJECTS;
constexpr size_t NUT_MAX_CHILDREN_PER_OBJECT = NUT_CFG_MAX_CHILDREN_PER_OBJECT;
constexpr size_t NUT_MAX_SCRIPTS_PER_OBJECT = NUT_CFG_MAX_SCRIPTS_PER_OBJECT;
constexpr size_t NUT_MAX_OWNED_OBJECTS = NUT_CFG_MAX_OWNED_OBJECTS;
constexpr size_t NUT_MAX_OWNED_MESHES = NUT_CFG_MAX_OWNED_MESHES;
constexpr size_t NUT_MAX_OWNED_SCRIPTS = NUT_CFG_MAX_OWNED_SCRIPTS;
constexpr size_t NUT_MAX_MESHES = NUT_CFG_MAX_MESHES;
constexpr size_t NUT_MAX_OBJECTS = NUT_CFG_MAX_OBJECTS;
constexpr size_t NUT_MAX_SCRIPT_RECORDS = NUT_CFG_MAX_SCRIPT_RECORDS;
constexpr size_t NUT_MAX_VERTICES_PER_MESH = NUT_CFG_MAX_VERTICES_PER_MESH;
constexpr size_t NUT_MAX_EDGES_PER_MESH = NUT_CFG_MAX_EDGES_PER_MESH;
constexpr size_t NUT_MAX_STRING_TABLE_BYTES = NUT_CFG_MAX_STRING_TABLE_BYTES;
constexpr size_t NUT_SCRIPT_ARENA_BYTES = NUT_CFG_SCRIPT_ARENA_BYTES;
#else
constexpr size_t NUT_MAX_ROOT_OBJECTS = 32;
constexpr size_t NUT_MAX_CHILDREN_PER_OBJECT = 16;
constexpr size_t NUT_MAX_SCRIPTS_PER_OBJECT = 8;
constexpr size_t NUT_MAX_OWNED_OBJECTS = 128;
constexpr size_t NUT_MAX_OWNED_MESHES = 64;
constexpr size_t NUT_MAX_OWNED_SCRIPTS = 128;
constexpr size_t NUT_MAX_MESHES = 64;
constexpr size_t NUT_MAX_OBJECTS = 128;
constexpr size_t NUT_MAX_SCRIPT_RECORDS = 128;
constexpr size_t NUT_MAX_VERTICES_PER_MESH = 2048;
constexpr size_t NUT_MAX_EDGES_PER_MESH = 4096;
constexpr size_t NUT_MAX_STRING_TABLE_BYTES = 4096;
constexpr size_t NUT_SCRIPT_ARENA_BYTES = 1024;
#endif

} // namespace nut
