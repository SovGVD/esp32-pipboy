#include "Inventory.h"
#include "InventoryData.h"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cmath>
#include <cstdint>

namespace inventory {
const Category* category(size_t index) {
    return index < categoryCount ? &categories[index] : nullptr;
}

uint16_t clampSelection(const Category& group, uint16_t selection) {
    return group.count ? std::min<uint16_t>(selection, group.count - 1) : 0;
}

namespace {
constexpr uint16_t noFace = 0xFFFF;
constexpr unsigned maxVertices = 1024, maxFaces = 1024;
constexpr int depthUnitsPerPixel = 16;
constexpr int depthBias = depthUnitsPerPixel * 3 / 2; // lines on a surface still pass
constexpr int16_t emptyDepth = INT16_MIN;

// Static scratch space: the firmware draws one model per frame.
int16_t depth[maxModelWidth * maxModelHeight];
uint8_t lit[(maxModelWidth * maxModelHeight + 7) / 8];
float screenX[maxVertices], screenY[maxVertices], screenZ[maxVertices];
bool facing[maxFaces];

void fillTriangle(int width, int height, unsigned a, unsigned b, unsigned c) {
    const float ax = screenX[a], ay = screenY[a], bx = screenX[b], by = screenY[b], cx = screenX[c], cy = screenY[c];
    const float area = (bx - ax) * (cy - ay) - (by - ay) * (cx - ax);
    if (std::fabs(area) < 1e-6f) return;
    const int left = std::max(0, static_cast<int>(std::floor(std::min({ax, bx, cx}))));
    const int right = std::min(width - 1, static_cast<int>(std::ceil(std::max({ax, bx, cx}))));
    const int top = std::max(0, static_cast<int>(std::floor(std::min({ay, by, cy}))));
    const int bottom = std::min(height - 1, static_cast<int>(std::ceil(std::max({ay, by, cy}))));
    // A small tolerance makes neighbouring triangles overlap instead of leaving
    // a one-pixel crack that a hidden line could show through.
    const float tolerance = -0.02f * std::fabs(area);
    for (int y = top; y <= bottom; ++y) {
        const float py = y + 0.5f;
        for (int x = left; x <= right; ++x) {
            const float px = x + 0.5f;
            float wa = (bx - px) * (cy - py) - (by - py) * (cx - px);
            float wb = (cx - px) * (ay - py) - (cy - py) * (ax - px);
            float wc = (ax - px) * (by - py) - (ay - py) * (bx - px);
            if (area < 0) { wa = -wa; wb = -wb; wc = -wc; }
            if (wa < tolerance || wb < tolerance || wc < tolerance) continue;
            const float z = (wa * screenZ[a] + wb * screenZ[b] + wc * screenZ[c]) / std::fabs(area);
            int16_t& stored = depth[y * width + x];
            if (z > stored) stored = static_cast<int16_t>(z);
        }
    }
}

void drawLine(int width, int height, unsigned a, unsigned b) {
    const float dx = screenX[b] - screenX[a], dy = screenY[b] - screenY[a];
    const int steps = std::max(1, static_cast<int>(std::ceil(std::max(std::fabs(dx), std::fabs(dy)))));
    for (int i = 0; i <= steps; ++i) {
        const float t = static_cast<float>(i) / steps;
        const int x = static_cast<int>(std::floor(screenX[a] + dx * t));
        const int y = static_cast<int>(std::floor(screenY[a] + dy * t));
        if (x < 0 || y < 0 || x >= width || y >= height) continue;
        const float z = screenZ[a] + (screenZ[b] - screenZ[a]) * t;
        const int index = y * width + x;
        if (z + depthBias >= depth[index]) lit[index / 8] |= static_cast<uint8_t>(1 << (index % 8));
    }
}
}

void drawModel(Canvas& canvas, const Mesh& model, int x, int y, int width, int height, float yaw) {
    if (width <= 0 || height <= 0 || !model.vertexCount || model.vertexCount > maxVertices ||
        model.halfWidth <= 0 || model.halfHeight <= 0) return;
    // Larger boxes keep the model centred inside the scratch buffer's size.
    x += std::max(0, width - maxModelWidth) / 2;
    y += std::max(0, height - maxModelHeight) / 2;
    width = std::min(width, maxModelWidth);
    height = std::min(height, maxModelHeight);
    const float scale = 0.96f * std::min(width / 2.0f / model.halfWidth, height / 2.0f / model.halfHeight);
    const float cosYaw = std::cos(yaw), sinYaw = std::sin(yaw);
    const float cosPitch = std::cos(viewPitch), sinPitch = std::sin(viewPitch);
    auto view = [&](float vx, float vy, float vz, float& outX, float& outY, float& outZ) {
        const float turnedX = vx * cosYaw + vz * sinYaw, turnedZ = -vx * sinYaw + vz * cosYaw;
        outX = turnedX;
        outY = vy * cosPitch - turnedZ * sinPitch;
        outZ = vy * sinPitch + turnedZ * cosPitch;
    };
    for (unsigned i = 0; i < model.vertexCount; ++i) {
        float vx, vy, vz;
        view(model.vertices[i * 3], model.vertices[i * 3 + 1], model.vertices[i * 3 + 2], vx, vy, vz);
        screenX[i] = width / 2.0f + vx * scale;
        screenY[i] = height / 2.0f - vy * scale;
        screenZ[i] = vz * scale * depthUnitsPerPixel;
    }
    std::fill(depth, depth + width * height, emptyDepth);
    std::fill(lit, lit + (width * height + 7) / 8, 0);
    unsigned faceCount = 0;
    for (uint16_t i = 0; i < model.triangleCount; ++i)
        faceCount = std::max<unsigned>(faceCount, model.triangles[i * 4] + 1u);
    if (faceCount > maxFaces) return;
    for (unsigned f = 0; f < faceCount; ++f) {
        float nx, ny, nz;
        view(model.normals[f * 3], model.normals[f * 3 + 1], model.normals[f * 3 + 2], nx, ny, nz);
        facing[f] = nz > 0.5f; // normals are scaled to 127
    }
    // Front faces fill the depth buffer; nothing is drawn for them yet.
    for (uint16_t i = 0; i < model.triangleCount; ++i) {
        const uint16_t* t = model.triangles + i * 4;
        if (facing[t[0]]) fillTriangle(width, height, t[1], t[2], t[3]);
    }
    // Silhouettes (one side facing), sharp edges between front faces, and only
    // where they are not behind a nearer surface.
    for (uint16_t i = 0; i < model.edgeCount; ++i) {
        const uint16_t* e = model.edges + i * 5;
        const bool front = facing[e[2]], back = e[3] != noFace && facing[e[3]];
        if (!(front || back)) continue;
        if (front == back && !e[4]) continue;
        drawLine(width, height, e[0], e[1]);
    }
    // Emit lit pixels as horizontal runs.
    for (int row = 0; row < height; ++row) {
        int run = -1;
        for (int column = 0; column <= width; ++column) {
            const int index = row * width + column;
            const bool on = column < width && (lit[index / 8] >> (index % 8) & 1);
            if (on && run < 0) run = column;
            if (!on && run >= 0) {
                canvas.rectangle(x + run, y + row, column - run, 1, ink);
                run = -1;
            }
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

unsigned firstVisible(const Category& category, uint16_t selection) {
    selection = clampSelection(category, selection);
    return std::min<unsigned>(selection > 2 ? selection - 2 : 0,
        category.count > visibleRows ? category.count - visibleRows : 0);
}

void render(Canvas& canvas, size_t categoryIndex, uint16_t selection, bool browsing, uint32_t timeMs) {
    const auto* group = category(categoryIndex);
    if (!group || !group->count) {
        canvas.text(12, 90, "NO ITEMS", ink);
        return;
    }
    selection = clampSelection(*group, selection);
    const unsigned start = firstVisible(*group, selection);
    canvas.line(220, 64, 220, 250, dim);
    for (unsigned i = start; i < group->count && i < start + visibleRows; ++i) {
        const int baseline = listTop + 18 + static_cast<int>(i - start) * listRowHeight;
        const bool active = i == selection;
        if (active) canvas.rectangle(8, baseline - 18, 204, 24, browsing ? ink : dim);
        canvas.text(12, baseline, group->items[i].name, active && browsing ? black : ink);
    }
    const auto& selected = group->items[selection];
    canvas.text(234, 78, selected.name, ink);
    canvas.text(234, 97, selected.kind, ink);
    drawModel(canvas, selected.model, 234, 101, 234, 88, timeMs * turnRadiansPerMs);
    description(canvas, selected.description);
    char footer[48];
    std::snprintf(footer, sizeof(footer), "%02u/%02u  %s", static_cast<unsigned>(selection + 1), static_cast<unsigned>(group->count),
                  browsing ? "TURN: ITEM  HOLD: BACK" : "CLICK: ITEMS  TURN: CATEGORY");
    canvas.text(10, 265, footer, ink);
}
}
