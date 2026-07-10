#pragma once

#ifdef ARDUINO

#include "../../../src/render/Graphics.h"
#include "../../../src/core/FixedVector.h"
#include "../../../src/scene/RuntimeScene.h"
#include "../../../src/math/Vec2.h"

#include <Arduino.h>
#include <avr/pgmspace.h>
#include <stdint.h>
#include <math.h>

namespace nut {

// NanoWireframeRenderer is the Nano-specific wireframe pipeline.
//
// Pipeline role:
// 1. Walk the loaded scene and compute world-space object transforms.
// 2. Read compact mesh data directly from the compiled scene blob in flash.
// 3. Project vertices once and cache their 2D screen positions for the frame.
// 4. Reuse that cache while drawing OLED pages, instead of re-projecting every page.
// 5. Emit 2D line commands through Graphics for the active page.
//
// The key difference from the desktop renderer is that this version is shaped
// around Nano constraints: tiny RAM, page-based OLED output, and compiled scene
// data stored in flash instead of fully materialized meshes in memory.
class NanoWireframeRenderer {
private:
    struct RotationTrig {
        float cx;
        float sx;
        float cy;
        float sy;
        float cz;
        float sz;
    };

    struct CachedVertex {
        int8_t x;
        int8_t y;
        uint8_t visible;
    };

    struct CachedObjectMesh {
        uint8_t vertexStart;
        uint8_t vertexCount;
        uint16_t edgeBaseOffset;
        uint16_t edgeCount;
        uint8_t sharedOutcode;
    };

    // Targeted Nano budget: enough for the current demo and similar small scenes,
    // while still leaving room for the rest of the runtime.
    static constexpr size_t kMaxCachedVertices = 64;

    Graphics* m_graphics;
    uint16_t m_lastAttemptedEdges = 0;
    uint16_t m_lastVisibleEdges = 0;
    uint16_t m_lastOffscreenRejectedEdges = 0;
    FixedVector<CachedVertex, kMaxCachedVertices> m_cachedVertices;
    FixedVector<CachedObjectMesh, NUT_MAX_OWNED_OBJECTS> m_cachedObjects;

    static int8_t clampToI8(float value) {
        if (value < -127.0f) {
            return static_cast<int8_t>(-127);
        }
        if (value > 127.0f) {
            return static_cast<int8_t>(127);
        }
        return static_cast<int8_t>(value);
    }

    uint8_t computeOutcode(float x, float y) const {
        const int width = m_graphics ? m_graphics->width() : 0;
        const int height = m_graphics ? m_graphics->height() : 0;
        uint8_t outcode = 0;
        if (x < 0.0f) {
            outcode |= 0x01;
        } else if (x >= static_cast<float>(width)) {
            outcode |= 0x02;
        }
        if (y < 0.0f) {
            outcode |= 0x04;
        } else if (y >= static_cast<float>(height)) {
            outcode |= 0x08;
        }
        return outcode;
    }

    bool isLineCompletelyOffscreen(const CachedVertex& a, const CachedVertex& b) const {
        const int width = m_graphics ? m_graphics->width() : 0;
        const int height = m_graphics ? m_graphics->height() : 0;
        if (width <= 0 || height <= 0) {
            return true;
        }

        if (a.x < 0 && b.x < 0) {
            return true;
        }
        if (a.x >= width && b.x >= width) {
            return true;
        }
        if (a.y < 0 && b.y < 0) {
            return true;
        }
        if (a.y >= height && b.y >= height) {
            return true;
        }

        return false;
    }

    static uint8_t readU8(const uint8_t* data, size_t offset) {
        return pgm_read_byte(data + offset);
    }

    static uint16_t readU16(const uint8_t* data, size_t offset) {
        return static_cast<uint16_t>(readU8(data, offset))
            | (static_cast<uint16_t>(readU8(data, offset + 1)) << 8);
    }

