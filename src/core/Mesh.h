#pragma once

#include "../math/Vec3.h"
#ifdef ARDUINO
#include "FixedVector.h"
#include "NanoRuntimeConfig.h"
#else
#include <vector>
#endif

namespace nut {

// Represents a connection between two vertices
struct Edge {
    int a;
    int b;
    Edge() : a(0), b(0) {}
    Edge(int a, int b) : a(a), b(b) {}
};

// Mesh holds the raw geometry data: vertices (points) and edges (lines).
// These are defined in local coordinates, meaning they are relative to the
// object's own origin, usually around (0,0,0).
//
// Mental model:
// - Mesh = what shape the object has.
// - Transform = where/how that shape appears in the scene.
// - GameObject = the thing that combines Mesh, Transform, and later behavior.
//
// Mesh can be created in code, like createCube(), or imported from a simple
// asset format such as OBJ using ObjLoader.
class Mesh {
public:
#ifdef ARDUINO
    FixedVector<math::Vec3, NUT_MAX_VERTICES_PER_MESH> vertices;
    FixedVector<Edge, NUT_MAX_EDGES_PER_MESH> edges;
#else
    std::vector<math::Vec3> vertices;
    std::vector<Edge> edges;
#endif

    // Factory method to create a simple wireframe cube
    static Mesh createCube() {
        Mesh mesh;
        
        // Vertices of a cube from -1 to 1
        mesh.vertices.push_back(math::Vec3(-1, -1, -1)); // 0
        mesh.vertices.push_back(math::Vec3( 1, -1, -1)); // 1
        mesh.vertices.push_back(math::Vec3( 1,  1, -1)); // 2
        mesh.vertices.push_back(math::Vec3(-1,  1, -1)); // 3
        mesh.vertices.push_back(math::Vec3(-1, -1,  1)); // 4
        mesh.vertices.push_back(math::Vec3( 1, -1,  1)); // 5
        mesh.vertices.push_back(math::Vec3( 1,  1,  1)); // 6
        mesh.vertices.push_back(math::Vec3(-1,  1,  1)); // 7

        // Front face edges
        mesh.edges.push_back(Edge(0, 1));
        mesh.edges.push_back(Edge(1, 2));
        mesh.edges.push_back(Edge(2, 3));
        mesh.edges.push_back(Edge(3, 0));

        // Back face edges
        mesh.edges.push_back(Edge(4, 5));
        mesh.edges.push_back(Edge(5, 6));
        mesh.edges.push_back(Edge(6, 7));
        mesh.edges.push_back(Edge(7, 4));

        // Connecting edges
        mesh.edges.push_back(Edge(0, 4));
        mesh.edges.push_back(Edge(1, 5));
        mesh.edges.push_back(Edge(2, 6));
        mesh.edges.push_back(Edge(3, 7));

        return mesh;
    }
};

} // namespace nut
