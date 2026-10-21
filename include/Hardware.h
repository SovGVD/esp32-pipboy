#pragma once

// JC4827W543 / ESP32-4827A043 QSPI, matching the original Pip-Boy display
// and the working esp32spi-tests SD + NS4163 audio example.
#define GFX_BL 1
#define CANVAS

namespace hardware {
constexpr int sdCs = 10;
constexpr int sdSck = 12;
constexpr int sdMiso = 13;
constexpr int sdMosi = 11;
// XPT2046 resistive touch shares the SD SPI bus (SCK/MISO/MOSI above).
// BOOT button: free after reset. Holding it during power-on/reset starts the
// ROM bootloader instead, so it is only read once the firmware runs.
constexpr int bootButton = 0;
constexpr unsigned long bootButtonHoldMs = 2000;
constexpr int touchCs = 38;
constexpr int touchIrq = 3;
// Starting calibration from the Dragonfruit JC4827W543 port, not a measurement
// of an individual panel. "Touch raw=..." in the serial log helps adjust it.
constexpr int touchRawXMin = 200, touchRawXMax = 3900;
constexpr int touchRawYMin = 200, touchRawYMax = 3900;
constexpr bool touchSwapXY = false, touchInvertX = true, touchInvertY = true;
constexpr int touchPressureMin = 400; // raise to reject light/noisy contacts
constexpr int audioBclk = 42;
constexpr int audioLrclk = 2;
constexpr int audioData = 41;
constexpr unsigned audioVolume = 21; // ESP32-audioI2S range: 0..21
constexpr unsigned audioBoostLevel = 1; // 0=off, 1..15=+6..+90 dB in 6 dB steps
static_assert(audioVolume <= 21, "Audio volume must be in 0..21");
static_assert(audioBoostLevel <= 15, "Audio boost level must be in 0..15");
}
