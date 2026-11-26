# ESP32 Roomba 612 Web Controller

Drive a Roomba 612 over Wi-Fi from any browser. An ESP32 talks to the vacuum's serial port using iRobot's Open Interface protocol and serves a simple web page with GO / BACK / LEFT / RIGHT / STOP buttons.

Written in C-style code for the Arduino IDE.

![Platform](https://img.shields.io/badge/platform-ESP32-blue) ![IDE](https://img.shields.io/badge/IDE-Arduino-teal)

## How it works

```
Browser ──Wi-Fi──> ESP32 ──serial @19200 8N1──> Roomba Mini-DIN port
                  WebServer.on("/cmd")          OI opcode 137 (DRIVE)
```

- The ESP32 joins your Wi-Fi network and runs a small web server (port 80).
- Pressing a button issues `GET /cmd?cmd=<command>`.
- Each command maps to an iRobot OI **DRIVE** packet (opcode 137) with signed 16-bit big-endian velocity/radius values.
  - Straight driving uses radius 32768; in-place turns use radius ±1.
- Startup sends opcode **128 (START)** then **131 (SAFE)** so the robot accepts motor commands. Cliff and bump sensors remain active in safe mode.

## Hardware

### Bill of materials

- Roomba 612 (works with other 500/600-series Roombas using the same OI protocol)
- ESP32 development board (e.g. ESP32 DevKit v1)
- Mini-DIN 7-pin connector (or discrete wires inserted into the Roomba's port)
- 10 kΩ and 20 kΩ resistors (voltage divider)
- USB cable / power for the ESP32

### Wiring

Roomba Mini-DIN 7-pin port:

| DIN pin | Signal | ESP32 |
|---|---|---|
| 1 | Roomba TX | GPIO16 (RX2) **via voltage divider** (10k series, 20k to GND) |
| 2 | Roomba RX | GPIO17 (TX2) directly |
| 3 | Device Detect | Not connected (600-series defaults to 19200 baud) |
| 4 | GND | GND (common ground required) |
| 5–7 | Battery (~16 V) | **Do not connect** |

**Important:** never wire Roomba battery power directly to the ESP32. Power the ESP32 from USB or a proper buck converter. The voltage divider is mandatory — the Roomba's TX outputs ~5 V logic while the ESP32 pins are 3.3 V.

## Software setup

1. **Arduino IDE** with the ESP32 board package (*Boards Manager → esp32 by Espressif Systems*).
2. Clone this repo and open `roomba_web.ino`.
3. Select board **ESP32 Dev Module** and the correct serial port.

### WiFi credentials (kept out of the repo)

Credentials are loaded from `secrets.h`, which is gitignored:

1. Copy `secrets.h.example` to `secrets.h`.
2. Fill in your SSID and password.

On first connection the ESP32's IP address is printed to the Serial Monitor (115200 baud). Bookmark it, or reserve it in your router's DHCP settings.

## Flashing

1. Power on the Roomba.
2. Upload the sketch.
3. Open Serial Monitor at **115200** and press the ESP32's reset button if the IP doesn't appear.
4. Browse to the printed IP address.

## Usage

| Button | Action | DRIVE parameters |
|---|---|---|
| GO | Drive straight forward | 250 mm/s, radius 32768 |
| BACK | Reverse straight | −250 mm/s, radius 32768 |
| LEFT | Spin counter-clockwise | 200 mm/s, radius +1 |
| RIGHT | Spin clockwise | 200 mm/s, radius −1 |
| STOP | Halt immediately | 0 mm/s, radius 0 |

## Troubleshooting

| Symptom | Likely cause / fix |
|---|---|
| One beep, no reaction to commands | Robot still in passive mode — verify the 128→131 handshake ran |
| No beeps at all | Check TX/RX not swapped, common ground present, Roomba on battery power (not just docked) |
| Upload fails: port busy / permission denied | Add your user to `dialout`; on Debian remove `brltty` |
| Roomba ignores commands entirely | May be set to 57 600 baud — pulse the DD line (pin 3) low 3× during boot to force 19200 |
| Can't reach web page | Confirm IP on Serial Monitor; same network for phone/laptop |

## Safety notes

- Safe mode keeps cliff sensors active, but don't leave the robot unattended near stairs while testing.
- Put the Roomba on blocks or a table edge-guard when first testing spin commands — wheels will move as soon as the page loads a button press.
- Disable `OI_SAFE` (switch to opcode 132 `FULL`) only if you understand the safety sensors will be ignored.

## References

- [iRobot Roomba 600 Open Interface Specification](https://www.irobotweb.com/-/media/MainSite/Documents/About/Roomba600_Open_Interface_Spec.pdf) — full OI protocol documentation
- ESP32 WebServer library documentation

## License

MIT
