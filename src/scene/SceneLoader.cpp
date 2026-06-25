#include "SceneLoader.h"
#include "../core/ObjLoader.h"
#include <fstream>
#include <iostream>
#include <sstream>

namespace nut {
namespace {

math::Vec3 readVec3(const Json& value, const math::Vec3& defaultValue) {
    if (!value.isArray() || value.size() < 3) {
        return defaultValue;
    }

    return math::Vec3(
        static_cast<float>(value.at(0).asNumber(defaultValue.x)),
        static_cast<float>(value.at(1).asNumber(defaultValue.y)),
        static_cast<float>(value.at(2).asNumber(defaultValue.z))
    );
}

bool readTextFile(const std::string& path, std::string& outText) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return false;
    }

    std::ostringstream stream;
    stream << file.rdbuf();
    outText = stream.str();
    return true;
}

GameObject* loadObject(
    const Json& objectJson,
    LoadedScene& scene,
    const ScriptRegistry& scripts
) {
    std::string name = objectJson.get("name").asString("GameObject");
    GameObject* object = scene.createObject(name);

    object->transform.position = readVec3(objectJson.get("position"), math::Vec3(0, 0, 0));
    object->transform.rotation = readVec3(objectJson.get("rotation"), math::Vec3(0, 0, 0));
    object->transform.scale = readVec3(objectJson.get("scale"), math::Vec3(1, 1, 1));

    std::string meshPath = objectJson.get("mesh").asString("");
    if (!meshPath.empty()) {
        Mesh* mesh = scene.createMesh();
        if (ObjLoader::load(meshPath, *mesh)) {
            object->mesh = mesh;
        } else {
            std::cout << "Could not load mesh: " << meshPath << std::endl;
        }
    }

    const Json& scriptList = objectJson.get("scripts");
    if (scriptList.isArray()) {
        for (size_t i = 0; i < scriptList.size(); ++i) {
            const Json& scriptJson = scriptList.at(i);
            std::string typeName = scriptJson.get("type").asString("");
            if (typeName.empty()) {
                continue;
            }

            std::unique_ptr<Script> script = scripts.create(typeName, scriptJson);
            if (script) {
                scene.attachScript(object, std::move(script));
            } else {
                std::cout << "Unknown script type: " << typeName << std::endl;
            }
        }
    }

    const Json& children = objectJson.get("children");
    if (children.isArray()) {
        for (size_t i = 0; i < children.size(); ++i) {
            GameObject* child = loadObject(children.at(i), scene, scripts);
            object->addChild(child);
        }
    }

    return object;
}

} // namespace

bool SceneLoader::load(const std::string& path, LoadedScene& outScene, const ScriptRegistry& scripts) {
    std::string text;
    if (!readTextFile(path, text)) {
        std::cout << "Could not read scene file: " << path << std::endl;
        return false;
    }

    Json root;
    std::string error;
    if (!Json::parse(text, root, &error)) {
        std::cout << "Could not parse scene file: " << error << std::endl;
        return false;
    }

    if (!root.isObject()) {
        std::cout << "Scene file root must be a JSON object." << std::endl;
        return false;
    }

    outScene.clearLoadedData();

    const Json& camera = root.get("camera");
    if (camera.isObject()) {
        outScene.camera().transform.position = readVec3(camera.get("position"), math::Vec3(0, 0, -5));
        outScene.camera().transform.rotation = readVec3(camera.get("rotation"), math::Vec3(0, 0, 0));
    }

    const Json& objects = root.get("objects");
    if (!objects.isArray()) {
        std::cout << "Scene file must contain an objects array." << std::endl;
        return false;
    }

    for (size_t i = 0; i < objects.size(); ++i) {
        GameObject* object = loadObject(objects.at(i), outScene, scripts);
        outScene.addObject(object);
    }

    return true;
}

} // namespace nut
