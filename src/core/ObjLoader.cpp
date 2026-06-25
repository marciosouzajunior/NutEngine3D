#include "ObjLoader.h"
#include <fstream>
#include <sstream>
#include <unordered_set>

namespace nut {
namespace {

int parseObjIndex(const std::string& token, int vertexCount) {
    // OBJ face tokens can look like "3", "3/1", "3/1/2", or "3//2".
    // We only need the first number because this wireframe renderer uses positions.
    size_t slashPos = token.find('/');
    std::string indexText = token.substr(0, slashPos);
    if (indexText.empty()) {
        return -1;
    }

    int objIndex = std::stoi(indexText);

    // Positive OBJ indices are 1-based. Negative indices are relative to the
    // current end of the vertex list. Both are converted to 0-based C++ indices.
    if (objIndex > 0) {
        return objIndex - 1;
    }

    if (objIndex < 0) {
        return vertexCount + objIndex;
    }

    return -1;
}

long long makeEdgeKey(int a, int b) {
    int minIndex = a < b ? a : b;
    int maxIndex = a < b ? b : a;
    return (static_cast<long long>(minIndex) << 32) | static_cast<unsigned int>(maxIndex);
}

void addUniqueEdge(Mesh& mesh, std::unordered_set<long long>& edgeKeys, int a, int b) {
    if (a < 0 || b < 0 || a == b) {
        return;
    }

    long long key = makeEdgeKey(a, b);
    if (edgeKeys.insert(key).second) {
        mesh.edges.emplace_back(a, b);
    }
}

} // namespace

bool ObjLoader::load(const std::string& path, Mesh& outMesh) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return false;
    }

    Mesh mesh;
    std::unordered_set<long long> edgeKeys;
    std::string line;

    while (std::getline(file, line)) {
        std::istringstream stream(line);
        std::string type;
        stream >> type;

        if (type == "v") {
            float x = 0.0f;
            float y = 0.0f;
            float z = 0.0f;

            if (stream >> x >> y >> z) {
                mesh.vertices.emplace_back(x, y, z);
            }
        } else if (type == "f") {
            std::vector<int> faceIndices;
            std::string token;

            while (stream >> token) {
                int index = parseObjIndex(token, static_cast<int>(mesh.vertices.size()));
                if (index >= 0 && index < static_cast<int>(mesh.vertices.size())) {
                    faceIndices.push_back(index);
                }
            }

            // Convert the face outline to wireframe edges.
            // A quad "f 1 2 3 4" becomes edges 1-2, 2-3, 3-4, 4-1.
            for (size_t i = 0; i < faceIndices.size(); ++i) {
                int a = faceIndices[i];
                int b = faceIndices[(i + 1) % faceIndices.size()];
                addUniqueEdge(mesh, edgeKeys, a, b);
            }
        }
    }

    outMesh = mesh;
    return true;
}

} // namespace nut
