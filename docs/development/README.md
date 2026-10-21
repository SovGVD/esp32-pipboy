# Build and development

[Project overview](../../README.md) · [Hardware](../hardware/README.md) · [Screens](../screens/README.md)

Run all commands below from the repository root.

## Build and upload

Open this repository in VS Code with the PlatformIO extension, or install the
PlatformIO CLI and run from the repository root:

```sh
pio run
pio run -t upload
pio device monitor
```

If several serial devices are connected, specify the board with
`pio run -t upload --upload-port /dev/ttyACM0` (use your actual port).
Close the serial monitor before uploading. On Windows, use a port such as `COM3`.

[platformio.ini](../../platformio.ini) pins ESP32 platform 6.12.0 / Arduino 2.0.17 and the libraries,
including the same ESP32-audioI2S **3.0.8 Git tag** used by `esp32spi-tests`.
That audio library reports `2.0.0` in its package manifest; this is expected.
See [hardware](../hardware/README.md) for board configuration and wiring.

The former `pipboy/*.ino` sketch is now ordinary C++ in `src/`, with explicit
declarations in `include/Pipboy.h`. Edit these files and build from the root;
Arduino IDE sketch concatenation is no longer required. Fonts and GIF assets
are preserved in `include/assets/`. The vendor PDF remains in `JC4827W543/`.

## Verification

```sh
./test/host/run.sh
pio run
```

The host checks compile with AddressSanitizer and UndefinedBehaviorSanitizer.
They cover ID3 versions, encodings, large artwork, fallback, truncation and
10,000 deterministic malformed-tag mutations. The actual music service runs
against host doubles for SD, Audio and FreeRTOS to check independent background
servicing, playback controls, EOF advancement, catalog bounds and recovery.
Additional DSP tests cover all boost levels, exact bypass, stereo linking,
full-scale transients, bounded samples, limiter recovery and silence. Service
tests verify that the I2S callback continues playback, boost controls work and
mute remains effective with boost enabled.
Leak detection defaults off for compatibility with traced/sandboxed runners;
set `ASAN_OPTIONS=detect_leaks=1` if your host supports it. The service test uses
process-lifetime RTOS objects, as does the firmware.

The same command also checks the inventory catalog, vector generation and
native layout, arcade simulation, rendering and the DATA/encoder input
handlers, plus the chat keyboard, text sanitizing, word wrap, rendering, touch calibration
mapping, 4-point calibration and tap debouncing.
See the [inventory](../inventory/README.md), [game](../games/README.md) and
[chat](../communication/README.md) guides for coverage and preview commands.

Host checks and builds do not establish audible playback, physical display
behavior or control feel. Follow the device checklists in the
[music](../music/README.md), [inventory](../inventory/README.md),
[games](../games/README.md) and [communication](../communication/README.md) guides.

Board reference: [PlatformIO ESP32-S3 DevKitC-1 documentation](https://docs.platformio.org/en/latest/boards/espressif32/esp32-s3-devkitc-1.html).
