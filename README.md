# ENGG1100 Station Keeper

Station Keeper is an offline iPhone controller for Team Lavender's floating evacuation-centre prototype. An ESP32-WROOM-32U creates its own Wi-Fi network, drives four corner tether winches, and reads a Jaycar XC3732 / Keyestudio KS0270 (MMA8452Q) tilt sensor. The phone interface provides proportional drive, manual per-corner tether payout/retrieval, pitch and roll, a display-only vertical-motion estimate, and latched emergency stop.

The prototype has no separate propulsion system. A 60 RPM N20 gear motor sits at each of the house's four corners, each with a 3D-printed pulley spooling fishing line to a fixed anchor point in that corner's direction. Reeling a corner's line in pulls the house that way; the opposing corner(s) must pay their line out at the same time, so all station-keeping and station-return movement comes from differentially driving the four winches — there are no poles or rigid channels, matching the project's dimensional constraints.

The XC3732 can indicate short **RISING**, **FALLING**, and **STEADY** movements. The first upward or downward movement is detected sensitively and held on screen for about 1.2 seconds so it is visible on the phone. It does not measure water height or rope tension, and its estimate never controls a motor. A very slow constant water-level change may appear steady because an accelerometer cannot determine absolute height.

## Wi-Fi and controller address

| Setting | Value |
|---|---|
| Network name (SSID) | `ENGG1100-Lavender` |
| Password | `station1100` |
| Controller address | `http://192.168.4.1/` |
| Internet required | No |

## Complete signal wiring

Use the GPIO numbers printed below, not a board vendor's `D` numbers. Each corner winch is one 60 RPM N20 gear motor with a 3D-printed pulley spooling fishing line to a fixed anchor point in that corner's direction. Two dual-channel L9110S boards drive all four corner winches; there is no separate propulsion motor set.

| Device | Terminal | ESP32-WROOM-32U |
|---|---|---:|
| L9110S board 1, front-left winch channel | `A-IA`, `A-IB` | GPIO `13`, GPIO `14` |
| L9110S board 1, front-right winch channel | `B-IA`, `B-IB` | GPIO `16`, GPIO `17` |
| L9110S board 2, rear-left winch channel | `A-IA`, `A-IB` | GPIO `18`, GPIO `19` |
| L9110S board 2, rear-right winch channel | `B-IA`, `B-IB` | GPIO `25`, GPIO `26` |
| XC3732 / KS0270 | `SDA`, `SCL` | GPIO `21`, GPIO `22` |
| XC3732 / KS0270 | `VCC`, `GND` | `3V3`, `GND` |
| XC3732 / KS0270 | `INT1`, `INT2` | Not connected; optional interrupts are not used |

```text
ESP32-WROOM-32U                   SIGNAL CONNECTIONS

GPIO 13, 14  ------------------> L9110S #1 channel A --> front-left winch motor
GPIO 16, 17  ------------------> L9110S #1 channel B --> front-right winch motor
GPIO 18, 19  ------------------> L9110S #2 channel A --> rear-left winch motor
GPIO 25, 26  ------------------> L9110S #2 channel B --> rear-right winch motor
GPIO 21      ------------------> XC3732 SDA
GPIO 22      ------------------> XC3732 SCL
3V3          ------------------> XC3732 VCC
GND          ------------------> XC3732 GND
```

Connect each motor only to its driver's matching output pair (`A-OA/A-OB` or `B-OA/B-OB`). If a motor reels the wrong direction (pays out when it should retrieve, or vice versa), swap that motor's two output wires; do not change its GPIO input pair. Fit a 10 kΩ pull-down resistor between every L9110S input and ground so the motors remain off while the ESP32 starts.

Mount the XC3732 rigidly above the water line, component side upward, with its printed X axis pointing forward and Y axis pointing right. Power it from **3.3 V only**. Do not connect it to 5 V or 12 V.

## Electrical power wiring

The ESP32 and every motor driver must share ground, but the ESP32 must receive regulated 5 V—not the raw 12 V motor supply.

```text
SUPPLIED 12 V / 2 A
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

ESP32 3V3 ----------------------------------------------------------------------> XC3732 VCC
ESP32 GND ----------------------------------------------------------------------> XC3732 GND
```

Never connect 12 V to an ESP32 GPIO, `3V3`, `5V`, or USB pin. Select drivers and wiring for each motor's stall current. The four corner winches, ESP32, sensor, and converter together must remain within the project-wide 12 V / 2 A limit. Check the worst-case current before putting the prototype in water.

## Build and upload

The safe test build reads a real XC3732 when present but simulates all motor outputs:

```bash
cd ~/Desktop/ENGG1100
PLATFORMIO_CORE_DIR=.platformio .venv/bin/pio run -e esp32-test
PLATFORMIO_CORE_DIR=.platformio .venv/bin/pio run -e esp32-test -t upload
```

After checking the complete motor and power wiring, upload the hardware build:

```bash
PLATFORMIO_CORE_DIR=.platformio .venv/bin/pio run -e esp32-hardware
PLATFORMIO_CORE_DIR=.platformio .venv/bin/pio run -e esp32-hardware -t upload
PLATFORMIO_CORE_DIR=.platformio .venv/bin/pio device monitor -e esp32-hardware
```

At startup the serial monitor should report the MMA8452Q at `0x1D` or `0x1C`, the selected operating mode, the Wi-Fi name, and `http://192.168.4.1/`.

## Operating instructions

1. Power on with the prototype supported and the motor-power switch off.
2. Join `ENGG1100-Lavender` on the iPhone using password `station1100`. Accept the no-internet warning and stay connected.
3. Open `http://192.168.4.1/`. Confirm **ONLINE** and **HARDWARE**; **TEST MODE** means motor outputs are simulated.
4. Float the house level and stationary, then press **SET LEVEL**. Nose-up pitch now reads positive.
5. Select **DRIVE**. Hold and drag the joystick to pull the house across the water: each corner winch retrieves or pays out its tether to produce the requested direction. Release the joystick to stop. Hold either rotate button to turn in place via diagonal corner pairs.
6. Select **WINCHES**. Hold **ALL OUT** as water rises and the tethers become tight. Hold **ALL IN** as water falls to remove slack without pulling the house down. Use the FL/FR/RL/RR controls to correct a single corner with unequal rope length.
7. Treat ↑ **RISING**, ↓ **FALLING**, and ● **STEADY** as short-term motion guidance only. Rocking, driving, or abrupt tilting can affect the estimate.
8. Press **STOP** for a normal stop. Press **EMERGENCY STOP** to latch all outputs off; remove the hazard before pressing **CLEAR E-STOP**. Clearing does not restart a motor.

All movement is hold-to-run. Release, pointer cancellation, tab switching, page hiding, Wi-Fi disconnection, invalid commands, STOP, E-stop, or a missing command heartbeat stops the motors. The independent firmware dead-man timeout is 600 ms.

Before attaching lines, test each corner winch with the platform supported. Verify that **OUT** releases fishing line and **IN** winds it in. Fit spool end stops or limit switches and retain a physical motor-power E-stop because this firmware cannot detect line tension, jams, or end of travel.
