#include <Inventory.h>
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <set>
#include <string>
#include <vector>

#include "NativeFrame.h"

class Frame : public NativeFrame<inventory::Canvas> {
public:
    void header(size_t selectedCategory) {
        // The existing main/submenu renderer positions characters individually
        // at 10 px. Reproduce that chrome; body rendering below is actual code.
        font = &monofonto11pt7b;
        const char* main[] = {"STAT", "INV", "DATA", "MAP", "RADIO"};
        const int positions[] = {60, 140, 210, 290, 360};
        for (unsigned i = 0; i < 5; ++i) fixedText(positions[i], 21, main[i], inventory::ink);
        line(0,27,133,27,inventory::ink); line(133,27,133,11,inventory::ink);
        line(134,11,137,11,inventory::ink); line(173,11,177,11,inventory::ink);
        line(177,11,177,27,inventory::ink); line(177,27,479,27,inventory::ink);
        const char* labels[] = {"WEAPONS", "APPAREL", "AID", "MISC", "JUNK"};
        const int offsets[] = {0,81,162,203,254};
        strict = false; // Original scrolling submenu intentionally runs offscreen.
        for (unsigned i = 0; i < 5; ++i)
            fixedText(140+offsets[i]-offsets[selectedCategory],51,labels[i], i==selectedCategory?inventory::ink:inventory::dim);
        strict = true;
        font = &monofonto10pt7b;
    }
};

namespace {
constexpr uint32_t previewTime = 750; // a three-quarter view
constexpr float previewYaw = previewTime * inventory::turnRadiansPerMs;

// A 2x2x2 cube centred at (0, 0, z): 8 vertices, 6 faces, 12 edges.
struct Cube {
    int16_t vertices[24];
    int8_t normals[18];
    uint16_t triangles[48];
    uint16_t edges[60];
    explicit Cube(int16_t z, int16_t half = 400, int16_t offset = 0) {
        for (int i = 0; i < 8; ++i) {
            vertices[i*3] = (i & 4 ? half : -half);
            vertices[i*3+1] = (i & 2 ? half : -half);
            vertices[i*3+2] = static_cast<int16_t>(z + offset + (i & 1 ? half : -half));
        }
        // Faces: -x, +x, -y, +y, -z, +z; corners counter-clockwise from outside.
        const uint16_t faces[6][4] = {{0,1,3,2},{4,6,7,5},{0,4,5,1},{2,3,7,6},{0,2,6,4},{1,5,7,3}};
        const int8_t n[6][3] = {{-127,0,0},{127,0,0},{0,-127,0},{0,127,0},{0,0,-127},{0,0,127}};
        unsigned e = 0;
        for (uint16_t f = 0; f < 6; ++f) {
            for (int k = 0; k < 3; ++k) normals[f*3+k] = n[f][k];
            const uint16_t t[8] = {f, faces[f][0], faces[f][1], faces[f][2], f, faces[f][0], faces[f][2], faces[f][3]};
            std::copy(t, t + 8, triangles + f*8);
            for (int k = 0; k < 4; ++k) {
                const uint16_t a = faces[f][k], b = faces[f][(k+1)%4];
                if (a > b) continue; // each edge once, owned by the face with a < b
                uint16_t other = 0;
                for (uint16_t g = 0; g < 6; ++g)
                    for (int m = 0; m < 4; ++m)
                        if (g != f && faces[g][m] == b && faces[g][(m+1)%4] == a) other = g;
                const uint16_t edge[5] = {a, b, f, other, 1};
                std::copy(edge, edge + 5, edges + e*5);
                ++e;
            }
        }
        assert(e == 12);
    }
    inventory::Mesh mesh(int16_t halfSize) const {
        return {vertices, 8, normals, triangles, 12, edges, 12, halfSize, halfSize};
    }
};

long litPixels(const Frame& frame) {
    return std::count(frame.pixels.begin(), frame.pixels.end(), inventory::ink);
}

void hiddenLineChecks() {
    // Seen from an angle a cube shows 9 of its 12 edges; the 3 behind it
    // must not be drawn, so it has fewer lit pixels than its full wireframe.
    const Cube cube(0);
    const inventory::Mesh mesh = cube.mesh(800);
    Frame angled;
    inventory::drawModel(angled, mesh, 0, 0, 200, 100, 0.6f);
    assert(litPixels(angled) > 200);
    // The far corner is hidden:
    // its projected point lies inside the front faces, so it stays dark.
    {
        const float yaw = 0.6f, pitch = inventory::viewPitch;
        const float scale = 0.96f * std::min(100.0f / 800, 50.0f / 800);
        // Vertex 4 (+x, -y, -z) is the far, lower corner for this yaw and pitch.
        const float x = 400, y = -400, z = -400;
        const float tx = x*std::cos(yaw) + z*std::sin(yaw), tz = -x*std::sin(yaw) + z*std::cos(yaw);
        const float sx = 100 + tx*scale, sy = 50 - (y*std::cos(pitch) - tz*std::sin(pitch))*scale;
        for (int dy = -1; dy <= 1; ++dy)
            for (int dx = -1; dx <= 1; ++dx)
                assert(angled.pixels[(static_cast<int>(sy)+dy)*480 + static_cast<int>(sx)+dx] != inventory::ink);
    }
    // A small cube hidden right behind a big one disappears completely.
    Frame front, both;
    const Cube big(400, 400), small(-500, 150);
    inventory::drawModel(front, big.mesh(1000), 0, 0, 200, 100, 0.3f);
    struct Pair {
        int16_t vertices[48]; int8_t normals[36]; uint16_t triangles[96]; uint16_t edges[120];
    } pair;
    std::copy(big.vertices, big.vertices + 24, pair.vertices);
    std::copy(small.vertices, small.vertices + 24, pair.vertices + 24);
    std::copy(big.normals, big.normals + 18, pair.normals);
    std::copy(small.normals, small.normals + 18, pair.normals + 18);
    for (int i = 0; i < 48; ++i) {
        pair.triangles[i] = big.triangles[i];
        pair.triangles[48+i] = static_cast<uint16_t>(small.triangles[i] + (i % 4 ? 8 : 6));
    }
    for (int i = 0; i < 60; ++i) {
        pair.edges[i] = big.edges[i];
        const int field = i % 5;
        pair.edges[60+i] = static_cast<uint16_t>(small.edges[i] + (field < 2 ? 8 : field < 4 ? 6 : 0));
    }
    const inventory::Mesh combined = {pair.vertices, 16, pair.normals, pair.triangles, 24, pair.edges, 24, 1000, 1000};
    inventory::drawModel(both, combined, 0, 0, 200, 100, 0.3f);
    assert(both.pixels == front.pixels);
    // Moved beside the big cube, the small one shows up.
    for (int i = 24; i < 48; i += 3) pair.vertices[i] = static_cast<int16_t>(pair.vertices[i] + 700);
    Frame beside;
    inventory::drawModel(beside, combined, 0, 0, 200, 100, 0.3f);
    assert(litPixels(beside) > litPixels(front));
    // Rotation changes the drawing.
    Frame turned;
    inventory::drawModel(turned, mesh, 0, 0, 200, 100, 1.1f);
    assert(turned.pixels != angled.pixels);
}
}

