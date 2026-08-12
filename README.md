# ENGG1100 Station-Keeping Controller

This is the offline phone controller for Team Lavender's floating station-keeping prototype. It now targets the **ESP32-WROOM-32U** and directly supports the Jaycar **XC3732** / Keyestudio **KS0270** tri-axis tilt sensor. That module uses the **MMA8452Q** accelerometer—not the MPU6050 described by older revisions of this project.

The ESP32 creates the `ENGG1100-Lavender` Wi-Fi network and serves the controller page without an internet connection.

> [!IMPORTANT]
> The XC3732 measures the platform's tilt; it cannot measure water height or rope tension. Physical tether control requires two reversible geared winch/spool motors and two H-bridge channels. The controller includes safe manual two-winch controls ready for that hardware, but they remain simulated until those parts are added.

## What changed

- Uses the real MMA8452Q over I2C, checks its device ID, and automatically tries both valid addresses (`0x1D`, then `0x1C`).
- Reads live acceleration at 100 Hz, uses a faster low-pass response, updates the browser at 20 Hz, and calculates pitch and roll for the artificial horizon.
- Reverses the artificial horizon's vertical movement so its up/down response matches the requested display direction.
- Shows an explicit `IMU NOT FOUND` warning in hardware mode instead of silently displaying demo values.
- Adds coordinated hold-to-run **BOTH OUT** and **BOTH IN** controls plus independent left/right rope trim. They use the same STOP, emergency stop, Wi-Fi-disconnect stop, and 600 ms dead-man timeout as all other motor controls.
- Changes the project from the future ESP32-S3 configuration to the ESP32-WROOM-32U (`esp32dev`) configuration.

## 1. Wire the XC3732 now

Only four connections are required. Read the labels printed on the module; do not rely on the physical order of its pins.

| XC3732 / KS0270 label | ESP32-WROOM-32U pin | Purpose |
|---|---:|---|
| `VCC` | `3V3` | 3.3 V power |
| `GND` | `GND` | Common ground |
| `SDA` | GPIO `21` | I2C data |
| `SCL` | GPIO `22` | I2C clock |
| `INT1`, `INT2` | Not connected | Optional interrupts; this firmware polls I2C |

```text
ESP32-WROOM-32U                         Jaycar XC3732 / Keyestudio KS0270

3V3  ---------------------------------- VCC
GND  ---------------------------------- GND
GPIO 21 ------------------------------- SDA
GPIO 22 ------------------------------- SCL
                                           INT1  not connected
                                           INT2  not connected
```

