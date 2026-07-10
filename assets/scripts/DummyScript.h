#pragma once

#include "../../src/core/Script.h"
#include <stddef.h>
#include <stdint.h>
#ifndef ARDUINO
#include <string>
#endif

class DummyScript : public nut::Script {
private:
#ifndef ARDUINO
    std::string m_note;
#endif
    bool m_enabled;

public:
    static constexpr uint16_t kScriptId = 2;
    static constexpr const char* kTypeName = "DummyScript";

#ifndef ARDUINO
    DummyScript(std::string note, bool enabled);
    explicit DummyScript(bool enabled);
#else
    explicit DummyScript(bool enabled);
#endif

    void onUpdate(float deltaTime) override;
};
