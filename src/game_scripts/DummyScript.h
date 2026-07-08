#pragma once

#include "../core/Script.h"
#include "../scene/Json.h"
#include <memory>
#include <string>

class DummyScript : public nut::Script {
private:
    std::string m_note;
    bool m_enabled;

public:
    DummyScript(std::string note, bool enabled);

    void onUpdate(float deltaTime) override;

    static std::unique_ptr<nut::Script> createFromConfig(const nut::Json& config);
};
