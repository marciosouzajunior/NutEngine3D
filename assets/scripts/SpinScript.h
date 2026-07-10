#pragma once

#include "../../src/core/Script.h"
#include "../../src/math/Vec3.h"
#include <stddef.h>
#include <stdint.h>

class SpinScript : public nut::Script {
private:
    nut::math::Vec3 m_rotationSpeed;

public:
    static constexpr uint16_t kScriptId = 1;
    static constexpr const char* kTypeName = "SpinScript";

    explicit SpinScript(const nut::math::Vec3& rotationSpeed);

    void onUpdate(float deltaTime) override;
};