int main(int argc, char** argv) {
    hiddenLineChecks();
    std::set<std::string> ids;
    std::set<std::vector<uint16_t>> silhouettes;
    unsigned count = 0;
    for (size_t c = 0; c < inventory::categoryCount; ++c) {
        const auto* category = inventory::category(c);
        assert(category && category->count == 6);
        assert(inventory::clampSelection(*category,65535) == category->count-1);
        for (uint16_t i = 0; i < category->count; ++i) {
            const auto& item = category->items[i];
            assert(ids.insert(item.id).second);
            assert(item.model.vertices && item.model.vertexCount && item.model.triangleCount && item.model.edgeCount);
            Frame model;
            inventory::drawModel(model,item.model,234,101,234,88,previewYaw);
            const auto illuminated = std::count(model.pixels.begin(),model.pixels.end(),inventory::ink);
            assert(illuminated > 120); // No blank/vanishing models.
            assert(silhouettes.insert(model.pixels).second); // Every item has distinct art.
            Frame native;
            native.header(c);
            inventory::render(native,c,i,true,previewTime);
            assert(native.texts > 8 && native.lines > 0);
            if (argc >= 2) native.save(std::string(argv[1])+"/"+item.id+".ppm");
            // Turntable frames of one item for the animated preview.
            if (argc == 3 && std::string(argv[2]) == item.id) {
                const uint32_t frames = 48, turnMs = static_cast<uint32_t>(6.2831853f / inventory::turnRadiansPerMs);
                for (uint32_t f = 0; f < frames; ++f) {
                    Frame spin;
                    spin.header(c);
                    inventory::render(spin,c,i,true,f * turnMs / frames);
                    spin.save(std::string(argv[1])+"/spin-"+std::to_string(f)+".ppm");
                }
            }
            Frame navigation;
            inventory::render(navigation,c,i,false,previewTime);
            assert(native.pixels != navigation.pixels);
            ++count;
        }
        Frame stale;
        inventory::render(stale,c,65535,true);
        Frame finalItem;
        inventory::render(finalItem,c,category->count-1,true);
        assert(stale.pixels == finalItem.pixels);
    }
    assert(!inventory::category(inventory::categoryCount));
    Frame invalid;
    inventory::render(invalid,9999,65535,true);
    assert(invalid.texts == 1);
    Frame degenerate;
    inventory::drawModel(degenerate,inventory::category(0)->items[0].model,0,0,0,0,0);
    assert(degenerate.lines == 0 && degenerate.triangles == 0);
    assert(count == 30 && ids.size() == 30);
    std::cout << "Inventory: 30 unique hidden-line models, all categories/selections, native text/pixel bounds and stale-index checks passed\n";
}
