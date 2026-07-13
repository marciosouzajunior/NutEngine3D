#include "GameScriptDispatcher.h"
#include "DummyScript.h"
#include "PlayerMoveScript.h"
#include "SpinScript.h"
#include "TunnelRunScript.h"
#include "../../src/scene/RuntimeScene.h"

namespace nut::game {

void updateScripts(RuntimeScene& scene, float deltaTime) {
    for (size_t i = 0; i < scene.scriptInstanceCount(); ++i) {
        CompiledScriptInstance* instance = scene.scriptInstanceAt(i);
        if (!instance) continue;

        GameObject* object = scene.loadedObjectAt(instance->objectIndex);
        if (!object) continue;

        switch (instance->scriptId) {
        case SpinScript::kScriptId:
            SpinScript::update(*instance, *object, scene.inputState(), deltaTime);
            break;
        case DummyScript::kScriptId:
            DummyScript::update(*instance, *object, scene.inputState(), deltaTime);
            break;
        case PlayerMoveScript::kScriptId:
            PlayerMoveScript::update(*instance, *object, scene.inputState(), deltaTime);
            break;
        case TunnelRunScript::kScriptId:
            TunnelRunScript::update(*instance, *object, scene.inputState(), deltaTime);
            break;
        default:
            break;
        }
    }
}

} // namespace nut::game
