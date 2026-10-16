# SD music in RADIO

[Project overview](../../README.md) · [Hardware](../hardware/README.md) · [Emoji communication](../communication/README.md)

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
[include/Hardware.h](../../include/Hardware.h). Volume, boost and track selection are session settings and
are not saved to EEPROM.

## Volume and boost

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

## ID3 metadata and catalog limits

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

## Card changes and recovery

**RESCAN SD stops playback**, remounts the card, and rebuilds the catalog. Use
STOP before removing/replacing a card, then RESCAN after inserting it. No card,
no MP3 files, failed track opens, decoder failures and unavailable PSRAM have
visible status messages. Rescanning or selecting another track allows recovery.

## Background playback

[src/MusicPlayer.cpp](../../src/MusicPlayer.cpp) owns Audio and SD on a dedicated FreeRTOS task on core 0.
The UI sends queued commands and reads protected copies of player state, so
tab changes and GIF/display work do not control audio servicing. ESP-NOW
receive callbacks also queue bounded packets for processing on the UI thread.

## Playback checks on the board

- Play tagged and untagged MP3s; check title, artist, album and playback time.
- Pause/resume, skip, change volume and boost, then let a track finish to check
  automatic advancement. Compare boost settings at VOL 21 and reduce boost if
  distortion or limiter pumping worsens.
- Leave RADIO for other tabs, including the Vault Boy animation, games and
  ESP-NOW messages, and confirm uninterrupted audio.
- Boot without a card, insert one and RESCAN; check an empty card and a bad file.
- Check a large catalog, long labels, encoder navigation and STOP/rescan recovery.

These require the real board, SD card and speaker. For automated audio and
metadata checks, see [development and verification](../development/README.md).

References: [ESP32-audioI2S 3.0.8](https://github.com/schreibfaul1/ESP32-audioI2S/tree/3.0.8),
[ID3v2.3](https://id3.org/id3v2.3.0), [ID3v2.4](https://id3.org/id3v2.4.0-structure).
