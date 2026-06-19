#pragma once

#include "Object3D.h"
#include <vector>

namespace nut {

// The World holds the root objects of the scene.
// Rendering starts here and traverses the Scene Graph.
class World {
public:
    std::vector<Object3D*> rootObjects;

    void add(Object3D* obj) {
        if (obj) {
            rootObjects.push_back(obj);
        }
    }
};

} // namespace nut
