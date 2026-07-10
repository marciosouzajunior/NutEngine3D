#include "DummyScript.h"

#ifndef ARDUINO
DummyScript::DummyScript(std::string note, bool enabled)
    : m_note(std::move(note)), m_enabled(enabled) {}

DummyScript::DummyScript(bool enabled)
    : m_note("Compiled dummy script"), m_enabled(enabled) {}
#else
DummyScript::DummyScript(bool enabled)
    : m_enabled(enabled) {}
#endif

void DummyScript::onUpdate(float deltaTime) {
    (void)deltaTime;

    if (!m_enabled) {
        return;
    }

    // Intentionally empty: this script exists only to exercise editor inspection.
}
