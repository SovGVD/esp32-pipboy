#include <Touch.h>
#include <cassert>
#include <iostream>

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
    std::cout << "Touch: calibration mapping, clamping, swap, median filter and tap debouncing passed\n";
}
