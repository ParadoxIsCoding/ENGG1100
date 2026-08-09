# ENGG1100 Station-Keeping Controller

This repository contains the ESP32-S3 firmware and phone-friendly web controller for Team Lavender's station-keeping flood-resistant evacuation-centre prototype. The ESP32 creates its own Wi-Fi network and serves the control page directly, so the controller does **not** need an internet connection while operating.

The default build is **test mode**. It runs the complete Wi-Fi, web interface, command, dead-man timeout, and emergency-stop logic without enabling any motor GPIO. This makes it safe to test with only an ESP32-S3 and USB cable.

> [!CAUTION]
> **Never power an N20 motor from the ESP32, its 3.3 V pin, or its 5 V pin.** The motors will use the separate 12 V supply through the L9110S motor-driver boards. The ESP32 will receive regulated 5 V from the buck converter. Join the ESP32, buck-converter, 12 V supply, and L9110S grounds at a common ground; do not join the 12 V positive rail to the ESP32.

## File structure

```text
ENGG1100/
├── platformio.ini                 PlatformIO board and build environments
├── include/
│   ├── config.h                   Wi-Fi, timeout, GPIO, and future I2C settings
│   ├── attitude_sensor.h          Pitch/roll interface and level calibration
│   ├── motor_controller.h         Motion interface
│   └── web_page.h                 Embedded offline mobile web app
├── src/
│   ├── main.cpp                   Wi-Fi, HTTP API, safety logic, and startup
│   ├── attitude_sensor.cpp        Temporary demo attitude source / IMU hook
│   └── motor_controller.cpp       Four-motor mixing and safe output control
└── README.md                      Setup, use, wiring, and troubleshooting
```

No external Arduino libraries, CDNs, fonts, or cloud services are used. The Arduino framework components (`WiFi` and `WebServer`) are supplied by the ESP32 PlatformIO platform.

## 1. Install PlatformIO

### Visual Studio Code method

