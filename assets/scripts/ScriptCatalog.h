#pragma once

#include "DummyScript.h"
#include "PlayerMoveScript.h"
#include "SpinScript.h"

#ifndef ARDUINO
#include <string>
#endif

namespace nut::scripts {

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
    default:
        return "";
    }
}

} // namespace nut::scripts
