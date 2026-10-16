# ESP32 Pip-Boy

Pip-Boy UI for the JC4827W543 ESP32-S3 display board, built with PlatformIO
and the Arduino framework. 3D models: https://www.thingiverse.com/thing:6891612

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

`platformio.ini` pins ESP32 platform 6.12.0 / Arduino 2.0.17 and the libraries,
including the same ESP32-audioI2S **3.0.8 Git tag** used by `esp32spi-tests`.
That audio library reports `2.0.0` in its package manifest; this is expected.
The board configuration uses 240 MHz, 4 MB flash, DIO at 80 MHz, OPI PSRAM,
USB CDC and the Huge APP partition (3 MB application, no OTA).

The former `pipboy/*.ino` sketch is now ordinary C++ in `src/`, with explicit
declarations in `include/Pipboy.h`. Edit these files and build from the root;
Arduino IDE sketch concatenation is no longer required. Fonts and GIF assets
are preserved in `include/assets/`. The vendor PDF remains in `JC4827W543/`.

## Hardware

The display configuration preserves the original sketch's NV3041A QSPI panel
and 480×272 framebuffer. SD and NS4163 audio wiring matches the working
`esp32spi-tests` project. Pins are defined in `include/Hardware.h` and
`src/Display.cpp`.

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

## SD music in RADIO

1. Format an SD card as FAT32 and copy `.mp3` files to its root or subfolders.
   The extension is case-insensitive. Insert the card before boot.
2. Turn the main encoder to **RADIO**. Its default subpage is **MUSIC**;
   **EMOTES** retains the existing ESP-NOW peer communication.
3. Click the secondary encoder to browse the list. Rotate it to select a track
   and click again to play. Clicking the current playing track pauses it;
   clicking that paused track resumes it.
4. Hold the secondary button, or select **BACK**, to return to subpage selection.
   Rotate the main encoder to use STAT or another tab. **Music continues**.

The scrolling list ends with **PLAY / PAUSE**, **STOP**, **PREVIOUS**, **NEXT**,
**VOLUME −**, **VOLUME +**, **BOOST −**, **BOOST +**, **RESCAN SD** and **BACK**. Songs advance automatically
and wrap to the beginning of the catalog. Boot starts stopped. The initial
volume is 21 of 21 with +6 dB boost; both defaults can be changed in
`include/Hardware.h`. Volume, boost and track selection are session settings and
are not saved to EEPROM.

**BOOST** selects 0 (bypass) through **+90 dB (MAX)** of additional digital gain
in 6 dB steps: +6, +12, +18, +24, +30, +36, +42, +48, +54, +60, +66, +72,
+78, +84 and +90 dB. Startup remains at +6 dB.
The header displays the current volume and boost. This processing uses the
library's final I2S callback; increasing its `setVolumeSteps()` value alone only
adds volume positions and does not amplify beyond the original maximum.
A stereo-linked limiter reduces gain immediately at peaks, with up to 100 ms
recovery, keeping boosted samples within ±30000. Boost zero is a bit-exact
bypass and volume zero stays muted. Boost can make quieter passages louder but
compresses dynamics; the peak limiter does not guarantee distortion-free sound
from the physical amplifier/speaker or increase their maximum output power.
MAX can bring even a single nonzero 16-bit sample unit to the limiter ceiling;
larger requested gain cannot raise that bounded output any further. These are
requested digital gains, not acoustic loudness increases: peaks still stop at
±30000. High settings strongly compress dynamics and amplify hiss/noise. PCM
multiplication uses 64-bit intermediates to prevent overflow at these gains.

Rows show the ID3 title and artist; the selected song's artist and album appear
below the list. The footer shows the current song and playback time (duration
may remain zero until the decoder determines it). Files without a title tag
use the filename. ID3v2.2/2.3/2.4 and ID3v1 are supported, with Latin-1, UTF-8
and UTF-16 decoding. The existing font contains ASCII glyphs, so unsupported
characters display as `?`; long lines are shortened to fit. Compressed or
encrypted frames and whole-tag ID3v2.2/2.3 unsynchronisation are skipped, with
ID3v1/filename fallback. Artwork is skipped without loading it.

Scanning discovers up to 256 tracks, traverses up to four subfolder levels,
and examines at most 4096 directory entries. Hidden entries are ignored; paths
must fit in 255 bytes. Discovered tracks are sorted by title, then path. A `+`
beside the track count indicates that a scan limit was reached. The catalog
uses about 209 KiB of PSRAM, keeping its strings out of internal RAM.

**RESCAN SD stops playback**, remounts the card, and rebuilds the catalog. Use
STOP before removing/replacing a card, then RESCAN after inserting it. No card,
no MP3 files, failed track opens, decoder failures and unavailable PSRAM have
visible status messages. Rescanning or selecting another track allows recovery.

`src/MusicPlayer.cpp` owns Audio and SD on a dedicated FreeRTOS task on core 0.
The UI sends queued commands and reads protected copies of player state, so
tab changes and GIF/display work do not control audio servicing. ESP-NOW
receive callbacks also queue bounded packets for processing on the UI thread.

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

These checks do not establish audible playback or physical display/input
behavior. Before accepting the firmware on the board:

- Play tagged and untagged MP3s from the real SD card; check title/artist/album.
- Pause/resume, skip, change volume and boost, then let a track finish to check
  advancement. Compare the boost settings at VOL 21, listening for distortion
  and limiter pumping; reduce boost if sound quality worsens.
- Leave RADIO for STAT and other tabs and confirm uninterrupted audio, including
  while the Vault Boy GIF and ESP-NOW emotes are active.
- Boot without a card, insert one and RESCAN; check an empty card and a bad file.
- Check a large catalog, encoder navigation, long labels and STOP/rescan recovery.

Library references: [PlatformIO board documentation](https://docs.platformio.org/en/latest/boards/espressif32/esp32-s3-devkitc-1.html),
[ESP32-audioI2S 3.0.8](https://github.com/schreibfaul1/ESP32-audioI2S/tree/3.0.8),
[ID3v2.3](https://id3.org/id3v2.3.0), [ID3v2.4](https://id3.org/id3v2.4.0-structure).
