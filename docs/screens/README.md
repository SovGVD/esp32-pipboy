# Screens and controls

[Project overview](../../README.md) · [Hardware](../hardware/README.md)

The display uses a 480×272 green-on-black Pip-Boy interface. Turn the main
encoder to switch between STAT, INV, DATA, MAP and RADIO.

## Navigation

| Control | Action |
| --- | --- |
| Main encoder turn | Switch the main tab. |
| Secondary encoder turn | Select a subpage, or move through its list after entering it. |
| Secondary encoder click | Enter a list or activate the selected item. |
| Secondary button hold | Return from a list to subpage selection. |

Games use the secondary encoder for movement and click for start/pause/resume;
hold returns to the game list. See the [game controls](../games/README.md#controls).
Incoming emotes open a message window; click or hold dismisses it. Games pause
while that window is open and require a click to resume. Music keeps playing.

## Main tabs

| Tab | Pages and current behavior |
| --- | --- |
| STAT | STATUS shows the Vault Boy animation and demo HP/level/AP values. SPECIAL shows randomized attributes and their descriptions. PERKS is a placeholder. |
| INV | WEAPONS, APPAREL, AID, MISC and JUNK each contain six illustrated items. This is a reference catalog with no equipment or consumption effects. |
| DATA | GAMES launches ATOM BREAKER and STAR PATROL. Runs and best scores last until reboot. |
| MAP | Placeholder; no map or GPS view is implemented. |
| RADIO | MUSIC plays MP3s from SD. EMOTES discovers nearby Pip-Boys and sends named emotes. |

The current font contains ASCII glyphs; unsupported music-tag characters appear
as `?`. Inventory and games have native-resolution software previews for layout
review; the real display and encoder feel still need checking on the board.

## Feature guides and previews

- [Inventory: items, art and editing](../inventory/README.md) ·
  [All 30 item screens](../inventory/previews/contact-sheet.png)
- [Games: controls and rules](../games/README.md) ·
  [Game screens](../games/previews/contact-sheet.png)
- [Music: SD setup, metadata and playback](../music/README.md)
- [Emoji communication: peers and notifications](../communication/README.md)
