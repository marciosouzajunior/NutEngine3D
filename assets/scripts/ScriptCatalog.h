#pragma once

#include "AutoTranslateScript.h"
#include "ClampPositionScript.h"
#include "GameControllerScript.h"
#include "PlayerMoveScript.h"
#include "PulseScaleScript.h"
#include "SpinScript.h"
#include "TunnelRunScript.h"
#include "WrapPositionScript.h"

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

    if (name == AutoTranslateScript::kTypeName) {
        return AutoTranslateScript::kScriptId;
    }

    if (name == GameControllerScript::kTypeName) {
        return GameControllerScript::kScriptId;
    }

    if (name == PlayerMoveScript::kTypeName) {
        return PlayerMoveScript::kScriptId;
    }

    if (name == ClampPositionScript::kTypeName) {
        return ClampPositionScript::kScriptId;
    }

    if (name == PulseScaleScript::kTypeName) {
        return PulseScaleScript::kScriptId;
    }

    if (name == TunnelRunScript::kTypeName) {
        return TunnelRunScript::kScriptId;
    }

    if (name == WrapPositionScript::kTypeName) {
        return WrapPositionScript::kScriptId;
    }

    return 0;
}
#endif

inline const char* scriptNameFromId(uint16_t scriptId) {
    switch (scriptId) {
    case SpinScript::kScriptId:
        return SpinScript::kTypeName;
    case AutoTranslateScript::kScriptId:
        return AutoTranslateScript::kTypeName;
    case GameControllerScript::kScriptId:
        return GameControllerScript::kTypeName;
    case PlayerMoveScript::kScriptId:
        return PlayerMoveScript::kTypeName;
    case ClampPositionScript::kScriptId:
        return ClampPositionScript::kTypeName;
    case PulseScaleScript::kScriptId:
        return PulseScaleScript::kTypeName;
    case TunnelRunScript::kScriptId:
        return TunnelRunScript::kTypeName;
    case WrapPositionScript::kScriptId:
        return WrapPositionScript::kTypeName;
    default:
        return "";
    }
}

} // namespace nut::game
