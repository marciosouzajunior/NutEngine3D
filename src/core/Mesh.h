#pragma once

#include "../math/Vec3.h"
#include <vector>

namespace nut {

// Represents a connection between two vertices
struct Edge {
    int a;
    int b;
    Edge(int a, int b) : a(a), b(b) {}
};

// Mesh holds the raw geometry data: vertices (points) and edges (lines).
// These are defined in "Local Space", meaning they are relative to the object's center (0,0,0).
class Mesh {
public:
    std::vector<math::Vec3> vertices;
    std::vector<Edge> edges;

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
        mesh.edges.emplace_back(0, 1);
        mesh.edges.emplace_back(1, 2);
        mesh.edges.emplace_back(2, 3);
        mesh.edges.emplace_back(3, 0);

        // Back face edges
        mesh.edges.emplace_back(4, 5);
        mesh.edges.emplace_back(5, 6);
        mesh.edges.emplace_back(6, 7);
        mesh.edges.emplace_back(7, 4);

        // Connecting edges
        mesh.edges.emplace_back(0, 4);
        mesh.edges.emplace_back(1, 5);
        mesh.edges.emplace_back(2, 6);
        mesh.edges.emplace_back(3, 7);

        return mesh;
    }
};

} // namespace nut
