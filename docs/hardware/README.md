# Hardware

[Project overview](../../README.md) · [Build and upload](../development/README.md) · [Screens](../screens/README.md)

The display configuration preserves the original sketch's NV3041A QSPI panel
and 480×272 framebuffer. SD and NS4163 audio wiring matches the working
`esp32spi-tests` project. Pins are defined in [include/Hardware.h](../../include/Hardware.h) and
[src/Display.cpp](../../src/Display.cpp).

| Device | Signal | GPIO |
| --- | --- | --- |
| SD card, SPI2 / FSPI | CS / SCK / MISO / MOSI | 10 / 12 / 13 / 11 |
| NS4163, I2S | BCLK / LRCLK / DATA | 42 / 2 / 41 |
| Display, SPI3 | CS / SCK / D0 / D1 / D2 / D3 | 45 / 47 / 21 / 48 / 40 / 39 |
| Display | Backlight | 1 |
| Main encoder | A / B | 16 / 15 |
| Secondary encoder | A / B / button | 17 / 18 / 6 |

`ESP32QSPI_SPI_HOST=SPI3_HOST` is required: the GFX library otherwise defaults
to SPI2, which conflicts with the SD card. PSRAM is required for the display,
audio buffers and music catalog.

The PlatformIO board configuration uses 240 MHz, 4 MB flash, DIO at 80 MHz,
OPI PSRAM, USB CDC and the Huge APP partition (3 MB application, no OTA).
See the [vendor files](../../JC4827W543/) and
[printable 3D models](https://www.thingiverse.com/thing:6891612) for board
and enclosure references.
