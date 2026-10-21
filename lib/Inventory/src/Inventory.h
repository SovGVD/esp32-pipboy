#pragma once

#include <cstddef>
#include <cstdint>

namespace inventory {
constexpr uint16_t ink = 0x45a2;
constexpr uint16_t dim = 0x2201;
constexpr uint16_t black = 0;
constexpr size_t categoryCount = 5;

// Low-poly solid model generated from assets/inventory/models.py. Faces are
// only referenced by index: their normals (x, y, z per face, scaled to 127),
// their triangles (face, a, b, c) and their edges (a, b, faceA, faceB, hard).
struct Mesh {
    const int16_t* vertices; // x, y, z per vertex, within +-1000
    uint16_t vertexCount;
    const int8_t* normals;
    const uint16_t* triangles;
    uint16_t triangleCount;
    const uint16_t* edges;
    uint16_t edgeCount;
    // Largest half width/height seen during a full turn at viewPitch, so the
    // model keeps one size while it rotates.
    int16_t halfWidth, halfHeight;
};
struct Item {
    const char* id;
    const char* name;
    const char* kind;
    const char* description;
    Mesh model;
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
constexpr float viewPitch = 0.35f;        // radians the camera looks down; matches inventory_mesh.py
constexpr float turnRadiansPerMs = 0.0008f; // about 46 degrees per second
constexpr int maxModelWidth = 240, maxModelHeight = 100;
// Draws the model turned by yaw radians as a hidden-line wireframe: only
// silhouettes and sharp edges, and only where no nearer surface covers them.
void drawModel(Canvas& canvas, const Mesh& model, int x, int y, int width, int height, float yaw);
void render(Canvas& canvas, size_t categoryIndex, uint16_t selection, bool browsing, uint32_t timeMs = 0);
// Item list rows as render() draws them, for touch: rows of 26 px from y 67, x 8..211.
constexpr unsigned visibleRows = 6;
constexpr int listTop = 67, listRowHeight = 26, listX = 8, listWidth = 204;
unsigned firstVisible(const Category& category, uint16_t selection);
}
