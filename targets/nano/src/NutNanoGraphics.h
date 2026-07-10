#pragma once

#include "../../../src/render/Graphics.h"
#include "NutNanoDisplayAdapter.h"

#include <Arduino.h>

// NutNanoGraphics is the Nano-side implementation of the generic Graphics API.
//
// Pipeline role:
// - expose the OLED dimensions and per-frame delta time to the engine
// - forward 2D drawLine()/clear()/present() commands to the display adapter
// - add Nano-specific control for page-based rendering
//
// This layer does not decide what to draw. It sits between the Nano renderer
// and the low-level SSD1306 adapter.
class NutNanoGraphics : public nut::Graphics {
private:
    NutNanoDisplayAdapter* m_display;
    unsigned long m_lastFrameMs;
    float m_deltaTime;

public:
    explicit NutNanoGraphics(NutNanoDisplayAdapter* display)
        : m_display(display), m_lastFrameMs(0), m_deltaTime(1.0f / 30.0f) {}

    // Initialize the display side of the pipeline before frames begin.
    void begin() {
        if (m_display) {
            m_display->begin();
        }
        m_lastFrameMs = millis();
    }

    void beginFrame() override {
        // Nano uses beginFrame mainly to measure frame time.
        // The actual page flushing happens later in present().
        const unsigned long now = millis();
        const unsigned long elapsed = now - m_lastFrameMs;
        m_lastFrameMs = now;
        m_deltaTime = elapsed > 0 ? static_cast<float>(elapsed) / 1000.0f : (1.0f / 30.0f);
        // Clamp large frame gaps so a slow OLED update does not turn one frame
        // into a huge rotation jump that makes motion look "broken" on Nano.
        if (m_deltaTime > 0.12f) {
            m_deltaTime = 0.12f;
        }
    }

    int width() const override {
        return m_display ? m_display->width() : 0;
    }

    int height() const override {
        return m_display ? m_display->height() : 0;
    }

    int pageCount() const {
        return m_display ? m_display->pageCount() : 0;
    }

    // Select which 8-pixel-tall OLED page the next clear/draw/present calls target.
    void setPage(int page) {
        if (m_display && page >= 0) {
            m_display->setPage(static_cast<uint8_t>(page));
        }
    }

    void clear() override {
        if (m_display) {
            m_display->clear();
        }
    }

    void drawLine(int x1, int y1, int x2, int y2) override {
        if (m_display) {
            m_display->drawLine(x1, y1, x2, y2);
        }
    }

    void present() override {
        if (m_display) {
            m_display->present();
        }
    }

    bool shouldClose() const override {
        return false;
    }

    float deltaTime() const override {
        return m_deltaTime;
    }
};
