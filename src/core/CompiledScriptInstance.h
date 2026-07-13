#pragma once

#include <stddef.h>
#include <stdint.h>

namespace nut {

// Runtime record for one occurrence of a script attached to a scene object.
// It is not the script behavior itself: GameScriptDispatcher uses scriptId to
// choose the game-side module that knows how to execute it.
//
// For example, one TunnelRunScript instance contains:
// - objectIndex: index of the root "Game" object that owns the behavior
// - scriptId: TunnelRunScript::kScriptId
// - configWords: baseSpeed, speedStep, and collisionRadius compiled from JSON
// - stateWord/stateFlags: changing values such as difficulty and game-over
//
// Configuration normally stays unchanged for the scene lifetime. State is
// modified while the game runs. The engine deliberately treats both areas as
// opaque storage; only modules in assets/scripts assign meaning to them. This
// keeps the core independent from gameplay and each instance fixed at 20 bytes.
struct CompiledScriptInstance {
    static constexpr size_t CONFIG_CAPACITY = 12;

    uint16_t objectIndex = 0; // Target object in RuntimeScene's flat object list.
    uint16_t scriptId = 0; // Selects the game-side script implementation.
    uint32_t configWords[CONFIG_CAPACITY / sizeof(uint32_t)] {}; // Compiled properties, up to 12 bytes.
    uint16_t stateWord = 0; // General-purpose mutable 16-bit state.
    uint8_t stateFlags = 0; // Mutable boolean flags packed into bits.
    uint8_t stateByte = 0; // Additional mutable 8-bit state when needed.

    void setConfigByte(size_t index, uint8_t value) {
        if (index >= CONFIG_CAPACITY) return;

        const size_t wordIndex = index / sizeof(uint32_t);
        const uint8_t shift = static_cast<uint8_t>((index % sizeof(uint32_t)) * 8);
        const uint32_t mask = static_cast<uint32_t>(0xFF) << shift;
        configWords[wordIndex] = (configWords[wordIndex] & ~mask)
            | (static_cast<uint32_t>(value) << shift);
    }

    uint8_t configByte(size_t index) const {
        if (index >= CONFIG_CAPACITY) return 0;

        const size_t wordIndex = index / sizeof(uint32_t);
        const uint8_t shift = static_cast<uint8_t>((index % sizeof(uint32_t)) * 8);
        return static_cast<uint8_t>((configWords[wordIndex] >> shift) & 0xFF);
    }

    // Config words preserve the serialized IEEE-754 bits. Convert without
    // pointer casts, avoiding strict-aliasing problems on desktop and AVR.
    float configFloat(size_t index) const {
        if (index >= CONFIG_CAPACITY / sizeof(uint32_t)) {
            return 0.0f;
        }

        union FloatBits {
            uint32_t bits;
            float value;
        } decoded {};
        decoded.bits = configWords[index];
        return decoded.value;
    }
};

static_assert(sizeof(CompiledScriptInstance) == 20, "Compiled script instances must stay compact.");

} // namespace nut
