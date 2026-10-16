#pragma once
#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

// Adafruit-compatible font structs, matching Arduino_GFX. No display, Arduino
// or guessed desktop font is used for the native-resolution reference images.
struct GFXglyph {
    uint16_t bitmapOffset;
    uint8_t width, height, xAdvance;
    int8_t xOffset, yOffset;
};
struct GFXfont { uint8_t* bitmap; GFXglyph* glyph; uint16_t first, last; uint8_t yAdvance; };
#include "assets/fallout10.h"
#include "assets/fallout11.h"

template <typename Canvas>
class NativeFrame : public Canvas {
public:
    std::vector<uint16_t> pixels = std::vector<uint16_t>(480 * 272, 0);
    const GFXfont* font = &monofonto10pt7b;
    unsigned lines = 0, triangles = 0, texts = 0;
    bool strict = true;
    std::string lastText;

    void pixel(int x, int y, uint16_t color) {
        if (strict && (x < 0 || x >= 480 || y < 0 || y >= 272)) {
            std::cerr << "Out-of-bounds pixel " << x << ',' << y << " after text: " << lastText << '\n';
            std::abort();
        }
        if (x >= 0 && x < 480 && y >= 0 && y < 272) pixels[y * 480 + x] = color;
    }
    void line(int x1, int y1, int x2, int y2, uint16_t color) override {
        ++lines;
        const int dx = std::abs(x2-x1), dy = -std::abs(y2-y1);
        const int sx = x1 < x2 ? 1 : -1, sy = y1 < y2 ? 1 : -1;
        int error = dx + dy;
        for (;;) {
            pixel(x1, y1, color);
            if (x1 == x2 && y1 == y2) break;
            const int twice = error * 2;
            if (twice >= dy) { error += dy; x1 += sx; }
            if (twice <= dx) { error += dx; y1 += sy; }
        }
    }
    void triangle(int x1, int y1, int x2, int y2, int x3, int y3, uint16_t color) override {
        ++triangles;
        const int left = std::min({x1, x2, x3}), right = std::max({x1, x2, x3});
        const int top = std::min({y1, y2, y3}), bottom = std::max({y1, y2, y3});
        auto cross = [](int ax, int ay, int bx, int by, int x, int y) {
            return (bx-ax)*(y-ay)-(by-ay)*(x-ax);
        };
        const int area = cross(x1, y1, x2, y2, x3, y3);
        if (!area) { line(x1,y1,x2,y2,color); line(x2,y2,x3,y3,color); return; }
        for (int y = top; y <= bottom; ++y) for (int x = left; x <= right; ++x) {
            const int a = cross(x1,y1,x2,y2,x,y), b = cross(x2,y2,x3,y3,x,y), c = cross(x3,y3,x1,y1,x,y);
            if ((a >= 0 && b >= 0 && c >= 0) || (a <= 0 && b <= 0 && c <= 0)) pixel(x,y,color);
        }
    }
    void rectangle(int x, int y, int width, int height, uint16_t color) override {
        for (int py = y; py < y+height; ++py) for (int px = x; px < x+width; ++px) pixel(px,py,color);
    }
    void text(int x, int baseline, const char* value, uint16_t color) override {
        ++texts;
        lastText = value;
        for (const char* p = value; *p; ++p) {
            const unsigned char c = *p;
            assert(c >= font->first && c <= font->last);
            const auto& glyph = font->glyph[c-font->first];
            unsigned bit = 0;
            for (unsigned y = 0; y < glyph.height; ++y) for (unsigned px = 0; px < glyph.width; ++px, ++bit) {
                if (font->bitmap[glyph.bitmapOffset + bit/8] & (0x80 >> (bit%8)))
                    pixel(x + glyph.xOffset + px, baseline + glyph.yOffset + y, color);
            }
            x += glyph.xAdvance;
        }
    }
    void fixedText(int x, int y, const char* text, uint16_t color) {
        for (unsigned c = 0; text[c]; ++c) { const char ch[] = {text[c],0}; this->text(x+c*10,y,ch,color); }
    }
    void save(const std::string& path) {
        std::ofstream file(path, std::ios::binary);
        assert(file);
        file << "P6\n480 272\n255\n";
        for (uint16_t pixel : pixels) {
            const char rgb[] = {static_cast<char>((pixel>>11)*255/31),
                static_cast<char>(((pixel>>5)&63)*255/63), static_cast<char>((pixel&31)*255/31)};
            file.write(rgb,3);
        }
    }
};

