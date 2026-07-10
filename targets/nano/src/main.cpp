#include "NutNanoGraphics.h"
#include "NanoWireframeRenderer.h"

#include "../../../build/assets/nano/demo_scene.h"
#include "../../../src/scene/RuntimeScene.h"
#include "../../../src/scene/SceneBinaryLoader.h"

namespace {

constexpr const char* kFirmwareVersion = "nano-fw-2026-07-10-b24";

NutNanoDisplayAdapter g_display;
NutNanoGraphics g_graphics(&g_display);
nut::NanoWireframeRenderer g_renderer(&g_graphics);
nut::RuntimeScene g_scene;
bool g_ready = false;

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

    logStep("setupScene: load scene");
    if (!nut::SceneBinaryLoader::load(demo_scene_data, demo_scene_size, g_scene)) {
        Serial.println("Failed to load demo_scene_data");
        g_display.showStatus("SCENE FAIL", "CHECK SERIAL");
        return;
    }

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
    g_scene.onUpdate(dt);
    g_renderer.prepareFrame(g_scene);

    for (int page = 0; page < g_graphics.pageCount(); ++page) {
        // Each pass draws only one 8-pixel-tall OLED page.
        // present() flushes that page immediately over I2C.
        g_graphics.setPage(page);
        g_graphics.clear();
        g_renderer.drawCachedLines(g_scene);
        g_graphics.present();
    }
}

} // namespace

void setup() {
    pinMode(LED_BUILTIN, OUTPUT);
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
