#pragma once

#include "../core/Script.h"
#include "../math/Vec3.h"
#include "../scene/Json.h"
#include <memory>

class SpinScript : public nut::Script {
private:
    nut::math::Vec3 m_rotationSpeed;

public:
    explicit SpinScript(const nut::math::Vec3& rotationSpeed);

    void onUpdate(float deltaTime) override;

    static std::unique_ptr<nut::Script> createFromConfig(const nut::Json& config);
};
