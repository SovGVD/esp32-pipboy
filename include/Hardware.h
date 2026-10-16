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
constexpr int audioBclk = 42;
constexpr int audioLrclk = 2;
constexpr int audioData = 41;
constexpr unsigned audioVolume = 21; // ESP32-audioI2S range: 0..21
constexpr unsigned audioBoostLevel = 1; // 0=off, 1..15=+6..+90 dB in 6 dB steps
static_assert(audioVolume <= 21, "Audio volume must be in 0..21");
static_assert(audioBoostLevel <= 15, "Audio boost level must be in 0..15");
}
