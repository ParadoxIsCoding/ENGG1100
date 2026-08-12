# ENGG1100 Station-Keeping Controller

This is the offline phone controller for Team Lavender's floating station-keeping prototype. It now targets the **ESP32-WROOM-32U** and directly supports the Jaycar **XC3732** / Keyestudio **KS0270** tri-axis tilt sensor. That module uses the **MMA8452Q** accelerometer—not the MPU6050 described by older revisions of this project.

The ESP32 creates the `ENGG1100-Lavender` Wi-Fi network and serves the controller page without an internet connection.

> [!IMPORTANT]
> The XC3732 measures the platform's tilt; it cannot move rope. Paying out or retrieving a tether physically requires a reversible geared winch/spool motor, an H-bridge driver, and a suitable separate motor supply. The controller includes safe manual winch controls ready for that hardware, but they remain simulated until those parts are added.

## What changed

- Uses the real MMA8452Q over I2C, checks its device ID, and automatically tries both valid addresses (`0x1D`, then `0x1C`).
- Reads live acceleration at 50 Hz, low-pass filters it, and calculates pitch and roll for the artificial horizon.
- Shows an explicit `IMU NOT FOUND` warning in hardware mode instead of silently displaying demo values.
- Adds hold-to-run **PAY OUT** and **TAKE IN** tether-winch controls. They use the same STOP, emergency stop, Wi-Fi-disconnect stop, and 600 ms dead-man timeout as all other motor controls.
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
5. When water rises, hold **PAY OUT** just enough to prevent the tether becoming tight.
6. When water falls, hold **TAKE IN** only enough to remove dangerous slack; do not use it to pull the house under or against an immovable anchor.

Every drive and winch command stops on release/cancel, STOP, emergency stop, loss of page focus, page hide, Wi-Fi client disconnect, invalid command, or a 600 ms command timeout. Emergency stop latches and must be explicitly cleared; clearing it keeps everything stopped.

> [!CAUTION]
> Test the winch with the house supported and motor power current-limited. Use hard mechanical end stops or limit switches on a real spool, plus a physical normally-closed motor-power disconnect. Software cannot detect a rope jam, a reached end of travel, or actual rope tension with only an IMU.

## 4. Future tether-winch wiring

The current project already uses all four channels of two dual-channel L9110S boards for propulsion. The tether therefore needs **one spare reversible H-bridge channel** (a third compatible driver board or a suitable dedicated driver) and a **geared winch motor/spool**. Do not connect a motor directly to the ESP32.

| Function | ESP32 GPIO | Extra H-bridge connection |
|---|---:|---|
| Winch input A | GPIO `27` | `IN1` / `IA` |
| Winch input B | GPIO `32` | `IN2` / `IB` |
| Winch motor | — | Driver output pair only |
| Driver ground | ESP32 `GND` | Common ground with ESP32 and motor supply negative |
| Driver motor power | — | Correct fused motor supply for the selected winch and driver |

Use a motor driver rated for the winch motor's **stall current** and supply voltage. Put a 10 kΩ pull-down from GPIO 27 and GPIO 32 to ground at the driver inputs so the winch remains off while the ESP32 resets. Before using the roof/anchor assembly, test direction with the rope removed: **PAY OUT** must release rope; **TAKE IN** must wind rope in. If directions are reversed, swap the two wires between the winch motor and its H-bridge output—do not change the safety logic.

The selected GPIOs deliberately avoid ESP32 flash pins 6–11 and boot-strap pins. Confirm the pin labels on your own ESP32 development board: labels such as `D21` can differ by board, so use the GPIO number shown above.

## Project layout

```text
platformio.ini                 ESP32-WROOM build environments
include/config.h               GPIO, timeout, and controller configuration
include/attitude_sensor.h      MMA8452Q attitude interface
include/motor_controller.h     Propulsion and tether-winch commands
include/web_page.h             Offline mobile controller page
src/attitude_sensor.cpp        MMA8452Q driver, filtering, and calibration
src/motor_controller.cpp       H-bridge control and output-safe changes
src/main.cpp                   Wi-Fi, API, and independent safety task
```
