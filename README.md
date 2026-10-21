# ESP32 Pip-Boy

Pip-Boy UI for the JC4827W543 ESP32-S3 display board, built with PlatformIO
and the Arduino framework. Includes illustrated inventory, two arcade games,
background SD music and ESP-NOW text chat with an on-screen keyboard.

## Guides

| Category | Contents |
| --- | --- |
| [Hardware](docs/hardware/README.md) | Board, display, wiring, SD, audio and enclosure references. |
| [Screens and controls](docs/screens/README.md) | Main tabs, encoder navigation and current screen features. |
| [Inventory](docs/inventory/README.md) | Fallout items, rotating 3D wireframe models, sources and model editing. |
| [Games](docs/games/README.md) | ATOM BREAKER and STAR PATROL controls, rules and previews. |
| [Music](docs/music/README.md) | SD setup, playback, ID3 tags, volume and boost. |
| [Chat communication](docs/communication/README.md) | Nearby peers, on-screen keyboard, ESP-NOW messages and format. |
| [Build and development](docs/development/README.md) | PlatformIO setup, migration details and verification. |

## Quick start

Open this folder in VS Code with the PlatformIO extension, or use the
PlatformIO CLI from the repository root:

```sh
pio run
pio run -t upload
pio device monitor
```

For serial port selection and troubleshooting, see
[build and upload](docs/development/README.md#build-and-upload).
