#include "Inventory.h"
#include "InventoryData.h"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <cstdlib>

namespace inventory {
const Category* category(size_t index) {
    return index < categoryCount ? &categories[index] : nullptr;
}

uint16_t clampSelection(const Category& group, uint16_t selection) {
    return group.count ? std::min<uint16_t>(selection, group.count - 1) : 0;
}

void drawModel(Canvas& canvas, const Model& model, int x, int y, int width, int height) {
    if (width <= 0 || height <= 0) return;
    // Preserve each source SVG's 200x120 aspect ratio and centre it in the box.
    const int scale = std::min(width * 1000 / 200, height * 1000 / 120);
    x += (width - 200 * scale / 1000) / 2;
    y += (height - 120 * scale / 1000) / 2;
    for (uint16_t i = 0; i < model.count; ++i) {
        const auto& command = model.commands[i];
        int p[6];
        for (unsigned c = 0; c < 6; ++c) {
            p[c] = (c % 2 ? y : x) + command.coordinates[c] * scale / 1000;
        }
        const auto color = command.paint ? ink : black;
        if (command.triangle) canvas.triangle(p[0], p[1], p[2], p[3], p[4], p[5], color);
        else {
            canvas.line(p[0], p[1], p[2], p[3], color);
            // Two-pixel strokes remain legible at the small native art size.
            if (std::abs(p[2] - p[0]) >= std::abs(p[3] - p[1]))
                canvas.line(p[0], p[1] + 1, p[2], p[3] + 1, color);
            else canvas.line(p[0] + 1, p[1], p[2] + 1, p[3], color);
        }
    }
}

namespace {
void description(Canvas& canvas, const char* value) {
    constexpr size_t columns = 23;
    constexpr unsigned lines = 3;
    for (unsigned row = 0; row < lines && *value; ++row) {
        while (*value == ' ') ++value;
        size_t count = std::min(columns, std::strlen(value));
        if (value[count] && value[count] != ' ') {
            while (count && value[count] != ' ') --count;
            if (!count) count = columns;
        }
        char line[columns + 1] = {};
        std::memcpy(line, value, count);
        value += count;
        while (*value == ' ') ++value;
        if (row == lines - 1 && *value) {
            count = std::min(count, columns - 2);
            line[count++] = '.';
            line[count++] = '.';
            line[count] = '\0';
        }
        canvas.text(234, 207 + row * 20, line, ink);
    }
}
}

void render(Canvas& canvas, size_t categoryIndex, uint16_t selection, bool browsing) {
    const auto* group = category(categoryIndex);
    if (!group || !group->count) {
        canvas.text(12, 90, "NO ITEMS", ink);
        return;
    }
    selection = clampSelection(*group, selection);
    constexpr unsigned visibleRows = 6;
    const unsigned start = std::min<unsigned>(selection > 2 ? selection - 2 : 0,
        group->count > visibleRows ? group->count - visibleRows : 0);
    canvas.line(220, 64, 220, 250, dim);
    for (unsigned i = start; i < group->count && i < start + visibleRows; ++i) {
        const int baseline = 85 + (i - start) * 26;
        const bool active = i == selection;
        if (active) canvas.rectangle(8, baseline - 18, 204, 24, browsing ? ink : dim);
        canvas.text(12, baseline, group->items[i].name, active && browsing ? black : ink);
    }
    const auto& selected = group->items[selection];
    canvas.text(234, 78, selected.name, ink);
    canvas.text(234, 97, selected.kind, ink);
    drawModel(canvas, selected.model, 234, 101, 234, 88);
    description(canvas, selected.description);
    char footer[48];
    std::snprintf(footer, sizeof(footer), "%02u/%02u  %s", static_cast<unsigned>(selection + 1), static_cast<unsigned>(group->count),
                  browsing ? "TURN: ITEM  HOLD: BACK" : "CLICK: ITEMS  TURN: CATEGORY");
    canvas.text(10, 265, footer, ink);
}
}
