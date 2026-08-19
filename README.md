# ENGG1100 Station Keeper

Station Keeper is an offline iPhone controller for Team Lavender's floating evacuation-centre prototype. A standard ESP32 (ESP32-D0WD-V3 DevKit, `esp32dev`) creates its own Wi-Fi network, drives four corner tether winches, and reads a GY-521 (MPU6050) tilt sensor. The phone interface provides proportional drive, manual per-corner tether payout/retrieval, a motor speed control, four motor-status cards, pitch and roll, a display-only vertical-motion estimate, and latched emergency stop. The project will later move to an ESP32-S3 N16R8; the `esp32-s3-*` environments exist for that but are not physically wired yet.

The prototype has no separate propulsion system. A 60 RPM N20 gear motor sits at each of the house's four corners, each with a 3D-printed pulley spooling fishing line to a fixed anchor point in that corner's direction. Reeling a corner's line in pulls the house that way; the opposing corner(s) must pay their line out at the same time, so all station-keeping and station-return movement comes from differentially driving the four winches — there are no poles or rigid channels, matching the project's dimensional constraints.

The MPU6050 can indicate short **RISING**, **FALLING**, and **STEADY** movements. The first upward or downward movement is detected sensitively and held on screen for about 1.2 seconds so it is visible on the phone. It does not measure water height or rope tension, and its estimate never controls a motor. A very slow constant water-level change may appear steady because an accelerometer cannot determine absolute height.

## Wi-Fi and controller address

| Setting | Value |
|---|---|
| Network name (SSID) | `ENGG1100-Lavender` |
| Password | `station1100` |
| Controller address | `http://192.168.4.1/` |
| Internet required | No |

## Complete signal wiring

Use the GPIO numbers printed below, not a board vendor's `D` numbers. Each corner winch is one 60 RPM N20 gear motor with a 3D-printed pulley spooling fishing line to a fixed anchor point in that corner's direction. Two dual-channel L9110S boards drive all four corner winches; there is no separate propulsion motor set. Pin numbers below are for the standard ESP32 (`esp32-hardware`); see `include/config.h` for the placeholder ESP32-S3 map.

| Device | Terminal | Standard ESP32 |
|---|---|---:|
| L9110S board 1 (front-left / rear-left), channel A | `A-IA`, `A-IB` | GPIO `13`, GPIO `14` |
| L9110S board 1 (front-left / rear-left), channel B | `B-IA`, `B-IB` | GPIO `16`, GPIO `17` |
| L9110S board 2 (front-right / rear-right), channel A | `A-IA`, `A-IB` | GPIO `18`, GPIO `19` |
| L9110S board 2 (front-right / rear-right), channel B | `B-IA`, `B-IB` | GPIO `25`, GPIO `26` |
| GY-521 (MPU6050) | `SDA`, `SCL` | GPIO `21`, GPIO `22` |
| GY-521 (MPU6050) | `VCC`, `GND` | `3V3`, `GND` |
| GY-521 (MPU6050) | `XDA`, `XCL`, `AD0`, `INT` | Not connected |

```text
STANDARD ESP32                    SIGNAL CONNECTIONS

GPIO 13, 14  ------------------> L9110S #1 channel A (A-IA/A-IB) --> front-left winch motor
GPIO 16, 17  ------------------> L9110S #1 channel B (B-IA/B-IB) --> rear-left winch motor
GPIO 18, 19  ------------------> L9110S #2 channel A (A-IA/A-IB) --> front-right winch motor
GPIO 25, 26  ------------------> L9110S #2 channel B (B-IA/B-IB) --> rear-right winch motor
GPIO 21      ------------------> GY-521 SDA
GPIO 22      ------------------> GY-521 SCL
3V3          ------------------> GY-521 VCC
GND          ------------------> GY-521 GND
```

Do not use GPIO 6-11 on the standard ESP32: they are tied to the module's internal flash interface.

Connect each motor only to its driver's matching output pair (`A-OA/A-OB` or `B-OA/B-OB`). Motor polarity has not been physically verified yet. If a motor reels the wrong direction (pays out when it should retrieve, or vice versa), the fix is a one-line software change: flip that motor's `k*Inverted` flag in `include/config.h` — never rewire the GPIO input pair or edit the movement/joystick math. Fit a 10 kΩ pull-down resistor between every L9110S input and ground so the motors remain off while the ESP32 starts.

Mount the GY-521 rigidly above the water line, component side upward. Power it from **3.3 V only**. Do not connect it to 5 V or 12 V.

## Electrical power wiring

The ESP32 and every motor driver must share ground, but the ESP32 must receive regulated 5 V (or, on the current bench test, USB power only) — never the raw motor supply.

**Current temporary bench test wiring** (no fuse yet): an 8xAA battery holder feeds a female XT30 pigtail, which mates to a male XT30 pigtail on the harness. Male XT30 red/black go to a positive/negative Wago each; the positive Wago feeds `VCC` on both L9110S boards, the negative Wago feeds `GND` on both. ESP32 `GND` shares that same negative rail. The ESP32 itself is powered only through USB, separately from the AA pack. Because there is no fuse in this temporary setup, keep first tests brief, supervised, and one motor at a time — see the first-power procedure below.

