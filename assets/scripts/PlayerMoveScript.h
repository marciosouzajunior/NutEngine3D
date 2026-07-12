#pragma once

#include "../../src/core/Script.h"
#include "../../src/math/Vec3.h"
#include <stdint.h>

class PlayerMoveScript : public nut::Script {
private:
    nut::math::Vec3 m_unitsPerSecond;

public:
    static constexpr uint16_t kScriptId = 3;
    static constexpr const char* kTypeName = "PlayerMoveScript";

    explicit PlayerMoveScript(const nut::math::Vec3& unitsPerSecond);

    void onUpdate(float deltaTime) override;
};
