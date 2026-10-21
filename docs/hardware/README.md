# Hardware

[Project overview](../../README.md) · [Build and upload](../development/README.md) · [Screens](../screens/README.md)

The display configuration preserves the original sketch's NV3041A QSPI panel
and 480×272 framebuffer. SD and NS4163 audio wiring matches the working
`esp32spi-tests` project. Pins are defined in [include/Hardware.h](../../include/Hardware.h) and
[src/Display.cpp](../../src/Display.cpp).

| Device | Signal | GPIO |
| --- | --- | --- |
| SD card, SPI2 / FSPI | CS / SCK / MISO / MOSI | 10 / 12 / 13 / 11 |
| XPT2046 resistive touch, SPI2 shared with SD | CS / IRQ | 38 / 3 |
| NS4163, I2S | BCLK / LRCLK / DATA | 42 / 2 / 41 |
| Display, SPI3 | CS / SCK / D0 / D1 / D2 / D3 | 45 / 47 / 21 / 48 / 40 / 39 |
| Display | Backlight | 1 |
| Main encoder | A / B | 16 / 15 |
| Secondary encoder | A / B / button | 17 / 18 / 6 |

`ESP32QSPI_SPI_HOST=SPI3_HOST` is required: the GFX library otherwise defaults
to SPI2, which conflicts with the SD card. PSRAM is required for the display,
audio buffers and music catalog.

## Resistive touch

The XPT2046 controller shares SCK/MISO/MOSI with the SD card. `touchSetup()`
drives both chip selects high and starts the SPI bus before the music task
mounts the card. Each touch sample runs inside an `SPI` transaction at 2 MHz,
which the Arduino core serializes with the SD library's own transactions on the
music task. Five X/Y samples are median-filtered, discarding the first
conversion after each axis switch. Touch is polled once per UI frame and is
currently used by RADIO → CHAT and to dismiss message windows.

### Calibration

On the first start (no saved calibration in EEPROM) the Pip-Boy shows a
calibration screen before the main UI. Tap the center of each cross: four
corner targets, then a center check. If the check lands more than 20 px away,
or the corner taps barely differ, it starts over. The result (raw ranges,
axis swap and inversion) is saved in EEPROM next to the device name. Click the
dial to skip; the defaults are then saved so it is not asked again.

To recalibrate later, hold the secondary encoder button while powering on to
enter [setup mode](../screens/README.md#setup-mode) and choose
**CALIBRATE TOUCH** (click the dial there to cancel).

The defaults in `include/Hardware.h` come from the
[Dragonfruit JC4827W543 port](https://github.com/SovGVD/Dragonfruit-JC4827W543-esp32/blob/main/docs/jc4827w543.md#calibrate-resistive-touch)
(raw 200–3900 on both axes, both axes inverted). `touchPressureMin` (400) is
not calibrated; raise it if light contacts register as taps. Each new contact
logs `Touch raw=x,y pressure=p screen=x,y` to the serial monitor, and a saved
calibration is logged as `Touch calibration saved: ...`.

The PlatformIO board configuration uses 240 MHz, 4 MB flash, DIO at 80 MHz,
OPI PSRAM, USB CDC and the Huge APP partition (3 MB application, no OTA).
See the [vendor files](../../JC4827W543/) and
[printable 3D models](https://www.thingiverse.com/thing:6891612) for board
and enclosure references.
