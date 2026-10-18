# NerdMiner RIG

**The NerdSoloMiner v2**

I designed a dedicated PCB to serve as a mining RIG for up to 4 ESP boards running the NerdMiner v2 software. No fancy display this time (but still eye-catching). An ESP8266 controls an OLED display that shows the current mining pool status, date & time, and your account statistics.

This project assumes some familiarity with the Arduino IDE, basic electronics, and is designed to help save space for your miners (more performance, less bulk).

The ESP8266 reads the statistics from the public API of the pool your miners use: [public-pool.io](https://web.public-pool.io/) (the NerdMiner default) or your own [public-pool](https://github.com/benjamin-wilson/public-pool) instance, and ckpool pools such as the official NerdMiner pool [pool.nerdminers.org](https://pool.nerdminers.org/) or [solo.ckpool.org](https://solo.ckpool.org/). You can omit the OLED display and ESP8266 entirely and use the “bare” PCB as a rig. On the other hand, you can stack multiple PCBs together, using the 5V and GND pins, to save even more space.

## Requirements

### Hardware

- PCB  
- ESP32-WROOM-32, ESP32-DevKit1 (30-pin board)  
- ESP8266 D1 Mini  
- 0.96-inch OLED SSD1306 Display I2C 128×64  
- Electrolytic capacitor 220µF  
- Barrel jack connector  
- 5V power adapter (minimum 2A recommended; more if multiple PCBs are used)  

You can also power your RIG with a micro-USB cable connected to the ESP8266 instead of using the barrel jack connector.

### Schematics

![NerdMiner RIG Schematic](images/Schematic_Nerd-Miner-RIG.png)

### Software

- [VS Code](https://code.visualstudio.com/) with the [PlatformIO IDE](https://platformio.org/install/ide?install=vscode) extension, or [Arduino IDE 2.0](https://www.arduino.cc/en/software)  
- [NM2 online flasher](https://flasher.bitronics.store/)  

## Build

![NerdMiner RIG PCB](images/pcb_3d.png)

- Download the Gerber files and use a PCB manufacturer of your choice.  
- A super-simple acrylic “case” is also available if you want a nice desktop stand.  
- Solder all components in place, following the schematic and PCB markings.  
- Flash the NerdMiner firmware on each ESP32 using the [microMiners Flashtool](https://github.com/BitMaker-hub/NerdMiner_v2?tab=readme-ov-file#microminers-flashtool-recommended).  
- *Note: You need to use firmware **NMV2 1.6.3 on ESP32-WROOM***.  

#### Code configuration

Copy this repo or download the latest release.  

In the ```main``` folder, rename ```secrets.example``` to ```secrets.h```. Then open the repository folder in VS Code, or ```main/main.ino``` in Arduino IDE. Inside the secrets file, add your Wi-Fi credentials, set up a time zone, enter your BTC wallet address and choose the pool.  

The display shows the stats of the pool and wallet your miners actually mine on, so ```POOL_API``` and ```POOL_URL``` have to match the pool set in the NerdMiner configuration:

| Pool | Pool in NerdMiner | ```POOL_API``` | ```POOL_URL``` |
|---|---|---|---|
| public-pool.io (NerdMiner default) | ```public-pool.io```, port ```21496``` | ```PoolApi::PublicPool``` | ```https://public-pool.io:40557``` |
| pool.nerdminers.org | ```pool.nerdminers.org```, port ```3333``` | ```PoolApi::Ckpool``` | ```https://pool.nerdminers.org``` |
| solo.ckpool.org | ```stratum.ckpool.org```, port ```3333``` | ```PoolApi::Ckpool``` | ```https://solo.ckpool.org``` |

public-pool does not report share counts, so with it the *Your Stats* page shows workers and the best share only.  

```TIMEZONE``` takes a POSIX time zone string, which includes the daylight saving rules, so the clock switches on the right day by itself. Find the string for your zone in [posix_tz_db](https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv).  

*Updating from v1.1: copy the new settings from ```secrets.example``` into your ```secrets.h```. ```POOL_API```, ```POOL_URL```, ```TIMEZONE``` and ```OTA_PASSWORD``` are new, ```UTC_OFFSET``` and ```DST_REGION``` are gone.*  

#### Programming

##### VS Code with PlatformIO

Open the repository folder in VS Code with the PlatformIO IDE extension installed. On the first build PlatformIO downloads the ESP8266 platform and the library versions pinned in ```platformio.ini```.

Connect the D1 Mini by USB and use the PlatformIO toolbar, or run these commands in the PlatformIO terminal:

- Build: ```pio run```
- Upload: ```pio run -t upload```
- Serial monitor: ```pio device monitor```

##### Updates over Wi-Fi

Once the display runs, you can update it over Wi-Fi instead of opening the case:

1. Set ```OTA_PASSWORD``` in ```main/secrets.h``` (letters, digits, ```.```, ```-``` and ```_```) and upload once over USB. With an empty password, updates over Wi-Fi stay off.
2. Run ```pio run -e d1_mini_ota -t upload```, or *d1_mini_ota → Upload* in the PlatformIO sidebar. The display shows a progress bar and restarts with the new firmware.

The upload looks for the board as ```nerdminer-rig.local```. If your computer does not resolve ```.local``` names, use the IP address the board prints on the serial monitor after connecting to Wi-Fi: ```pio run -e d1_mini_ota -t upload --upload-port 192.168.1.50```. During the upload the board connects back to your computer on a random port between 10000 and 60000, so a firewall on the computer has to let that connection in.

##### Arduino IDE

In Arduino IDE, install ESP8266 board support via Boards Manager and download the required libraries. Then select your ESP8266 board before uploading the code.

1. Open **Arduino IDE → File → Preferences** and add this URL to *Additional Boards Manager URLs*:
```http://arduino.esp8266.com/stable/package_esp8266com_index.json```

2. Go to **Tools → Board → Boards Manager**, search for *ESP8266* and click *Install*.  

3. Select your board under **Tools → Board → ESP8266 Boards (D1 Mini clone)** and set **Tools → CPU Frequency** to *160 MHz*, which speeds up the encrypted connections to the pool.  

4. Install the required libraries via **Sketch → Include Library → Manage Libraries**: *Adafruit SSD1306*, *Adafruit GFX Library* and *ArduinoJson* (version 7).  

5. Connect the board by USB, select the correct Port, and upload the code.

#### Tests

Parsing of the pool API responses and the number formatting have unit tests that run on your computer, without a board. They need a C++ compiler such as GCC or Clang:

```pio test -e native```