1. Install [Visual Studio Code](https://code.visualstudio.com/).
2. Open Extensions (`Shift+Command+X` on macOS).
3. Search for **PlatformIO IDE** and select **Install**.
4. Restart VS Code if prompted.
5. Select **File > Open Folder…** and open this repository.
6. Wait for PlatformIO to finish preparing the project. The first setup/build needs internet access to download the ESP32 toolchain; normal use of the flashed controller does not.

### Command-line method

Install PlatformIO Core in a project-local Python environment, then verify it:

```bash
cd ~/Desktop/ENGG1100
python3 -m venv .venv
.venv/bin/python -m pip install platformio
.venv/bin/pio --version
```

On this Mac, the system command named `pio` is an unrelated Python package, not PlatformIO. Use `.venv/bin/pio` for the commands below (or activate the environment with `source .venv/bin/activate` first).

## 2. Connect and identify the ESP32-S3

1. Use a USB **data** cable, not a charge-only cable.
2. Connect the cable to the ESP32-S3 USB/serial port. Some boards have separate `USB` and `UART` sockets; use the socket recommended for flashing by the board manufacturer, normally `UART` for a DevKitC-1.
3. In VS Code, select the PlatformIO alien-head icon, expand **Project Tasks > esp32-s3-test > General**, and select **Devices**.
4. Or run:

   ```bash
   .venv/bin/pio device list
   ```

Look for a newly appearing port such as `/dev/cu.usbmodem…` or `/dev/cu.usbserial…`. Disconnecting and reconnecting the board is a simple way to confirm which entry is the ESP32-S3.

If PlatformIO chooses the wrong port, add the exact detected port temporarily under the relevant environment in `platformio.ini`:

```ini
upload_port = /dev/cu.usbmodemXXXX
monitor_port = /dev/cu.usbmodemXXXX
```

Do not commit a machine-specific port name.

## 3. Compile

The default test build requires no attached hardware:

```bash
cd ~/Desktop/ENGG1100
.venv/bin/pio run -e esp32-s3-test
```

In VS Code, use **PlatformIO > Project Tasks > esp32-s3-test > General > Build**, or select the check-mark button in the bottom status bar (the default environment is test mode).

To compile the future physical motor build:

```bash
.venv/bin/pio run -e esp32-s3-hardware
```

## 4. Upload

For the safe default test build:

```bash
.venv/bin/pio run -e esp32-s3-test -t upload
```

In VS Code, use **PlatformIO > Project Tasks > esp32-s3-test > General > Upload**, or select the right-arrow button in the bottom status bar.

Only after completing and checking the power and driver wiring, upload hardware mode with:

```bash
.venv/bin/pio run -e esp32-s3-hardware -t upload
```

In VS Code, use **PlatformIO > Project Tasks > esp32-s3-hardware > General > Upload**. The environment chosen for upload—not the most recent build—determines whether GPIO is enabled.

## 5. Open the Serial Monitor

After uploading, run:

```bash
.venv/bin/pio device monitor -e esp32-s3-test
```

Or use **PlatformIO > Project Tasks > esp32-s3-test > General > Monitor** / the plug icon in the VS Code status bar. The baud rate is **115200**. Press the board's `RESET`/`EN` button if the startup text has already passed. Expected output includes:

```text
Mode: TEST (no motor hardware required)
Wi-Fi: ENGG1100-StationKeeper
Open: http://192.168.4.1/
Dead-man timeout: 600 ms
```

Exit the command-line monitor with `Control+C` before trying to upload again, because an open monitor can hold the serial port.

## 6. Connect a phone and open the web app

1. Power the ESP32-S3 and wait for it to start.
2. On the phone, open Wi-Fi settings.
3. Join **ENGG1100-StationKeeper**.
4. Enter the password **`station1100`**.
5. If the phone warns that this Wi-Fi has no internet, choose **Stay Connected**, **Use Without Internet**, or the equivalent option. Temporarily disable automatic switching to mobile data if the phone leaves the network.
6. Open a browser and enter **`http://192.168.4.1/`** exactly. Use `http`, not `https`.

The page is stored in the firmware. It is responsive, uses large touch targets, respects phone safe areas, and has a compact layout for short screens.

## Controls and safety behaviour

The movement joystick is hold-to-run. Dragging farther from the centre proportionally increases motor power; horizontal and vertical input are mixed so diagonal movement/turning is smooth. The central 12% is a dead zone and commands all motors to stop. The **Rotate left** and **Rotate right** buttons remain hold-to-run. Pressing **STOP** immediately stops every motor. Pressing **EMERGENCY STOP** stops every motor and latches the stop; movement commands are rejected until **Clear emergency stop** is pressed. Clearing the latch leaves the motors stopped.

While the joystick is held, the phone refreshes its X/Y command every 200 ms (the rotate buttons refresh every 250 ms). A dedicated firmware safety task checks every 20 ms and stops all motors if no valid movement command arrives for 600 ms; it remains independent of web-request handling. Releasing/cancelling the touch, losing pointer capture, hiding/leaving the page, or moving the browser out of focus also sends a stop. The ESP32 additionally stops immediately when a phone disconnects from its Wi-Fi access point. If an interrupted request does not produce a Wi-Fi disconnect event, the independent firmware dead-man timeout remains the final stop path.

Every motion command therefore has these stop paths:

- Release or cancel the held joystick/button.
- The on-screen **STOP** button.
- The latched **EMERGENCY STOP** button.
- The 600 ms firmware dead-man timeout.
- A Wi-Fi client disconnect event.
- Any invalid/missing motion request or unknown route while moving.
- Reset or power loss. Hardware-mode startup sets the output latch low before enabling each GPIO and then explicitly stops every channel.

> [!NOTE]
> A software emergency stop is not a substitute for a physical, normally-closed power-isolation switch in the 12 V motor circuit. Use a physical motor-power disconnect during bench testing and in the final prototype.

## What can be tested without external hardware

With the default `esp32-s3-test` firmware and only an ESP32-S3 plus USB cable, you can test:

- Access-point creation, password, phone connection, and offline page loading.
- Mobile layout, proportional joystick movement and dead zone, rotate buttons, release-to-stop behaviour, STOP, and emergency-stop latching/clearing.
- Command receipt in the Serial Monitor; motor messages end with `(simulated)`.
- The dead-man timeout by holding a movement control and then disabling the phone's Wi-Fi or moving out of range. The Serial Monitor should show `[safety] stop: dead-man timeout` within about 600 ms of the last received command.
- Rejection of movement while emergency stop is latched.

Test mode never configures the motor pins as outputs. No L9110S, motor, MPU6050, 12 V supply, or buck converter needs to be attached.

## Switch from test mode to hardware mode

1. Disconnect the 12 V motor supply and keep the physical motor-power switch off.
2. Confirm the exact ESP32-S3 board pinout and update the pins in `include/config.h` if required.
3. Complete the future wiring below and use a multimeter to set the buck converter to **5.0 V before connecting it to the ESP32**.
4. Compile and upload the `esp32-s3-hardware` environment:

   ```bash
   .venv/bin/pio run -e esp32-s3-hardware
   .venv/bin/pio run -e esp32-s3-hardware -t upload
   .venv/bin/pio device monitor -e esp32-s3-hardware
   ```

5. Confirm the monitor prints `Mode: HARDWARE` and `[motors] Hardware GPIO enabled; all outputs LOW`.
6. With 12 V still isolated, verify all eight L9110S logic inputs are low at rest.
7. Test one motor at a time with the platform lifted/secured and current limiting enabled. If a motor's physical direction is reversed, swap that motor's two L9110S output wires; do not change the safety logic.

`TEST_MODE` is set per environment in `platformio.ini`; no source edit is needed.

## Future L9110S wiring

The design assumes four N20 motors and two dual-channel L9110S boards. Labels vary (`A-IA`/`A-IB`, `B-IA`/`B-IB`, `VCC`, `GND`, `OA`/`OB`), so verify the markings and datasheet for the exact modules.

| Function | ESP32-S3 GPIO | L9110S connection |
|---|---:|---|
| Front-left input A | 4 | Board 1, channel A input 1 |
| Front-left input B | 5 | Board 1, channel A input 2 |
| Front-right input A | 6 | Board 1, channel B input 1 |
| Front-right input B | 7 | Board 1, channel B input 2 |
| Rear-left input A | 15 | Board 2, channel A input 1 |
| Rear-left input B | 16 | Board 2, channel A input 2 |
| Rear-right input A | 17 | Board 2, channel B input 1 |
| Rear-right input B | 18 | Board 2, channel B input 2 |
| Motor supply | — | Separate 12 V positive to both L9110S `VCC`/motor-supply inputs |
| Common ground | ESP32 GND | Both L9110S grounds, buck ground, and 12 V negative |
| Motor outputs | — | Each N20 motor connects only to its L9110S channel output pair |

Power arrangement:

```text
12 V supply + ── fuse / physical stop ──┬── L9110S board 1 motor supply
                                        ├── L9110S board 2 motor supply
                                        └── buck converter IN+
12 V supply - ──────────────────────────┬── both L9110S GND
                                        └── buck converter IN-
buck OUT+ (regulated 5.0 V) ─────────────── ESP32 5V/VIN
buck OUT- ───────────────────────────────── ESP32 GND (common ground)
```

Add a 10 kΩ pull-down from each of the eight L9110S input lines to ground so the drivers remain off while the ESP32 is resetting or before firmware takes control. Confirm that the selected L9110S modules tolerate the 12 V motor supply and that the combined motor stall current stays within the drivers, wiring, supply, and project limits. L9110S boards are not a regulated source for the ESP32.

The motion mix is a starting point and must be verified against the real motor orientation. All direction changes first stop every channel. Mechanical placement may require swapping individual motor output wires or revising the mix after controlled testing.

## Future MPU6050 wiring

The MPU6050 is reserved for later station-keeping feedback and is not accessed by this firmware yet.

The web controller currently runs a smooth pitch/roll demo so the artificial horizon and **Set Level** calibration can be tested without sensor hardware. To connect the physical IMU, initialise it in `AttitudeSensor::begin()` and replace the two clearly labelled placeholder assignments in `AttitudeSensor::update()` (`src/attitude_sensor.cpp`) with filtered pitch and roll values in degrees. Then set `kUseSmoothDemoMode` to `false`. The web API and interface need no other changes.

| MPU6050 | ESP32-S3 | Notes |
|---|---:|---|
| VCC | 3.3 V | Use 3.3 V unless the exact breakout explicitly supports another input voltage |
| GND | GND | Common ground |
| SDA | GPIO 8 | Reserved in `include/config.h` |
| SCL | GPIO 9 | Reserved in `include/config.h` |
| AD0 | GND | Default I2C address `0x68` |
| INT | Not connected | Can be assigned later if interrupt-driven sampling is required |

Keep I2C wires short and route them away from motor wires. Many breakout boards contain pull-ups, so check before adding more. Motor noise may require local decoupling, twisted motor leads, and filtering after measurement.

## GPIO warning

ESP32-S3 development boards do not all expose the same pins. Before hardware mode, check the schematic/pinout for the **exact board and revision**. Avoid changing motor outputs to boot-strapping pins (especially GPIO 0, 3, 45, and 46), the native USB pair (commonly GPIO 19/20), pins used by onboard flash/PSRAM, or pins already connected to onboard LEDs/peripherals. Some pins can pulse or float during reset; the recommended external 10 kΩ pull-downs keep the L9110S inputs off before `setup()` runs.

Do not assume a pin labelled `D4` is GPIO 4. Use the GPIO number printed in the board documentation and then update `include/config.h`.

## Common upload troubleshooting

- **No serial port:** try a known USB data cable, another USB port, avoid an unpowered hub, install any USB-UART driver required by the exact board, and compare `pio device list` before/after reconnection.
- **Wrong target:** confirm the board is an ESP32-S3 DevKitC-1 or change `board = esp32-s3-devkitc-1` in `platformio.ini` to its exact PlatformIO board ID.
- **Port busy / resource busy:** close the PlatformIO Serial Monitor and any Arduino IDE monitor, then upload again.
- **Cannot enter download mode / “No serial data received”:** hold the **BOOT** button, press and release **RESET/EN**, start the upload, and release **BOOT** when PlatformIO prints `Connecting...` (or once writing begins). On boards without RESET, hold BOOT while briefly reconnecting USB.
- **Upload starts but is unreliable:** lower `upload_speed` in `platformio.ini` from `460800` to `115200`, shorten the USB cable, and disconnect noisy motor power while flashing.
- **Monitor shows unreadable text:** use 115200 baud.
- **Phone cannot load the page:** remain connected to the ESP32 Wi-Fi despite the no-internet warning, enter `http://192.168.4.1/`, disable VPN/private relay for this local test if it blocks local addresses, and confirm the Serial Monitor shows the access point started.
- **Upload succeeds but old behaviour remains:** verify the environment name in the upload command, press RESET/EN, and watch the printed `Mode:` line.
- **Board resets when motors start:** immediately isolate motor power. Recheck separate power paths, common ground, buck regulation, motor stall current, supply current limit, wiring, and noise suppression. Never solve this by powering motors from the ESP32.

## Bench-test order

1. Build and upload test mode with no external hardware.
2. Exercise the joystick across its full range and dead zone, then every other control, and verify serial stop messages.
3. Verify the dead-man timeout by breaking the phone connection while moving.
4. Latch emergency stop, confirm all movement is rejected, clear it, and confirm motion remains stopped.
5. Wire drivers with 12 V isolated; verify 5 V buck output and idle input levels using a multimeter.
6. Upload hardware mode and test one secured motor at a time using current limiting.
7. Only then test the full four-motor mix, physical stop, and station-keeping system.
