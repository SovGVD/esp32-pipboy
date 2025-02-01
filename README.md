# ESP32 Pipboy

3D models: https://www.thingiverse.com/thing:6891612

## Arduino IDE + ESP32 settings
1. Add ESP32 board in Arduino IDE: https://randomnerdtutorials.com/installing-the-esp32-board-in-arduino-ide-windows-instructions/
2. Open `pipboy.ino` in Arduino IDE (depends on how you download it, make sure that folder with files also named `pipboy`)
3. Change Board setting in Tools menu as per JC4827W543 user manual:
   - Board: ESP32S3 Dev Module
   - USB CDC On Boot: Enabled
   - CPU frequency: 240MHz (WiFi)
   - Flash mode: DIO 80MHz
   - Flash size: 4MB (32Mb)
   - Partition Scheme: Huge APP (3MB No OTA/1MB SPIFFS)
   - PSRAM: OPI PSRAM
