#pragma once

#include <cstddef>
#include <cstdint>

namespace inventory {
constexpr uint16_t ink = 0x45a2;
constexpr uint16_t dim = 0x2201;
constexpr uint16_t black = 0;
constexpr size_t categoryCount = 5;

struct Command {
    bool triangle;
    uint8_t paint; // 0 black, 1 phosphor green
    int16_t coordinates[6];
};
struct Model { const Command* commands; uint16_t count; };
struct Item {
    const char* id;
    const char* name;
    const char* kind;
    const char* description;
    Model model;
};
struct Category { const char* name; const Item* items; uint16_t count; };

// Coordinates use the device's native 480x272 canvas. Text positions are
// baselines in the bundled monofonto10pt7b font (10 px advance).
class Canvas {
public:
    virtual ~Canvas() = default;
    virtual void line(int x1, int y1, int x2, int y2, uint16_t color) = 0;
    virtual void triangle(int x1, int y1, int x2, int y2, int x3, int y3, uint16_t color) = 0;
    virtual void rectangle(int x, int y, int width, int height, uint16_t color) = 0;
    virtual void text(int x, int baseline, const char* value, uint16_t color) = 0;
};

const Category* category(size_t index);
uint16_t clampSelection(const Category& category, uint16_t selection);
void drawModel(Canvas& canvas, const Model& model, int x, int y, int width, int height);
void render(Canvas& canvas, size_t categoryIndex, uint16_t selection, bool browsing);
// Item list rows as render() draws them, for touch: rows of 26 px from y 67, x 8..211.
constexpr unsigned visibleRows = 6;
constexpr int listTop = 67, listRowHeight = 26, listX = 8, listWidth = 204;
unsigned firstVisible(const Category& category, uint16_t selection);
}
