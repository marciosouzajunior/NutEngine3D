#pragma once

#include <Arduino.h>
#include <Wire.h>

constexpr uint8_t NUT_OLED_ADDR = 0x3C;
constexpr int NUT_OLED_PHYSICAL_WIDTH = 128;
constexpr int NUT_OLED_PHYSICAL_HEIGHT = 64;
constexpr int NUT_OLED_PAGE_HEIGHT = 8;
constexpr int NUT_OLED_PAGE_COUNT = NUT_OLED_PHYSICAL_HEIGHT / NUT_OLED_PAGE_HEIGHT;

// NutNanoDisplayAdapter is the lowest rendering layer in the Nano target.
//
// Pipeline role:
// - maintain one temporary 128-byte page buffer in RAM
// - rasterize 2D lines into that page buffer
// - send the buffer to the SSD1306 controller over I2C
//
// This is the final step before pixels appear on the OLED. It no longer knows
// anything about scenes, cameras, meshes, or projection; it only knows how to
// write bits into the current display page and flush them to hardware.
class NutNanoDisplayAdapter {
private:
    uint8_t m_pageBuffer[NUT_OLED_PHYSICAL_WIDTH];
    uint8_t m_currentPage;
    bool m_ready;

    static uint8_t lineOutcode(int x, int y, int minY, int maxY) {
        uint8_t code = 0;
        if (x < 0) {
            code |= 1;
        } else if (x >= NUT_OLED_PHYSICAL_WIDTH) {
            code |= 2;
        }
        if (y < minY) {
            code |= 4;
        } else if (y > maxY) {
            code |= 8;
        }
        return code;
    }

    // Clip a line to the current 128x8 page before Bresenham walks it.
    // Without this step, every intersected OLED page repeats the whole line,
    // including pixels that belong to other pages. The clipping math uses only
    // local integers and adds no persistent SRAM cost.
    static bool clipLineToPage(int& x1, int& y1, int& x2, int& y2, int minY, int maxY) {
        uint8_t out1 = lineOutcode(x1, y1, minY, maxY);
        uint8_t out2 = lineOutcode(x2, y2, minY, maxY);

        while (true) {
            if ((out1 | out2) == 0) {
                return true;
            }
            if ((out1 & out2) != 0) {
                return false;
            }

            const uint8_t out = out1 != 0 ? out1 : out2;
            int x = 0;
            int y = 0;

            if ((out & 4) != 0) {
                y = minY;
                x = x1 + (x2 - x1) * (minY - y1) / (y2 - y1);
            } else if ((out & 8) != 0) {
                y = maxY;
                x = x1 + (x2 - x1) * (maxY - y1) / (y2 - y1);
            } else if ((out & 2) != 0) {
                x = NUT_OLED_PHYSICAL_WIDTH - 1;
                y = y1 + (y2 - y1) * (x - x1) / (x2 - x1);
            } else {
                x = 0;
                y = y1 + (y2 - y1) * (0 - x1) / (x2 - x1);
            }

            if (out == out1) {
                x1 = x;
                y1 = y;
                out1 = lineOutcode(x1, y1, minY, maxY);
            } else {
                x2 = x;
                y2 = y;
                out2 = lineOutcode(x2, y2, minY, maxY);
            }
        }
    }

    void sendCommand(uint8_t command) {
        Wire.beginTransmission(NUT_OLED_ADDR);
        Wire.write(0x00);
        Wire.write(command);
        Wire.endTransmission();
    }

    void sendCommandList(const uint8_t* commands, size_t count) {
        for (size_t i = 0; i < count; ++i) {
            sendCommand(commands[i]);
        }
    }

    void setPixel(int x, int y) {
        if (x < 0 || x >= NUT_OLED_PHYSICAL_WIDTH) {
            return;
        }

        const int pageStartY = static_cast<int>(m_currentPage) * NUT_OLED_PAGE_HEIGHT;
        if (y < pageStartY || y >= pageStartY + NUT_OLED_PAGE_HEIGHT) {
            return;
        }

        const uint8_t bit = static_cast<uint8_t>(1u << (y - pageStartY));
        m_pageBuffer[x] |= bit;
    }

public:
    NutNanoDisplayAdapter()
        : m_pageBuffer(), m_currentPage(0), m_ready(false) {}

    // Bring the SSD1306 into page-addressing mode and clear every page once.
    void begin() {
        Wire.begin();
        Wire.setClock(700000L);
        delay(30);

        static const uint8_t initCommands[] = {
            0xAE,
            0xD5, 0x80,
            0xA8, 0x3F,
            0xD3, 0x00,
            0x40,
            0x8D, 0x14,
            0x20, 0x02,
            0xA1,
            0xC8,
            0xDA, 0x12,
            0x81, 0xCF,
            0xD9, 0xF1,
            0xDB, 0x40,
            0xA4,
            0xA6,
            0x2E,
            0xAF
        };

        sendCommandList(initCommands, sizeof(initCommands));
        m_ready = true;

        for (uint8_t page = 0; page < NUT_OLED_PAGE_COUNT; ++page) {
            setPage(page);
            clear();
            present();
        }
    }

