#include <Touch.h>
#include <cassert>
#include <cstdlib>
#include <iostream>

namespace {
// The raw sample a calibrated panel would report at a screen point.
touch::Sample rawAt(const touch::Calibration& c, int x, int y, int width, int height) {
    const int px = c.invertX ? width - 1 - x : x, py = c.invertY ? height - 1 - y : y;
    const int rx = c.xMin + px * (c.xMax - c.xMin) / (width - 1);
    const int ry = c.yMin + py * (c.yMax - c.yMin) / (height - 1);
    return c.swapXY ? touch::Sample{ry, rx} : touch::Sample{rx, ry};
}
// Calibrates from the four targets of a known panel and checks taps across the screen.
void recovers(const touch::Calibration& panel) {
    touch::Sample raw[touch::calibrationTargets];
    for (unsigned i = 0; i < touch::calibrationTargets; ++i) {
        int x, y; touch::calibrationTarget(i, 480, 272, x, y);
        raw[i] = rawAt(panel, x, y, 480, 272);
    }
    touch::Calibration found = {};
    assert(touch::calibrate(raw, 480, 272, 321, found));
    assert(found.swapXY == panel.swapXY && found.invertX == panel.invertX && found.invertY == panel.invertY);
    assert(found.pressureMin == 321);
    for (int x = 0; x < 480; x += 13) for (int y = 0; y < 272; y += 11) {
        const touch::Sample sample = rawAt(panel, x, y, 480, 272);
        int mx, my;
        assert(touch::map(found, sample.x, sample.y, 480, 272, mx, my));
        assert(std::abs(mx - x) <= 2 && std::abs(my - y) <= 2);
    }
}
}

int main() {
    const touch::Calibration calibration = {200, 3900, 200, 3900, false, true, true, 400};
    int x = 0, y = 0;
    assert(touch::valid(calibration));
    // Both axes are inverted by default: raw maximum is the top-left corner.
    assert(touch::map(calibration, 3900, 3900, 480, 272, x, y) && x == 0 && y == 0);
    assert(touch::map(calibration, 200, 200, 480, 272, x, y) && x == 479 && y == 271);
    assert(touch::map(calibration, 2050, 2050, 480, 272, x, y) && x == 240 && y == 136);
    // Samples beyond the calibrated range clamp to the screen edges.
    assert(touch::map(calibration, 0, 4095, 480, 272, x, y) && x == 479 && y == 0);
    assert(!touch::map(calibration, -1, 0, 480, 272, x, y) && !touch::map(calibration, 0, 4096, 480, 272, x, y));
    assert(!touch::map(calibration, 0, 0, 0, 272, x, y));
    touch::Calibration swapped = calibration;
    swapped.swapXY = true; swapped.invertX = swapped.invertY = false;
    assert(touch::map(swapped, 200, 3900, 480, 272, x, y) && x == 479 && y == 0);
    touch::Calibration broken = calibration;
    broken.xMin = broken.xMax;
    assert(!touch::valid(broken) && !touch::map(broken, 1000, 1000, 480, 272, x, y));
    broken = calibration; broken.pressureMin = 0;
    assert(!touch::valid(broken));

    int values[5] = {900, 5, 4000, 30, 31};
    assert(touch::median5(values) == 31);

    // One tap per press: needs two frames of contact, and two frames up to re-arm.
    touch::TapDetector tap;
    assert(!tap.update(true, 10, 20));
    assert(tap.update(true, 11, 21) && tap.x() == 11 && tap.y() == 21);
    for (int i = 0; i < 30; ++i) assert(!tap.update(true, 12, 22));
    assert(!tap.update(false, -1, -1));
    assert(!tap.update(true, 50, 50) && !tap.update(true, 50, 50)); // release bounce
    assert(!tap.update(false, -1, -1) && !tap.update(false, -1, -1));
    assert(!tap.update(true, 60, 70) && tap.update(true, 61, 71) && tap.x() == 61);
    assert(!tap.update(false, -1, -1) && !tap.update(false, -1, -1));
    assert(!tap.update(true, 5, 5) && !tap.update(false, -1, -1)); // single-frame noise
    // Calibration recovers default, swapped and partly inverted panels.
    recovers(calibration);
    recovers({300, 3700, 450, 3500, true, true, false, 400});
    recovers({150, 3950, 250, 3800, false, false, true, 400});
    recovers({1000, 2200, 900, 2000, true, false, false, 400});
    int tx, ty;
    touch::calibrationTarget(0, 480, 272, tx, ty); assert(tx == 32 && ty == 32);
    touch::calibrationTarget(3, 480, 272, tx, ty); assert(tx == 447 && ty == 239);
    // Identical taps (no axis movement) are rejected and leave the output alone.
    touch::Sample flat[touch::calibrationTargets] = {{2000,2000},{2050,2010},{2010,2040},{2060,2050}};
    touch::Calibration untouched = calibration;
    assert(!touch::calibrate(flat, 480, 272, 400, untouched) && untouched.xMin == 200);
    assert(!touch::calibrate(flat, 40, 40, 400, untouched));

    // Press capture: ignores a press already held, the first frame and short presses.
    touch::PressCapture capture;
    assert(!capture.update(true, 9, 9) && !capture.update(true, 9, 9));
    assert(!capture.update(false, 0, 0));
    assert(!capture.update(true, 50, 50) && !capture.update(false, 0, 0)); // one frame is too short
    assert(!capture.update(true, 4000, 4000)); // light first contact
    assert(!capture.update(true, 100, 200) && !capture.update(true, 110, 210) && !capture.update(true, 120, 220));
    assert(capture.update(false, 0, 0) && capture.result().x == 110 && capture.result().y == 210);
    assert(!capture.update(false, 0, 0));

    std::cout << "Touch: calibration mapping, clamping, swap, median filter, tap debouncing, 4-point calibration and press capture passed\n";
}