**Final prototype wiring** (not yet built): a fixed 12 V / 2 A supply through XT30, fused, feeding the L9110S boards, with a separate 5 V buck converter powering the ESP32 from the same 12 V rail and common ground throughout.

```text
SUPPLIED 12 V / 2 A  (final prototype; bench test uses 8xAA instead, no fuse yet)
        +
        +---- 2 A fuse ---- master switch ---- normally-closed motor E-STOP ----+
        |                                                                    |
        |                                                                    +--> L9110S #1 VCC
        |                                                                    +--> L9110S #2 VCC
        |
        +---- fused branch ---- 12 V to regulated 5 V buck converter --------+--> ESP32 5V/VIN

SUPPLY NEGATIVE -------------------------------------------------------------+--> ESP32 GND
                                                                             +--> all L9110S GND
                                                                             +--> buck converter GND

ESP32 3V3 ----------------------------------------------------------------------> GY-521 VCC
ESP32 GND ----------------------------------------------------------------------> GY-521 GND
```

Never connect 12 V (or the AA pack) to an ESP32 GPIO, `3V3`, `5V`, or USB pin. Select drivers and wiring for each motor's stall current. The four corner winches, ESP32, sensor, and converter together must remain within the project-wide 12 V / 2 A limit. Check the worst-case current before putting the prototype in water.

## Build and upload

Four PlatformIO environments are defined in `platformio.ini`:

| Environment | Board | `TEST_MODE` | Use |
|---|---|---|---|
| `esp32-test` (default) | Standard ESP32 | 1 | Hardware-free; motors and MPU6050 simulated |
| `esp32-hardware` | Standard ESP32 | 0 | The board currently on the bench, physical L9110S + optional GY-521 |
| `esp32-s3-test` | ESP32-S3 N16R8 | 1 | Future board, hardware-free |
| `esp32-s3-hardware` | ESP32-S3 N16R8 | 0 | Future board; GPIO map in `config.h` is a placeholder, not yet wired |

```bash
cd ~/Desktop/ENGG1100
PLATFORMIO_CORE_DIR=.platformio .venv/bin/pio run -e esp32-test
PLATFORMIO_CORE_DIR=.platformio .venv/bin/pio run -e esp32-test -t upload
```

After checking the complete motor and power wiring, upload the hardware build to the standard ESP32 (`/dev/cu.usbserial-0001` at 460800 baud upload speed):

```bash
PLATFORMIO_CORE_DIR=.platformio .venv/bin/pio run -e esp32-hardware
PLATFORMIO_CORE_DIR=.platformio .venv/bin/pio run -e esp32-hardware -t upload
PLATFORMIO_CORE_DIR=.platformio .venv/bin/pio device monitor -e esp32-hardware
```

At startup the serial monitor reports the firmware mode, board type, all 8 motor pin assignments (with any configured inversion), the MPU6050 detection result, the Wi-Fi SSID/password, the IP address, emergency-stop state, and a confirmation that all motors are STOPPED.

## Operating instructions

1. Power on with the prototype supported and motor power (XT30) disconnected.
2. Join `ENGG1100-Lavender` on the iPhone using password `station1100`. Accept the no-internet warning and stay connected.
3. Open `http://192.168.4.1/`. Confirm **ONLINE** and **HARDWARE**; **TEST MODE** means motor outputs are simulated.
4. Float the house level and stationary, then press **SET LEVEL**. Nose-up pitch now reads positive.
5. Use the **SPEED** slider to set the commanded motor speed (starts at 20%, capped at the configured maximum of 35% for first bench testing).
6. Select **DRIVE**. Hold and drag the joystick to pull the house across the water: each corner winch retrieves or pays out its tether to produce the requested direction. Release the joystick to stop. Hold either rotate button to turn in place via diagonal corner pairs. The four motor-status cards show each corner's live commanded direction (IN / OUT / STOPPED) and power.
7. Select **WINCHES**. Hold **ALL OUT** as water rises and the tethers become tight. Hold **ALL IN** as water falls to remove slack without pulling the house down. Use the FL/FR/RL/RR controls to correct a single corner with unequal rope length, and to test each motor independently on first power-up.
8. Treat ↑ **RISING**, ↓ **FALLING**, and ● **STEADY** as short-term motion guidance only. Rocking, driving, or abrupt tilting can affect the estimate.
9. Press **STOP** for a normal stop. Press **EMERGENCY STOP** to latch all outputs off; remove the hazard before pressing **CLEAR E-STOP**. Clearing does not restart a motor.

All movement is hold-to-run. Release, pointer cancellation, tab switching, page hiding, Wi-Fi disconnection, invalid commands, STOP, E-stop, or a missing command heartbeat (~500 ms) stops the motors. The independent firmware dead-man timeout is 600 ms.

Before attaching lines, test each corner winch with the platform supported. Verify that **OUT** releases fishing line and **IN** winds it in. Fit spool end stops or limit switches and retain a physical motor-power E-stop because this firmware cannot detect line tension, jams, or end of travel.
