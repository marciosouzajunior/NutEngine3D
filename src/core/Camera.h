#pragma once

#include "Transform.h"
#include "../math/MathUtils.h"

namespace nut {

// The Camera represents the viewpoint from which the scene is rendered.
class Camera {
public:
    Transform transform; // Position and rotation of the camera in the world
    
    float fov;       // Field of view in radians (typically 60-90 degrees)
    float nearPlane; // Minimum distance rendered
    float farPlane;  // Maximum distance rendered

    Camera() : fov(math::degToRad(60.0f)), nearPlane(0.1f), farPlane(100.0f) {
        // Camera starts a bit back so it can see the origin
        transform.position = math::Vec3(0, 0, -5); 
    }

    // Calculates the View Matrix.
    // The View Matrix moves the entire world in the opposite direction of the camera.
    // E.g., if the camera moves +5 in Z, the world moves -5 in Z.
    // Because we don't scale the camera, the inverse is just inverted rotation and inverted translation.
    math::Mat4 getViewMatrix() const {
        // Invert translation:
        math::Mat4 tInv = math::Mat4::makeTranslation(-transform.position.x, -transform.position.y, -transform.position.z);
        
        // Invert rotation (for Euler angles, we negate the angles and reverse the multiplication order)
        math::Mat4 rXInv = math::Mat4::makeRotationX(-transform.rotation.x);
        math::Mat4 rYInv = math::Mat4::makeRotationY(-transform.rotation.y);
        math::Mat4 rZInv = math::Mat4::makeRotationZ(-transform.rotation.z);
        
        math::Mat4 rInv = rXInv * rYInv * rZInv;

        // The View Matrix is: InverseRotation * InverseTranslation
        return rInv * tInv;
    }

    // Calculates the Perspective Projection Matrix.
    // 'aspect' is the width / height of the screen.
    // This matrix adds perspective (objects further away appear smaller).
    math::Mat4 getProjectionMatrix(float aspect) const {
        return math::Mat4::makePerspective(fov, aspect, nearPlane, farPlane);
    }
};

} // namespace nut
