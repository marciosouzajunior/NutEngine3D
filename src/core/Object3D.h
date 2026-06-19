#pragma once

#include "Transform.h"
#include "Mesh.h"
#include <vector>

namespace nut {

// Object3D represents an entity in the 3D scene.
// It HAS a Mesh (what it looks like) and HAS a Transform (where it is).
// It also supports a Scene Graph hierarchy by holding children objects.
class Object3D {
public:
    Transform transform;
    Mesh* mesh; // Pointer to allow sharing the same mesh across multiple objects

    std::vector<Object3D*> children;

    Object3D() : mesh(nullptr) {}

    // Adds a child object to this object.
    // The child's transform parent is set to this object's transform,
    // meaning the child will move, rotate, and scale relative to this object.
    void addChild(Object3D* child) {
        if (child) {
            child->transform.parent = &this->transform;
            children.push_back(child);
        }
    }
};

} // namespace nut
