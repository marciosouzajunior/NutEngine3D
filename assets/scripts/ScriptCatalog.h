#pragma once

#include "DummyScript.h"
#include "PlayerMoveScript.h"
#include "SpinScript.h"
#include "TunnelRunScript.h"

#ifndef ARDUINO
#include <string>
#endif

namespace nut::game {

#ifndef ARDUINO
// Tooling compiles script type names from .nutscene into compact numeric ids.
inline uint16_t scriptIdFromName(const std::string& name) {
    if (name == SpinScript::kTypeName) {
        return SpinScript::kScriptId;
    }

    if (name == DummyScript::kTypeName) {
        return DummyScript::kScriptId;
    }

    if (name == PlayerMoveScript::kTypeName) {
        return PlayerMoveScript::kScriptId;
    }

    if (name == TunnelRunScript::kTypeName) {
        return TunnelRunScript::kScriptId;
    }

    return 0;
}
#endif

inline const char* scriptNameFromId(uint16_t scriptId) {
    switch (scriptId) {
    case SpinScript::kScriptId:
        return SpinScript::kTypeName;
    case DummyScript::kScriptId:
        return DummyScript::kTypeName;
    case PlayerMoveScript::kScriptId:
        return PlayerMoveScript::kTypeName;
    case TunnelRunScript::kScriptId:
        return TunnelRunScript::kTypeName;
    default:
        return "";
    }
}

} // namespace nut::game
