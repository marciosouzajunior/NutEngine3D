#include "GameScriptDispatcher.h"
#include "AutoTranslateScript.h"
#include "AutoTranslateWrapScript.h"
#include "ClampPositionScript.h"
#include "GameControllerScript.h"
#include "PlayerMoveScript.h"
#include "PulseScaleScript.h"
#include "SpinScript.h"
#include "TunnelRunScript.h"
#include "WrapPositionScript.h"
#include "../../src/scene/RuntimeScene.h"

namespace nut::game {

void updateScripts(RuntimeScene& scene, float deltaTime) {
    for (size_t i = 0; i < scene.scriptInstanceCount(); ++i) {
        CompiledScriptInstance* instance = scene.scriptInstanceAt(i);
        if (!instance) continue;

        GameObject* object = scene.loadedObjectAt(instance->objectIndex);
        if (!object) continue;
        if (!object->isEnabled()) continue;

        switch (instance->scriptId) {
        case SpinScript::kScriptId:
            SpinScript::update(*instance, *object, scene.inputState(), deltaTime);
            break;
        case AutoTranslateScript::kScriptId:
            AutoTranslateScript::update(*instance, *object, scene.inputState(), deltaTime);
            break;
        case AutoTranslateWrapScript::kScriptId:
            AutoTranslateWrapScript::update(*instance, *object, scene.inputState(), deltaTime);
            break;
        case GameControllerScript::kScriptId:
            GameControllerScript::update(*instance, *object, scene.inputState(), deltaTime);
            break;
        case PlayerMoveScript::kScriptId:
            PlayerMoveScript::update(*instance, *object, scene.inputState(), deltaTime);
            break;
        case ClampPositionScript::kScriptId:
            ClampPositionScript::update(*instance, *object, scene.inputState(), deltaTime);
            break;
        case PulseScaleScript::kScriptId:
            PulseScaleScript::update(*instance, *object, scene.inputState(), deltaTime);
            break;
        case TunnelRunScript::kScriptId:
            TunnelRunScript::update(*instance, scene, scene.inputState(), deltaTime);
            break;
        case WrapPositionScript::kScriptId:
            WrapPositionScript::update(*instance, *object, scene.inputState(), deltaTime);
            break;
        default:
            break;
        }
    }
}

} // namespace nut::game
