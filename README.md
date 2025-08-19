# NerdMiner RIG

**The NerdSoloMiner v2**

I designed a dedicated PCB to work as Mining RIG for up to 4 ESP Boards with NerdMiner v2 Software. No fancy display at this time (but still eye catching). An ESP8266 controls OLED display which shows current status of Mining Pool, date & time and your account statistics.

This project assumess learning some Arduino IDE coding, some basic electronics and to save some space for your miners (more performance, less design).

This project is meant to be working with The official Nerdminer pool site ([pool.nerdminers.org](https://pool.nerdminers.org/)) as ESP8266 code is tailored to parse information from this pool API. You can always omit OLED Display and ESP8266 and use "bare" PCB as rig. On the other hand you can stack up as many PCBs as you want, using 5V and GND pins, to save even more space.

## Requirements

### Hardware

- PCB
- ESP32-WROOM-32, ESP32-Devkit1 (30 pin Board)
- ESP8266 D1 Mini
- 0.96 inch OLED SSD1306 Display I2C 128x64
- Electrolytic capacitor 220uF
- Jack barrel connector
- 5V power adapter (minimum 2A recommended, more if more PCBs in use)

You can also power your RIG with USB micro cable connected to ESP8266 instead of barrel connector.

### Software

- [Arduino IDE 2.0](https://www.arduino.cc/en/software)
- [NM2 flasher online](https://flasher.bitronics.store/)

## Build

- Download gerber files and use PCB manufacturer of your choice.
- There is also super simple acrylic "case" which you can use to have a nice piece standing around.
- Solder all components in place, follow scheme and PCB markings.
- Follow instructions to flash NerdMiner Firmware using [microMiners Flashtool](https://github.com/BitMaker-hub/NerdMiner_v2?tab=readme-ov-file#microminers-flashtool-recommended) for each ESP32.
- *Note: You need to use firmware **NMV2 1.6.3 on ESP32-WROOM***.

#### Code configuration

Copy this repo or download latest release.

Rename ```secrets.example``` to ```secrets.h```. Then open project (```main.ino```) in Arduino IDE. Inside secrets file you need to add your Wi-Fi credentials, set up a time zone and BTC wallet address.

#### Programming

In Arduino IDE, install ESP8266 board support via Boards Manager and download the required libraries, then select your ESP8266 board before uploading code.

1. Open Arduino IDE → File → Preferences, add this URL to Additional Boards Manager URLs:
http://arduino.esp8266.com/stable/package_esp8266com_index.json

2. Go to Tools → Board → Boards Manager, search ESP8266 and click Install.

3. Select your board under Tools → Board → ESP8266 Boards (D1 Mini clone).

4. Install required libraries via Sketch → Include Library → Manage Libraries.

5. Connect board by USB, select correct Port, and upload code.

