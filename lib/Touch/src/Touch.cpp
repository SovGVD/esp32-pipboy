#include "Touch.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>

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

void calibrationTarget(unsigned index, int width, int height, int& x, int& y) {
    x = index % 2 ? width - 1 - calibrationInset : calibrationInset;
    y = index / 2 ? height - 1 - calibrationInset : calibrationInset;
}

namespace {
constexpr int minimumSpan = 300; // raw units between the targets on one axis

// Fits raw = a + b * screen through two targets and extrapolates to both edges.
bool fitAxis(int rawLow, int rawHigh, int screenLow, int screenHigh, int size,
             int& minimum, int& maximum, bool& invert) {
    if (std::abs(rawHigh - rawLow) < minimumSpan) return false;
    const double slope = static_cast<double>(rawHigh - rawLow) / (screenHigh - screenLow);
    const long atStart = std::lround(rawLow - slope * screenLow);
    const long atEnd = std::lround(rawLow + slope * (size - 1 - screenLow));
    invert = atStart > atEnd;
    minimum = static_cast<int>(std::max(0L, std::min(atStart, atEnd)));
    maximum = static_cast<int>(std::min(4095L, std::max(atStart, atEnd)));
    return minimum < maximum;
}
}

bool calibrate(const Sample (&raw)[calibrationTargets], int width, int height, int pressureMin,
               Calibration& out) {
    if (width <= 2 * calibrationInset || height <= 2 * calibrationInset) return false;
    // Whichever raw channel changes more from left to right drives screen X.
    const int channelXLeftRight = raw[1].x + raw[3].x - raw[0].x - raw[2].x;
    const int channelYLeftRight = raw[1].y + raw[3].y - raw[0].y - raw[2].y;
    Calibration result = {};
    result.swapXY = std::abs(channelYLeftRight) > std::abs(channelXLeftRight);
    result.pressureMin = pressureMin;
    int screenRaw[calibrationTargets][2];
    for (unsigned i = 0; i < calibrationTargets; ++i) {
        screenRaw[i][0] = result.swapXY ? raw[i].y : raw[i].x;
        screenRaw[i][1] = result.swapXY ? raw[i].x : raw[i].y;
    }
    int left, top, right, bottom;
    calibrationTarget(0, width, height, left, top);
    calibrationTarget(3, width, height, right, bottom);
    const bool ok =
        fitAxis((screenRaw[0][0] + screenRaw[2][0]) / 2, (screenRaw[1][0] + screenRaw[3][0]) / 2,
                left, right, width, result.xMin, result.xMax, result.invertX) &&
        fitAxis((screenRaw[0][1] + screenRaw[1][1]) / 2, (screenRaw[2][1] + screenRaw[3][1]) / 2,
                top, bottom, height, result.yMin, result.yMax, result.invertY);
    if (!ok || !valid(result)) return false;
    out = result;
    return true;
}

PressCapture::PressCapture() : frames_(0), sumX_(0), sumY_(0), armed_(false), result_{-1, -1} {}

bool PressCapture::update(bool down, int rawX, int rawY) {
    if (down) {
        if (!armed_) return false;
        if (frames_++) { sumX_ += rawX; sumY_ += rawY; }
        return false;
    }
    armed_ = true;
    const unsigned frames = frames_;
    const long sumX = sumX_, sumY = sumY_;
    frames_ = 0;
    sumX_ = sumY_ = 0;
    if (frames < 3) return false;
    result_.x = static_cast<int>(sumX / (frames - 1));
    result_.y = static_cast<int>(sumY / (frames - 1));
    return true;
}

int listIndexAt(const ListRegion& region, int x, int y) {
    if (region.rowHeight <= 0 || x < region.x || x >= region.x + region.width || y < region.top) return -1;
    const unsigned row = static_cast<unsigned>((y - region.top) / region.rowHeight);
    return row < region.rows ? static_cast<int>(region.first + row) : -1;
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
