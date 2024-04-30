# NerdMiner RIG

**The NerdSoloMiner v2**

I designed a dedicated PCB to work as Mining RIG for up to 4 ESP Boards with NerdMiner v2 Software. No fancy display at this time. An ESP8266 controls OLED display which shows current status of Minig Pool, date & time and maybe some other info in the future...

This project assumess learning some Arduino IDE coding, some basic electonics and to save some space for your miners (more performance, less design).

## Requirements

### Hardware

- PCB
- ESP32-WROOM-32, ESP32-Devkit1 (30 pin Board)
- ESP8266 D1 Mini
- 0.96 inch OLED SSD1306 Display I2C 128x64
- Electrolytic capacitor 220uF
- Jack barrel connector
- 5V power adapter (minimum 2A recommended)

You can also power your RIG with USB micro cable connected to ESP8266 instead of barrel connector.

### Software

- [Arduino IDE 2.0](https://www.arduino.cc/en/software)
- [NerdMiner v2 Firmware](https://github.com/BitMaker-hub/NerdMiner_v2) or [NM2 flasher online](https://flasher.bitronics.store/)

## Build

- Download gerber files and use PCB manufacturer of your choice
- There is also super simple acrylic "case" which you can use to have a nice piece standing around
- Solder all components in place, follow scheme and PCB markings
- Follow [instructions](https://github.com/BitMaker-hub/NerdMiner_v2?tab=readme-ov-file#flash-firmware) to flash NerdMiner Firmware for each ESP32

#### ESP8266 configuration

Open project in Arduino IDE and change your Wi-fi and password credentials.

*README update in progress...*

