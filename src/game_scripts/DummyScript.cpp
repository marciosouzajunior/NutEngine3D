#include "DummyScript.h"

DummyScript::DummyScript(std::string note, bool enabled)
    : m_note(std::move(note)), m_enabled(enabled) {}

void DummyScript::onUpdate(float deltaTime) {
    (void)deltaTime;

    if (!m_enabled) {
        return;
    }

    // Intentionally empty: this script exists only to exercise editor inspection.
}

std::unique_ptr<nut::Script> DummyScript::createFromConfig(const nut::Json& config) {
    const std::string note = config.get("note").asString("Editor-only test script");
    const bool enabled = config.get("enabled").asBool(true);
    return std::make_unique<DummyScript>(note, enabled);
}
