#pragma once

#include <cstdint>

namespace touch {
// XPT2046 calibration, matching the Dragonfruit JC4827W543 resistive defaults.
// Raw X is the 0xD0 conversion and raw Y the 0x90 conversion; the bounds apply
// after the optional axis swap. Inversion is applied after scaling.
struct Calibration {
    int xMin, xMax, yMin, yMax;
    bool swapXY, invertX, invertY;
    int pressureMin;
};
bool valid(const Calibration& calibration);
// Maps 12-bit raw samples to screen pixels, clamping to the screen edges.
bool map(const Calibration& calibration, int rawX, int rawY, int width, int height, int& x, int& y);
int median5(int (&values)[5]);

// Turns per-frame contact samples into single taps. A tap fires on the second
// consecutive frame with contact, and the pen must be up for two frames before
// the next tap, so resistive bounce on press or release does not repeat it.
class TapDetector {
public:
    TapDetector();
    bool update(bool down, int x, int y);
    int x() const { return x_; }
    int y() const { return y_; }
private:
    unsigned downFrames_, upFrames_;
    bool armed_;
    int x_, y_;
};
}