    static float readF32(const uint8_t* data, size_t offset) {
        union {
            uint32_t bits;
            float number;
        } convert {};

        convert.bits = static_cast<uint32_t>(readU8(data, offset))
                     | (static_cast<uint32_t>(readU8(data, offset + 1)) << 8)
                     | (static_cast<uint32_t>(readU8(data, offset + 2)) << 16)
                     | (static_cast<uint32_t>(readU8(data, offset + 3)) << 24);
        return convert.number;
    }

    static RotationTrig buildRotationTrig(const math::Vec3& rotation) {
        RotationTrig trig {
            cos(rotation.x),
            sin(rotation.x),
            cos(rotation.y),
            sin(rotation.y),
            cos(rotation.z),
            sin(rotation.z)
        };
        return trig;
    }

    static math::Vec3 rotateXYZ(const math::Vec3& point, const RotationTrig& trig) {
        math::Vec3 result = point;

        const float y1 = result.y * trig.cx - result.z * trig.sx;
        const float z1 = result.y * trig.sx + result.z * trig.cx;
        result.y = y1;
        result.z = z1;

        const float x2 = result.x * trig.cy + result.z * trig.sy;
        const float z2 = -result.x * trig.sy + result.z * trig.cy;
        result.x = x2;
        result.z = z2;

        const float x3 = result.x * trig.cz - result.y * trig.sz;
        const float y3 = result.x * trig.sz + result.y * trig.cz;
        result.x = x3;
        result.y = y3;

        return result;
    }

    bool projectPoint(
        const math::Vec3& worldPoint,
        const math::Vec3& cameraPosition,
        int halfWidth,
        int halfHeight,
        math::Vec2& outPoint
    ) const {
        // Perspective projection from 3D world space into 2D screen space.
        // If this returns false, the point is outside the useful view range.
        const float depth = worldPoint.z - cameraPosition.z;
        if (depth <= 0.2f || depth > 80.0f) {
            return false;
        }

        const float focal = static_cast<float>(halfWidth);
        outPoint.x = static_cast<float>(halfWidth) + (worldPoint.x - cameraPosition.x) * focal / depth;
        outPoint.y = static_cast<float>(halfHeight) - (worldPoint.y - cameraPosition.y) * focal / depth;
        return true;
    }

    bool projectVertex(
        const uint8_t* sceneData,
        size_t vertexOffset,
        const GameObject* object,
        const math::Vec3& worldPosition,
        const RotationTrig& worldTrig,
        const math::Vec3& cameraPosition,
        int halfWidth,
        int halfHeight,
        math::Vec2& outPoint
    ) const {
        // Read one 3D vertex from the compiled scene blob in flash,
        // apply object scale/rotation/translation, then project to screen space.
        math::Vec3 vertex(
            readF32(sceneData, vertexOffset) * object->transform.scale.x,
            readF32(sceneData, vertexOffset + 4) * object->transform.scale.y,
            readF32(sceneData, vertexOffset + 8) * object->transform.scale.z
        );
        vertex = rotateXYZ(vertex, worldTrig);
        vertex += worldPosition;
        return projectPoint(vertex, cameraPosition, halfWidth, halfHeight, outPoint);
    }

    uint16_t countVisibleEdges(
        const uint8_t* sceneData,
        uint16_t edgeBaseOffset,
        uint16_t edgeCount,
        uint8_t vertexStart,
        uint8_t vertexCount
    ) const {
        uint16_t visibleEdges = 0;

        for (uint16_t edgeIndex = 0; edgeIndex < edgeCount; ++edgeIndex) {
            const size_t edgeOffset = static_cast<size_t>(edgeBaseOffset) + static_cast<size_t>(edgeIndex) * 4;
            const uint16_t a = readU16(sceneData, edgeOffset);
            const uint16_t b = readU16(sceneData, edgeOffset + 2);
            if (a >= vertexCount || b >= vertexCount) {
                continue;
            }

            const CachedVertex& v0 = m_cachedVertices[static_cast<size_t>(vertexStart) + a];
            const CachedVertex& v1 = m_cachedVertices[static_cast<size_t>(vertexStart) + b];
            if (v0.visible && v1.visible) {
                ++visibleEdges;
            }
        }

        return visibleEdges;
    }

