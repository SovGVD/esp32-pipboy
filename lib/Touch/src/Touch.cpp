#include "Touch.h"

namespace touch {
namespace {
int axis(int value, int minimum, int maximum, int size, bool invert) {
    if (value < minimum) value = minimum;
    if (value > maximum) value = maximum;
    const int pixel = (value - minimum) * (size - 1) / (maximum - minimum);
    return invert ? size - 1 - pixel : pixel;
}
}

bool valid(const Calibration& c) {
    return c.xMin >= 0 && c.xMin < c.xMax && c.xMax <= 4095 &&
        c.yMin >= 0 && c.yMin < c.yMax && c.yMax <= 4095 &&
        c.pressureMin >= 1 && c.pressureMin <= 4095;
}

bool map(const Calibration& c, int rawX, int rawY, int width, int height, int& x, int& y) {
    if (!valid(c) || width <= 0 || height <= 0 || rawX < 0 || rawX > 4095 || rawY < 0 || rawY > 4095)
        return false;
    if (c.swapXY) { const int swap = rawX; rawX = rawY; rawY = swap; }
    x = axis(rawX, c.xMin, c.xMax, width, c.invertX);
    y = axis(rawY, c.yMin, c.yMax, height, c.invertY);
    return true;
}

int median5(int (&values)[5]) {
    for (int i = 1; i < 5; ++i) {
        const int value = values[i];
        int j = i;
        while (j > 0 && values[j - 1] > value) { values[j] = values[j - 1]; --j; }
        values[j] = value;
    }
    return values[2];
}

TapDetector::TapDetector() : downFrames_(0), upFrames_(0), armed_(true), x_(-1), y_(-1) {}

bool TapDetector::update(bool down, int x, int y) {
    if (!down) {
        downFrames_ = 0;
        if (upFrames_ < 2 && ++upFrames_ == 2) armed_ = true;
        return false;
    }
    upFrames_ = 0;
    if (downFrames_ < 2) ++downFrames_;
    if (downFrames_ < 2 || !armed_) return false;
    armed_ = false;
    x_ = x;
    y_ = y;
    return true;
}
}
