// PlatformIO compiles sources under targets/nano/src. This bridge selects the
// shared engine implementation and game scripts that become part of this Nano
// firmware.
//
// Script selection is intentionally explicit. When a scene starts using a new
// native script, include its .cpp below. Do not include every script by default:
// each referenced implementation can consume flash even when the active scene
// does not instantiate it. The scene binary stores only script IDs and config;
// it does not contain executable script code.
#include "../../../build/assets/nano/demo_scene_limits.h"

#include "../../../src/scene/RuntimeScene.cpp"
#include "../../../src/scene/SceneBinaryLoader.cpp"
#include "../../../src/core/Scene.cpp"
#include "../../../src/core/Script.cpp"

// Scripts available to scenes shipped in this firmware.
#include "../../../assets/scripts/AutoTranslateScript.cpp"
#include "../../../assets/scripts/AutoTranslateWrapScript.cpp"
#include "../../../assets/scripts/ClampPositionScript.cpp"
#include "../../../assets/scripts/GameControllerScript.cpp"
#include "../../../assets/scripts/PulseScaleScript.cpp"
#include "../../../assets/scripts/SpinScript.cpp"
#include "../../../assets/scripts/PlayerMoveScript.cpp"
#include "../../../assets/scripts/TunnelRunScript.cpp"
#include "../../../assets/scripts/WrapPositionScript.cpp"
#include "../../../assets/scripts/GameScriptDispatcher.cpp"