    void cacheBinaryMesh(
        const RuntimeScene& scene,
        const GameObject* object,
        int16_t meshIndex,
        const math::Vec3& worldPosition,
        const RotationTrig& worldTrig,
        const math::Vec3& cameraPosition
    ) {
        // Frame-preparation stage for one mesh:
        // - read vertices from flash
        // - project each vertex once
        // - store the 2D result in the cache
        // - remember where the mesh edges live in the binary blob
        uint16_t geometryOffset = 0;
        uint16_t vertexCount = 0;
        uint16_t edgeCount = 0;
        if (!scene.getBinaryMeshInfo(meshIndex, geometryOffset, vertexCount, edgeCount)) {
            return;
        }

        const uint8_t* sceneData = scene.binarySceneData();
        if (!sceneData) {
            return;
        }

        if (m_cachedVertices.size() + vertexCount > kMaxCachedVertices) {
            return;
        }

        const size_t meshBaseOffset = static_cast<size_t>(scene.binaryMeshDataOffset()) + geometryOffset;
        const size_t vertexBytes = static_cast<size_t>(vertexCount) * 12;
        const uint16_t edgeBaseOffset = static_cast<uint16_t>(meshBaseOffset + vertexBytes);
        const int halfWidth = m_graphics->width() / 2;
        const int halfHeight = m_graphics->height() / 2;

        const uint8_t vertexStart = static_cast<uint8_t>(m_cachedVertices.size());
        bool allVerticesProjected = true;
        uint8_t sharedOutcode = 0x0F;
        for (uint16_t vertexIndex = 0; vertexIndex < vertexCount; ++vertexIndex) {
            math::Vec2 projected;
            const bool visible = projectVertex(
                sceneData,
                meshBaseOffset + static_cast<size_t>(vertexIndex) * 12,
                object,
                worldPosition,
                worldTrig,
                cameraPosition,
                halfWidth,
                halfHeight,
                projected
            );

            if (visible) {
                sharedOutcode &= computeOutcode(projected.x, projected.y);
            } else {
                allVerticesProjected = false;
                sharedOutcode = 0;
            }

            m_cachedVertices.push_back(CachedVertex {
                visible ? clampToI8(projected.x) : static_cast<int8_t>(0),
                visible ? clampToI8(projected.y) : static_cast<int8_t>(0),
                static_cast<uint8_t>(visible ? 1 : 0)
            });
        }

        m_cachedObjects.push_back(CachedObjectMesh {
            vertexStart,
            static_cast<uint8_t>(vertexCount),
            edgeBaseOffset,
            edgeCount,
            allVerticesProjected ? sharedOutcode : static_cast<uint8_t>(0)
        });

        m_lastAttemptedEdges = static_cast<uint16_t>(m_lastAttemptedEdges + edgeCount);
        m_lastVisibleEdges = static_cast<uint16_t>(
            m_lastVisibleEdges + countVisibleEdges(sceneData, edgeBaseOffset, edgeCount, vertexStart, static_cast<uint8_t>(vertexCount))
        );
    }

