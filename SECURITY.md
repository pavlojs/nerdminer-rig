# Security Policy

## Supported versions

Only the current code on the `main` branch is supported. The project has no releases.

## Reporting a vulnerability

Report vulnerabilities privately through GitHub: open the **Security** tab of this repository and choose **Report a vulnerability**. Please do not open a public issue for them.

## What to know before you deploy

- **Secrets in the firmware.** `main/secrets.h` holds your Wi-Fi password and the OTA password. Git ignores the file, but its values are compiled into the firmware, so anyone with physical access to the D1 Mini can read them from its flash.
- **Pool API traffic.** The display connects to the pool over HTTPS without verifying the server certificate, and over plain HTTP when `POOL_URL` starts with `http://`. Someone who can intercept that traffic can change the stats on the display and see which wallet address it asks for. The display only reads public stats and never sends credentials to the pool.
- **Updates over Wi-Fi.** OTA is off while `OTA_PASSWORD` is empty. When it is on, the board accepts new firmware from anyone on the local network who knows the password. ArduinoOTA checks the password with an MD5 challenge and sends the firmware unencrypted, so use a long random password and keep the board on a network you trust.
- **Miner firmware.** `pio run -e miner -t flash_miner` downloads the NerdMiner image from its GitHub release and flashes it only if its SHA-256 matches the value pinned in `scripts/flash_miner.py`.
