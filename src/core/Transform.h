#pragma once

#include "../math/Vec3.h"
#include "../math/Mat4.h"

namespace nut {

// Transform defines where an object is, how it is rotated, and its size.
class Transform {
public:
    math::Vec3 position;
    math::Vec3 rotation; // Euler angles in radians (x, y, z)
    math::Vec3 scale;

    // Pointer to parent transform to build the Scene Graph hierarchy.
    // If null, this transform is at the root of the world.
    Transform* parent;

    Transform() 
        : position(0, 0, 0), 
          rotation(0, 0, 0), 
          scale(1, 1, 1), 
          parent(nullptr) {}

    // Calculate the Local Transformation Matrix.
    // Order: Scale, then Rotate, then Translate.
    // This matrix moves vertices from the object's local space to its parent's space.
    math::Mat4 getLocalMatrix() const {
        math::Mat4 t = math::Mat4::makeTranslation(position.x, position.y, position.z);
        
        math::Mat4 rX = math::Mat4::makeRotationX(rotation.x);
        math::Mat4 rY = math::Mat4::makeRotationY(rotation.y);
        math::Mat4 rZ = math::Mat4::makeRotationZ(rotation.z);
        // Combine rotations: Z * Y * X
        math::Mat4 r = rZ * rY * rX;

        math::Mat4 s = math::Mat4::makeScale(scale.x, scale.y, scale.z);

        return t * r * s;
    }

    // Calculate the World Transformation Matrix.
    // If there is a parent, we multiply the parent's world matrix by our local matrix.
    // This allows nested objects (like a wheel attached to a car) to move together.
    math::Mat4 getWorldMatrix() const {
        math::Mat4 local = getLocalMatrix();
        if (parent) {
            return parent->getWorldMatrix() * local;
        }
        return local;
    }
};

} // namespace nut