    void cacheObject(
        const RuntimeScene& scene,
        GameObject* obj,
        const math::Vec3& parentPosition,
        const math::Vec3& parentRotation,
        const math::Vec3& cameraPosition
    ) {
        // Traverse one object subtree, accumulate parent transforms,
        // and prepare caches for every mesh that belongs to this subtree.
        if (!obj) {
            return;
        }
        const RotationTrig parentTrig = buildRotationTrig(parentRotation);
        const math::Vec3 localPosition = rotateXYZ(obj->transform.position, parentTrig);
        const math::Vec3 worldPosition = parentPosition + localPosition;
        const math::Vec3 worldRotation = parentRotation + obj->transform.rotation;
        const RotationTrig worldTrig = buildRotationTrig(worldRotation);

        if (obj->meshIndex() >= 0) {
            cacheBinaryMesh(scene, obj, obj->meshIndex(), worldPosition, worldTrig, cameraPosition);
        }

        for (GameObject* child : obj->children) {
            cacheObject(scene, child, worldPosition, worldRotation, cameraPosition);
        }
    }

public:
    explicit NanoWireframeRenderer(Graphics* graphics)
        : m_graphics(graphics)
        , m_lastAttemptedEdges(0)
        , m_lastVisibleEdges(0)
        , m_lastOffscreenRejectedEdges(0)
        , m_cachedVertices()
        , m_cachedObjects() {}

    // Build all per-frame caches before page drawing begins.
    //
    // After this step, the expensive 3D work is done for the frame:
    // the page loop can focus on replaying cached lines into the OLED pages.
    void prepareFrame(const RuntimeScene& scene) {
        if (!m_graphics || !scene.binarySceneData()) {
            return;
        }
        m_lastAttemptedEdges = 0;
        m_lastVisibleEdges = 0;
        m_lastOffscreenRejectedEdges = 0;
        m_cachedVertices.clear();
        m_cachedObjects.clear();

        const math::Vec3 cameraPosition = scene.camera().transform.position;
        const math::Vec3 zero(0.0f, 0.0f, 0.0f);

        for (GameObject* obj : scene.rootObjects()) {
            cacheObject(scene, obj, zero, zero, cameraPosition);
        }
    }

    // Replay cached geometry into the currently selected OLED page.
    //
    // This function is called once per page:
    // - lines outside the current page are ignored by the display backend
    // - lines fully off-screen are rejected here before reaching the OLED adapter
    void drawCachedLines(const RuntimeScene& scene) {
        if (!m_graphics || !scene.binarySceneData()) {
            return;
        }

        const uint8_t* sceneData = scene.binarySceneData();
        for (const CachedObjectMesh& cachedObject : m_cachedObjects) {
            if (cachedObject.sharedOutcode != 0) {
                m_lastOffscreenRejectedEdges = static_cast<uint16_t>(m_lastOffscreenRejectedEdges + cachedObject.edgeCount);
                continue;
            }
            for (uint16_t edgeIndex = 0; edgeIndex < cachedObject.edgeCount; ++edgeIndex) {
                const size_t edgeOffset = static_cast<size_t>(cachedObject.edgeBaseOffset) + static_cast<size_t>(edgeIndex) * 4;
                const uint16_t a = readU16(sceneData, edgeOffset);
                const uint16_t b = readU16(sceneData, edgeOffset + 2);
                if (a >= cachedObject.vertexCount || b >= cachedObject.vertexCount) {
                    continue;
                }

                const CachedVertex& v0 = m_cachedVertices[static_cast<size_t>(cachedObject.vertexStart) + a];
                const CachedVertex& v1 = m_cachedVertices[static_cast<size_t>(cachedObject.vertexStart) + b];
                if (!v0.visible || !v1.visible) {
                    continue;
                }
                if (isLineCompletelyOffscreen(v0, v1)) {
                    ++m_lastOffscreenRejectedEdges;
                    continue;
                }

                m_graphics->drawLine(v0.x, v0.y, v1.x, v1.y);
            }
        }
    }

    uint16_t lastAttemptedEdges() const {
        return m_lastAttemptedEdges;
    }

    uint16_t lastDrawnEdges() const {
        return static_cast<uint16_t>(m_lastVisibleEdges - m_lastOffscreenRejectedEdges);
    }

    uint16_t lastOffscreenRejectedEdges() const {
        return m_lastOffscreenRejectedEdges;
    }
};

} // namespace nut

#endif
