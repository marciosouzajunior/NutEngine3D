#pragma once

#include "Json.h"
#include "../core/Script.h"
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

namespace nut {

// ScriptRegistry maps script names from scene files to real C++ script objects.
// The IDE can save "SpinScript"; the game registers how to create SpinScript.
class ScriptRegistry {
private:
    // Factory is a type alias.
    // It means: "any callable thing that receives JSON config and returns a new Script".
    //
    // Example:
    // SpinScript::createFromConfig(const Json&) -> std::unique_ptr<Script>
    //
    // unique_ptr means the created script has one clear owner.
    // LoadedScene will store that pointer and keep the script alive.
    using Factory = std::function<std::unique_ptr<Script>(const Json& config)>;

    // Maps script names from .nutscene files to C++ factory functions.
    //
    // Example:
    // "SpinScript" -> SpinScript::createFromConfig
    std::unordered_map<std::string, Factory> m_factories;

public:
    void registerScript(const std::string& typeName, Factory factory);
    std::unique_ptr<Script> create(const std::string& typeName, const Json& config) const;
};

} // namespace nut
