#include "ScriptRegistry.h"

namespace nut {

void ScriptRegistry::registerScript(const std::string& typeName, Factory factory) {
    m_factories[typeName] = factory;
}

std::unique_ptr<Script> ScriptRegistry::create(const std::string& typeName, const Json& config) const {
    auto it = m_factories.find(typeName);
    if (it == m_factories.end()) {
        return nullptr;
    }

    return it->second(config);
}

} // namespace nut
