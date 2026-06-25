#pragma once

#include "Graphics.h"
#include "../core/Scene.h"
#include "../math/Vec2.h"

namespace nut {

// WireframeRenderer takes a Scene, computes the camera/object matrices,
// and draws the 3D objects onto a 2D Graphics interface.
class WireframeRenderer {
private:
    Graphics* m_graphics;

    // Recursively renders an object and its children
    void renderObject(GameObject* obj, const math::Mat4& viewProjMatrix) {
        if (!obj) return;

        // 1. Calculate the World Matrix (this object's transformation + its parent's).
        // This matrix says how to place the mesh in the scene for this frame.
        math::Mat4 worldMatrix = obj->transform.getWorldMatrix();

        // 2. Combine the World Matrix with the View/Projection Matrices
        // Model-View-Projection (MVP) matrix:
        math::Mat4 mvpMatrix = viewProjMatrix * worldMatrix;

        // 3. If the object has a mesh, draw its edges
        if (obj->mesh) {
            drawMesh(obj->mesh, mvpMatrix);
        }

        // 4. Render all children (they will compute their own world matrix based on this parent)
        for (GameObject* child : obj->children) {
            renderObject(child, viewProjMatrix);
        }
    }

    // Projects the mesh vertices to 2D and draws the lines
    void drawMesh(Mesh* mesh, const math::Mat4& mvpMatrix) {
        // We will cache the transformed 2D points to avoid re-projecting the same vertex multiple times
        std::vector<math::Vec2> screenPoints(mesh->vertices.size());
        std::vector<bool> pointValid(mesh->vertices.size(), false);

        int halfWidth = m_graphics->width() / 2;
        int halfHeight = m_graphics->height() / 2;

        for (size_t i = 0; i < mesh->vertices.size(); ++i) {
            // Apply Model-View-Projection matrix to the local vertex.
            // This transforms a copy of the mesh position for drawing; it does not
            // modify mesh->vertices.
            math::Vec3 ndc = mvpMatrix * mesh->vertices[i];
            
            // Simple clipping: if z is outside the normalized device coordinates range, don't draw
            // This prevents drawing objects behind the camera.
            // Z in NDC usually goes from -1 to 1 or 0 to 1 depending on the projection matrix.
            // We just do a simple check to ensure it's not behind the camera plane.
            if (ndc.z < 1.0f && ndc.z > -1.0f) {
                // Convert NDC (-1 to 1) to Screen Coordinates
                // X: -1 is left, 1 is right
                // Y: -1 is bottom, 1 is top (so we flip Y for screen coordinates where 0 is top)
                float screenX = (ndc.x + 1.0f) * halfWidth;
                float screenY = (1.0f - ndc.y) * halfHeight;
                
                screenPoints[i] = math::Vec2(screenX, screenY);
                pointValid[i] = true;
            }
        }

        // Draw edges
        for (const Edge& edge : mesh->edges) {
            if (pointValid[edge.a] && pointValid[edge.b]) {
                m_graphics->drawLine(
                    static_cast<int>(screenPoints[edge.a].x),
                    static_cast<int>(screenPoints[edge.a].y),
                    static_cast<int>(screenPoints[edge.b].x),
                    static_cast<int>(screenPoints[edge.b].y)
                );
            }
        }
    }

public:
    WireframeRenderer(Graphics* graphics) : m_graphics(graphics) {}

    // Renders the entire scene from the perspective of its camera.
    void render(Scene& scene) {
        if (!m_graphics) return;

        // Calculate aspect ratio
        float aspect = static_cast<float>(m_graphics->width()) / static_cast<float>(m_graphics->height());

        // 1. Get Camera View Matrix
        math::Mat4 viewMatrix = scene.camera().getViewMatrix();

        // 2. Get Camera Projection Matrix
        math::Mat4 projMatrix = scene.camera().getProjectionMatrix(aspect);

        // Pre-combine Projection and View matrices to save calculations per object
        math::Mat4 viewProjMatrix = projMatrix * viewMatrix;

        // Traverse the scene graph starting from the root objects
        for (GameObject* obj : scene.rootObjects()) {
            renderObject(obj, viewProjMatrix);
        }
    }
};

} // namespace nut