Power the XC3732 from **3.3 V only**. The MMA8452Q's supply and I2C logic must not exceed 3.6 V, so do not use the ESP32's 5 V/VIN pin and never connect it to the 12 V motor rail. Jaycar specifies the XC3732 interface range as 1.6–3.6 V and confirms its I2C MMA8452Q design; the NXP datasheet lists `0x1C` and `0x1D` as the two possible 7-bit I2C addresses. [Jaycar XC3732 product page](https://www.jaycar.com.au/arduino-compatible-tri-axis-digital-tilt-sensor/p/XC3732), [NXP MMA8452Q datasheet](https://www.nxp.com/docs/en/data-sheet/MMA8452Q.pdf)

Mount the module rigidly, above the expected water line, with the component side facing up, its printed **X axis pointing forward** and **Y axis pointing to the right** of the house. Keep the I2C wires short and away from motor wiring. Once the house is floating level, open the controller and press **Set Level**. This zeroes the current pitch and roll without changing the physical sensor mounting.

## 2. Build and upload

The test build is the safe default: it never drives any GPIO motor outputs. It does use a connected XC3732 if present; if the sensor is not present it shows a smooth attitude demo so the page remains testable.

```bash
cd ~/Desktop/ENGG1100
.venv/bin/pio run -e esp32-test
.venv/bin/pio run -e esp32-test -t upload
.venv/bin/pio device monitor -e esp32-test
```

Expected serial output with the sensor connected includes:

```text
[attitude] MMA8452Q connected at 0x1D
Mode: TEST (no motor hardware required)
Wi-Fi: ENGG1100-Lavender
Open: http://192.168.4.1/
```

If the module is not detected, recheck the 3.3 V connection, common ground, `SDA`/`SCL` order, and solder joints. The firmware reports `MMA8452Q not found` rather than pretending the hardware is working.

After the sensor has been proven in test mode, the hardware build is:

```bash
.venv/bin/pio run -e esp32-hardware
.venv/bin/pio run -e esp32-hardware -t upload
.venv/bin/pio device monitor -e esp32-hardware
```

Do not upload `esp32-hardware` until the motor wiring, separate supply, fuse, and physical power-isolation switch have been checked.

## 3. Phone controls and safety

1. Join Wi-Fi network **ENGG1100-Lavender** with password **`station1100`**.
2. Remain connected despite the no-internet warning, then open `http://192.168.4.1/`.
3. With the platform floating level, press **Set Level**.
4. Use the movement joystick as before. It is proportional and hold-to-run.
5. When water rises, hold **BOTH OUT** just enough to prevent either tether becoming tight.
6. When water falls, hold **BOTH IN** only enough to remove dangerous slack; do not use it to pull the house down.
7. If one rope has more slack, briefly use that rope's individual **in** control. Use individual **out** if one side becomes tighter than the other.

Every drive and winch command stops on release/cancel, STOP, emergency stop, loss of page focus, page hide, Wi-Fi client disconnect, invalid command, or a 600 ms command timeout. Emergency stop latches and must be explicitly cleared; clearing it keeps everything stopped.

> [!CAUTION]
> Test each winch with the house supported and motor power current-limited. Use hard mechanical end stops or limit switches on each spool, plus a physical normally-closed motor-power disconnect. Software cannot detect a rope jam, a reached end of travel, water height, or actual rope tension with only an IMU.

## 4. Two-winch design basis

The supplied **2025 ENGG1100 Learning Guide v1.1**, sections 4.4.1-4.5.2, confirms that:

- The water can rise and fall during the five-minute test and the structure must return as it subsides.
- Ropes/strands are permitted as non-rigid attachments.
- Only two base-plate contact points are allowed while buoyant, and each must fit inside 20 mm x 20 mm.
- Manual, wireless, semi-autonomous, and autonomous control are all permitted.
- External tethering must not interfere with the structure's movement.
- The entire prototype is limited to the supplied 12 V at 2 A, including propulsion and winches.

This firmware therefore controls exactly **two tether winches**. Coordinated controls change both rope lengths together as water height changes; individual controls correct unequal slack. It is intentionally manual because the XC3732 cannot distinguish a safe level house from a level house being pulled down equally by two tight ropes. Automatic tension regulation requires tension/load sensing or a proven mechanical constant-tension system.

## 5. Future tether-winch wiring

The current project already uses all four channels of two dual-channel L9110S boards for propulsion. The two tethers therefore need **two additional reversible H-bridge channels**, such as a third dual-channel driver board, and **two geared winch motors/spools**. Do not connect either motor directly to the ESP32.

| Function | ESP32 GPIO | Extra H-bridge connection |
|---|---:|---|
| Left winch input A | GPIO `27` | Left channel `IN1` / `IA` |
| Left winch input B | GPIO `32` | Left channel `IN2` / `IB` |
| Right winch input A | GPIO `23` | Right channel `IN1` / `IA` |
| Right winch input B | GPIO `33` | Right channel `IN2` / `IB` |
| Left winch motor | — | Left driver output pair only |
| Right winch motor | — | Right driver output pair only |
| Driver ground | ESP32 `GND` | Common ground with ESP32 and motor supply negative |
| Driver motor power | — | Fused 12 V supply, within the total 2 A project limit |

Use drivers rated for each motor's **stall current** and the supply voltage. The propulsion motors, two winches, ESP32, and all electronics together must remain under the guide's 12 V/2 A limit; measure worst-case current before demonstration. Put a 10 kΩ pull-down from GPIO 27, 32, 23, and 33 to ground at the corresponding driver inputs so both winches remain off while the ESP32 resets.

Before attaching rope, test one output at a time: every **out** control must release rope and every **in** control must wind it onto the correct spool. If a direction is reversed, swap that motor's two H-bridge output wires. Use exactly two non-rigid base-plate attachment points, keep each contact fitting within 20 mm x 20 mm, and route the ropes so they can change length without snagging or blocking the permitted movement.

The selected GPIOs deliberately avoid ESP32 flash pins 6–11 and boot-strap pins. Confirm the pin labels on your own ESP32 development board: labels such as `D21` can differ by board, so use the GPIO number shown above.

## Project layout

```text
platformio.ini                 ESP32-WROOM build environments
include/config.h               GPIO, timeout, and controller configuration
include/attitude_sensor.h      MMA8452Q attitude interface
include/motor_controller.h     Propulsion and two-winch commands
include/web_page.h             Offline mobile controller page
src/attitude_sensor.cpp        MMA8452Q driver, filtering, and calibration
src/motor_controller.cpp       H-bridge control and output-safe changes
src/main.cpp                   Wi-Fi, API, and independent safety task
```
