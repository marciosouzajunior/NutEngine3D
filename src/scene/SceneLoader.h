#pragma once

#include "LoadedScene.h"
#include "ScriptRegistry.h"
#include <string>

namespace nut {

// SceneLoader builds a LoadedScene from a JSON .nutscene file.
class SceneLoader {
public:
    static bool load(const std::string& path, LoadedScene& outScene, const ScriptRegistry& scripts);
};

} // namespace nut
