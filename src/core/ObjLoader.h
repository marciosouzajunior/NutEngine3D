#pragma once

#include "Mesh.h"
#include <string>

namespace nut {

// ObjLoader imports a small, static subset of the Wavefront OBJ format.
//
// Supported:
// - v x y z          vertex positions
// - f a b c ...      faces, converted into wireframe edges
// - f a/b/c tokens   texture/normal indices are ignored
//
// Not supported:
// - materials, textures, normals, animation, smoothing groups, or huge models.
//
// This intentionally stays small because NutEngine3D targets tiny devices later.
class ObjLoader {
public:
    static bool load(const std::string& path, Mesh& outMesh);
};

} // namespace nut
