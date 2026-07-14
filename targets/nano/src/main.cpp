#include "../../../build/assets/nano/demo_scene_limits.h"
#include "NanoGraphics.h"
#include "NanoWireframeRenderer.h"

#include "../../../build/assets/nano/demo_scene.h"
#include "../../../src/scene/RuntimeScene.h"
#include "../../../src/scene/SceneBinaryLoader.h"
#include "../../../assets/scripts/GameScriptDispatcher.h"
#include <new>
#include <stddef.h>
#ifdef ARDUINO
extern char __heap_start;
extern void* __brkval;
#endif

namespace {

#ifndef NUT_FW_TAG
#define NUT_FW_TAG "default"
#endif

constexpr const char* kFirmwareVersion = "nano-fw-2026-07-12-" NUT_FW_TAG;

#ifndef NUT_JOYSTICK_X_PIN
#define NUT_JOYSTICK_X_PIN A0
#endif

#ifndef NUT_JOYSTICK_Y_PIN
#define NUT_JOYSTICK_Y_PIN A1
#endif

#ifndef NUT_JOYSTICK_BUTTON_PIN
#define NUT_JOYSTICK_BUTTON_PIN 2
#endif

// The OLED is mounted in the 180-degree orientation selected by the display
// adapter, so this joystick's physical left/right direction is reversed too.
#ifndef NUT_JOYSTICK_X_DIRECTION
#define NUT_JOYSTICK_X_DIRECTION -1.0f
#endif

NanoDisplayAdapter g_display;
NanoGraphics g_graphics(&g_display);
nut::RuntimeScene g_scene;
bool g_ready = false;

float readNormalizedAxis(int pin) {
    const int raw = analogRead(pin);
    const long centered = static_cast<long>(raw) - 512L;
    const long magnitude = centered >= 0 ? centered : -centered;
    if (magnitude < 96L) {
        return 0.0f;
    }

    float normalized = static_cast<float>(centered) / 511.0f;
    if (normalized > 1.0f) {
        normalized = 1.0f;
    } else if (normalized < -1.0f) {
        normalized = -1.0f;
    }

    return normalized;
}

nut::InputState readJoystickInput() {
    nut::InputState input;
    input.moveX = readNormalizedAxis(NUT_JOYSTICK_X_PIN) * NUT_JOYSTICK_X_DIRECTION;
    input.moveY = readNormalizedAxis(NUT_JOYSTICK_Y_PIN);
    input.primaryPressed = digitalRead(NUT_JOYSTICK_BUTTON_PIN) == LOW;
    return input;
}

// A union gives multiple names/types to the same physical memory. Its members
// overlap: loaderScratch and renderer start at the same SRAM address, so this
// storage costs only the size of the larger member instead of their sum.
//
// Only one union member may be active at a time. Our firmware follows this
// sequence deliberately:
// 1. setupScene() uses loaderScratch to decode the compiled scene;
// 2. loading finishes and those temporary records are no longer needed;
// 3. placement-new constructs renderer over those same bytes;
// 4. every frame then uses renderer, while loaderScratch stays inactive.
//
// To load another scene later, destroy renderer first, reuse the bytes as
// loaderScratch, and construct renderer again only after loading succeeds.
union NanoPhaseStorage {
    alignas(max_align_t) uint8_t loaderScratch[nut::SceneBinaryLoader::NANO_SCRATCH_BYTES];
    nut::NanoWireframeRenderer renderer;

    NanoPhaseStorage() {}
    ~NanoPhaseStorage() {}
};

NanoPhaseStorage g_phaseStorage;
nut::NanoWireframeRenderer* g_renderer = nullptr;

void logStep(const char* message) {
    Serial.println(message);
}

void setBuiltinLed(bool on) {
    digitalWrite(LED_BUILTIN, on ? HIGH : LOW);
}

void blinkStage(int count, int delayMs) {
    for (int i = 0; i < count; ++i) {
        setBuiltinLed(true);
        delay(delayMs);
        setBuiltinLed(false);
        delay(delayMs);
    }
}

void setupScene() {
    g_display.showStatus("OLED OK", "LOAD SCENE");

    // A future scene reload must end the renderer lifetime before the loader
    // takes ownership of the shared phase storage again.
    if (g_renderer) {
        g_renderer->~NanoWireframeRenderer();
        g_renderer = nullptr;
    }

    logStep("setupScene: load scene");
    if (!nut::SceneBinaryLoader::load(
            demo_scene_data,
            demo_scene_size,
            g_scene,
            g_phaseStorage.loaderScratch,
            sizeof(g_phaseStorage.loaderScratch))) {
        Serial.println("Failed to load demo_scene_data");
        g_display.showStatus("SCENE FAIL", "CHECK SERIAL");
        return;
    }

    // The loader is finished, so construct the renderer over the same bytes.
    // This is placement-new: it initializes the object without heap allocation.
    g_renderer = new (&g_phaseStorage.renderer) nut::NanoWireframeRenderer(&g_graphics);
    g_ready = true;
    logStep("setupScene: ready");
}

void tickEngine() {
    if (!g_ready) {
        return;
    }

    // Nano frame pipeline:
    // 1. measure frame time
    // 2. update the scene and compiled scripts
    // 3. prepare one cached set of 2D projected vertices/edges for the frame
    // 4. replay that cached geometry page by page into the OLED
    g_graphics.beginFrame();
    const float dt = g_graphics.deltaTime();
    g_scene.setInputState(readJoystickInput());
    nut::game::updateScripts(g_scene, dt);
    g_renderer->prepareFrame(g_scene);
    for (int page = 0; page < g_graphics.pageCount(); ++page) {
        // Each pass draws only one 8-pixel-tall OLED page.
        // present() flushes that page immediately over I2C.
        g_graphics.setPage(page);
        g_graphics.clear();
        g_renderer->drawCachedLines(g_scene);
        g_graphics.present();
    }
}

} // namespace

void setup() {
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(NUT_JOYSTICK_BUTTON_PIN, INPUT_PULLUP);
    setBuiltinLed(false);
    blinkStage(1, 120);

    Serial.begin(115200);
    delay(1500);
    logStep("setup: boot");
    Serial.print("setup: fw ");
    Serial.println(kFirmwareVersion);
    blinkStage(2, 120);

    logStep("setup: serial ok");
    g_graphics.begin();
    blinkStage(3, 120);

    const bool oledReady = g_display.ready();
    logStep(oledReady ? "setup: oled ok" : "setup: oled fail");
    if (oledReady) {
        logStep("setup: oled boot draw skipped");
        blinkStage(4, 120);
    }

    setupScene();
    if (g_ready) {
        setBuiltinLed(true);
    }
}

void loop() {
    tickEngine();
}
