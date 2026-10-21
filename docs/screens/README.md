# Screens and controls

[Project overview](../../README.md) · [Hardware](../hardware/README.md)

The display uses a 480×272 green-on-black Pip-Boy interface. Turn the main
encoder, or tap a tab name, to switch between STAT, INV, DATA, MAP and RADIO.
Everything can be used with the dials, with the resistive touchscreen, or with
both mixed.

## Navigation

| Dials | Touch | Action |
| --- | --- | --- |
| Main encoder turn | Tap a tab (STAT, INV, ...) | Switch the main tab. |
| Secondary encoder turn | Tap a subpage name | Select a subpage. |
| Secondary encoder turn in a list | Tap a row | Select a list item. A row near the edge of a long list scrolls it. |
| Secondary encoder click | Tap the selected row again | Activate the selected item (play a track, open a game or chat, run a setup item). |
| Secondary button hold | Tap the current subpage name | Go back: leave a list, game or conversation. |

Touch reacts to taps; one tap is one action. Two taps on a row (select, then
activate) match one turn plus one click, so scrolling a list by tapping its
last row never starts anything by accident.

Games use the secondary encoder (or a finger held on the paddle/ship row) for
movement and click (or a tap above that row) for start/pause/resume; hold, or a
tap on GAMES, returns to the game list. See the [game controls](../games/README.md#controls).
In RADIO → CHAT, tap a key on the resistive touchscreen, or turn the secondary
encoder to move through the on-screen keyboard and click to press a key; see the [chat controls](../communication/README.md#keyboard).
Incoming messages open a message window; tap, click or hold dismisses it. Games pause
while that window is open and require a click to resume. Music keeps playing.

## Setup mode

Hold the secondary encoder button while powering on to open the setup menu.
Turn to choose and click to select, or tap an item twice:

| Item | Action |
| --- | --- |
| CHANGE NAME | Edit the device name used by chat. With the dial: turn to pick a character position and click, turn to pick the letter and click to set it. By touch: tap a position, then tap a letter in the palette below (the cursor moves to the next position; `_` is a space). Select or tap `[SUBMIT]` to save and return to the menu. |
| CALIBRATE TOUCH | Run the [touch calibration](../hardware/README.md#calibration) and save it. |
| SAVE AND EXIT | Save settings and restart. |

The menu also shows the current name and whether touch is calibrated. On the
very first start, touch calibration runs automatically once.

## Main tabs

| Tab | Pages and current behavior |
| --- | --- |
| STAT | STATUS shows the Vault Boy animation and demo HP/level/AP values. SPECIAL shows randomized attributes and their descriptions. PERKS is a placeholder. |
| INV | WEAPONS, APPAREL, AID, MISC and JUNK each contain six illustrated items. This is a reference catalog with no equipment or consumption effects. |
| DATA | GAMES launches ATOM BREAKER and STAR PATROL. Runs and best scores last until reboot. |
| MAP | Placeholder; no map or GPS view is implemented. |
| RADIO | MUSIC plays MP3s from SD. CHAT discovers nearby Pip-Boys and exchanges text messages typed on an on-screen keyboard. |

The current font contains ASCII glyphs; unsupported music-tag characters appear
as `?`. Inventory and games have native-resolution software previews for layout
review; the real display and encoder feel still need checking on the board.

## Feature guides and previews

- [Inventory: items, art and editing](../inventory/README.md) ·
  [All 30 item screens](../inventory/previews/contact-sheet.png)
- [Games: controls and rules](../games/README.md) ·
  [Game screens](../games/previews/contact-sheet.png)
- [Music: SD setup, metadata and playback](../music/README.md)
- [Chat communication: peers, keyboard and notifications](../communication/README.md)
