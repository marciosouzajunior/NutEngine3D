#pragma once

#include "Graphics.h"
#include "../core/Scene.h"
#ifdef ARDUINO
#include "../core/RuntimeLimits.h"
#endif
#include "../math/Vec2.h"

namespace nut {

// WireframeRenderer is the generic desktop/native wireframe pipeline.
//
// Pipeline role:
// 1. Read the active Scene and its Camera.
// 2. Build the view-projection matrix for the current frame.
// 3. Walk through the scene graph and build an MVP matrix per object.
// 4. Project each 3D mesh vertex to 2D screen coordinates.
// 5. Draw the mesh edges through Graphics.
//
// It is a straightforward reference pipeline: easy to reason about, but not as
// aggressively specialized as the Nano renderer.
class WireframeRenderer {
private:
    Graphics* m_graphics;

    // Render one object subtree.
    //
    // This stage still works in 3D object space:
    // - each object computes its world matrix
    // - that world matrix is combined with the scene's camera matrices
    // - the resulting MVP matrix is then passed to the mesh stage
    void renderObject(GameObject* obj, const math::Mat4& viewProjMatrix) {
        if (!obj) return;

        // 1. Calculate the World Matrix (this object's transformation + its parent's).
        // This matrix says how to place the mesh in the scene for this frame.
        math::Mat4 worldMatrix = obj->transform.getWorldMatrix();

        // 2. Combine the World Matrix with the View/Projection Matrices
        // Model-View-Projection (MVP) matrix:
        math::Mat4 mvpMatrix = viewProjMatrix * worldMatrix;

        // 3. If the object has a mesh, draw its edges
        if (obj->isEnabled() && obj->mesh) {
            drawMesh(obj->mesh, mvpMatrix);
        }

        // 4. Render all children (they will compute their own world matrix based on this parent)
        for (size_t i = 0; i < obj->childObjectCount(); ++i) {
            GameObject* child = obj->childObjectAt(i);
            renderObject(child, viewProjMatrix);
        }
    }

    // Convert one mesh from 3D local vertices to 2D screen-space lines.
    //
    // This is the stage where the final image really starts to appear:
    // - the mesh vertices are transformed by the MVP matrix
    // - valid vertices are projected to screen coordinates
    // - edges reuse those cached 2D points and emit drawLine() calls
    void drawMesh(Mesh* mesh, const math::Mat4& mvpMatrix) {
        // First we project each 3D vertex to screen space and store the 2D result.
        // Then the edge pass reuses those cached points instead of projecting the
        // same vertex again for every connected edge.
#ifdef ARDUINO
        math::Vec2 screenPoints[NUT_MAX_VERTICES_PER_MESH];
        bool pointValid[NUT_MAX_VERTICES_PER_MESH] = {};
        if (mesh->vertices.size() > NUT_MAX_VERTICES_PER_MESH) {
            return;
        }
#else
        std::vector<math::Vec2> screenPoints(mesh->vertices.size());
        std::vector<bool> pointValid(mesh->vertices.size(), false);
#endif

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

    // Entry point for one complete frame on the generic/native pipeline.
    //
    // By the time this function returns, all visible wireframe edges for the
    // scene have been converted into 2D drawLine() calls on the Graphics backend.
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
        for (size_t i = 0; i < scene.rootObjectCount(); ++i) {
            GameObject* obj = scene.rootObjectAt(i);
            renderObject(obj, viewProjMatrix);
        }
    }
};

} // namespace nut