    int width() const {
        return NUT_OLED_PHYSICAL_WIDTH;
    }

    int height() const {
        return NUT_OLED_PHYSICAL_HEIGHT;
    }

    int pageCount() const {
        return NUT_OLED_PAGE_COUNT;
    }

    void setPage(uint8_t page) {
        if (page >= NUT_OLED_PAGE_COUNT) {
            return;
        }

        m_currentPage = page;
    }

    uint8_t currentPage() const {
        return m_currentPage;
    }

    void clear() {
        memset(m_pageBuffer, 0, sizeof(m_pageBuffer));
    }

    // Rasterize a 2D line into the current page buffer only.
    // Pixels outside the active 8-row page are ignored here and will be handled
    // when the engine renders the other pages.
    void drawLine(int x1, int y1, int x2, int y2) {
        const int pageStartY = static_cast<int>(m_currentPage) * NUT_OLED_PAGE_HEIGHT;
        const int pageEndY = pageStartY + NUT_OLED_PAGE_HEIGHT - 1;
        const int minX = x1 < x2 ? x1 : x2;
        const int maxX = x1 > x2 ? x1 : x2;
        const int minY = y1 < y2 ? y1 : y2;
        const int maxY = y1 > y2 ? y1 : y2;

        if (maxX < 0 || minX >= NUT_OLED_PHYSICAL_WIDTH) {
            return;
        }
        if (maxY < pageStartY || minY > pageEndY) {
            return;
        }

        if (!clipLineToPage(x1, y1, x2, y2, pageStartY, pageEndY)) {
            return;
        }

        if (y1 == y2 && y1 >= pageStartY && y1 <= pageEndY) {
            int startX = x1 < x2 ? x1 : x2;
            int endX = x1 > x2 ? x1 : x2;
            if (startX < 0) {
                startX = 0;
            }
            if (endX >= NUT_OLED_PHYSICAL_WIDTH) {
                endX = NUT_OLED_PHYSICAL_WIDTH - 1;
            }

            const uint8_t bit = static_cast<uint8_t>(1u << (y1 - pageStartY));
            for (int x = startX; x <= endX; ++x) {
                m_pageBuffer[x] |= bit;
            }
            return;
        }

        int dx = abs(x2 - x1);
        const int sx = x1 < x2 ? 1 : -1;
        int dy = -abs(y2 - y1);
        const int sy = y1 < y2 ? 1 : -1;
        int err = dx + dy;

        while (true) {
            setPixel(x1, y1);
            if (x1 == x2 && y1 == y2) {
                break;
            }

            const int e2 = err * 2;
            if (e2 >= dy) {
                err += dy;
                x1 += sx;
            }
            if (e2 <= dx) {
                err += dx;
                y1 += sy;
            }
        }
    }

    void present() {
        if (!m_ready) {
            return;
        }

        // Flush the current 128-byte page buffer to SSD1306 RAM.
        // After this, that one slice of the frame becomes visible on the OLED.
        sendCommand(static_cast<uint8_t>(0xB0 | m_currentPage));
        sendCommand(0x00);
        sendCommand(0x10);

        for (int offset = 0; offset < NUT_OLED_PHYSICAL_WIDTH; offset += 16) {
            Wire.beginTransmission(NUT_OLED_ADDR);
            Wire.write(0x40);
            for (int i = 0; i < 16; ++i) {
                Wire.write(m_pageBuffer[offset + i]);
            }
            Wire.endTransmission();
        }
    }

    bool ready() const {
        return m_ready;
    }

    void showStatus(const char*, const char* = nullptr) {}

    void drawDiagnosticFrame() {
        if (!m_ready) {
            return;
        }

        for (uint8_t page = 0; page < NUT_OLED_PAGE_COUNT; ++page) {
            setPage(page);
            clear();
            drawLine(0, 0, NUT_OLED_PHYSICAL_WIDTH - 1, 0);
            drawLine(0, NUT_OLED_PHYSICAL_HEIGHT - 1, NUT_OLED_PHYSICAL_WIDTH - 1, NUT_OLED_PHYSICAL_HEIGHT - 1);
            drawLine(0, 0, 0, NUT_OLED_PHYSICAL_HEIGHT - 1);
            drawLine(NUT_OLED_PHYSICAL_WIDTH - 1, 0, NUT_OLED_PHYSICAL_WIDTH - 1, NUT_OLED_PHYSICAL_HEIGHT - 1);
            drawLine(0, 0, NUT_OLED_PHYSICAL_WIDTH - 1, NUT_OLED_PHYSICAL_HEIGHT - 1);
            drawLine(NUT_OLED_PHYSICAL_WIDTH - 1, 0, 0, NUT_OLED_PHYSICAL_HEIGHT - 1);
            drawLine(0, NUT_OLED_PHYSICAL_HEIGHT / 2, NUT_OLED_PHYSICAL_WIDTH - 1, NUT_OLED_PHYSICAL_HEIGHT / 2);
            present();
        }
    }
};
