# ENGG1100 Station Keeper

Station Keeper is an offline iPhone controller for Team Lavender's floating evacuation-centre prototype. A standard ESP32 (ESP32-D0WD-V3 DevKit, `esp32dev`) creates its own Wi-Fi network, drives four corner tether winches, and reads a GY-521 (MPU6050) tilt sensor. The phone interface provides proportional drive, manual per-corner tether payout/retrieval, a motor speed control, four motor-status cards, pitch and roll, a display-only vertical-motion estimate, a motor calibration tab, and latched emergency stop. The project will later move to an ESP32-S3 N16R8; the `esp32-s3-*` environments exist for that but are not physically wired yet.

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

Use the GPIO numbers printed below, not a board vendor's `D` numbers. Each corner winch is one 60 RPM N20 gear motor with a 3D-printed pulley spooling fishing line to a fixed anchor point in that corner's direction. Each winch has its own single-channel DRV8871 H-bridge module (four installed, one spare from the 5-pack); there is no separate propulsion motor set. Pin numbers below are for the standard ESP32 (`esp32-hardware`); see `include/config.h` for the placeholder ESP32-S3 map.

The four rows below are labelled "Motor 1"–"Motor 4" rather than by corner on purpose: which corner each one actually drives, and whether it needs to spin inverted, is **not** set in firmware. Wire them in whatever order is physically convenient, then use the phone UI's **CALIBRATE** tab to spin each motor and tell the controller which corner it is — see "Motor calibration" below.

| Device | Terminal | Standard ESP32 |
|---|---|---:|
| DRV8871 #1 (Motor 1) | `IN1`, `IN2` | GPIO `13`, GPIO `14` |
| DRV8871 #2 (Motor 2) | `IN1`, `IN2` | GPIO `16`, GPIO `17` |
| DRV8871 #3 (Motor 3) | `IN1`, `IN2` | GPIO `18`, GPIO `19` |
| DRV8871 #4 (Motor 4) | `IN1`, `IN2` | GPIO `25`, GPIO `26` |
| DRV8871 #1–#4 | logic `GND` | ESP32 `GND` (common ground) |
| DRV8871 #1–#4 | `OUT1`, `OUT2` | That module's N20 motor only |
| GY-521 (MPU6050) | `SDA`, `SCL` | GPIO `21`, GPIO `22` |
| GY-521 (MPU6050) | `VCC`, `GND` | `3V3`, `GND` |
| GY-521 (MPU6050) | `XDA`, `XCL`, `AD0`, `INT` | Not connected |

```text
STANDARD ESP32                    SIGNAL CONNECTIONS

GPIO 13, 14  ------------------> DRV8871 #1 IN1/IN2 --> OUT1/OUT2 --> Motor 1
GPIO 16, 17  ------------------> DRV8871 #2 IN1/IN2 --> OUT1/OUT2 --> Motor 2
GPIO 18, 19  ------------------> DRV8871 #3 IN1/IN2 --> OUT1/OUT2 --> Motor 3
GPIO 25, 26  ------------------> DRV8871 #4 IN1/IN2 --> OUT1/OUT2 --> Motor 4
GND          ------------------> DRV8871 #1-#4 GND
GPIO 21      ------------------> GY-521 SDA
GPIO 22      ------------------> GY-521 SCL
3V3          ------------------> GY-521 VCC
GND          ------------------> GY-521 GND
```

Do not use GPIO 6-11 on the standard ESP32: they are tied to the module's internal flash interface.

Connect each motor only to its own module's `OUT1`/`OUT2` terminals. Never parallel two DRV8871 outputs or share one motor between modules. The DRV8871 has no separate logic-supply pin: it runs from its motor supply (`VM`) and accepts the ESP32's 3.3 V logic directly, referenced to the shared `GND`. Terminal names vary slightly between module vendors (e.g. `VM`/`+`, `GND`/`-`), so check the silkscreen on your module.

### DRV8871 control behaviour

The firmware drives each module according to the DRV8871 datasheet truth table:

| `IN1` | `IN2` | Motor outputs | Firmware use |
|:---:|:---:|---|---|
| 0 | 0 | Both Hi-Z: coast; the chip sleeps after about 1 ms | **STOP**, E-stop, dead-man timeout, startup |
| PWM | 0 | Forward (`OUT1`→`OUT2`), coasting during PWM off-time | Positive power (IN / retrieve before inversion) |
| 0 | PWM | Reverse (`OUT2`→`OUT1`), coasting during PWM off-time | Negative power (OUT / payout before inversion) |
| 1 | 1 | Both low: brake | **Not used** |

PWM is 20 kHz, 8-bit, on one input at a time. A calibrated **INVERT** simply swaps which input carries the PWM. STOP deliberately coasts rather than brakes, so a software stop matches what the modules do when the ESP32 is resetting or unpowered (both inputs read LOW).

**Pull-downs:** the DRV8871 has internal pull-down resistors on `IN1` and `IN2` (about 100 kΩ per the datasheet), so an unpowered, resetting, or not-yet-configured ESP32 leaves each module in the coast state without extra parts. The 10 kΩ external pull-downs the old driver boards needed are therefore **not required**. They remain optional and harmless (about 0.33 mA at 3.3 V) if you want extra noise immunity on long signal leads near the motor wiring; some modules already fit them, so check before adding more. Note that no pull-down can stop an ESP32 pin that the chip itself briefly drives during boot. GPIO 14 (Motor 1 `IN2`) is known to output a short signal at boot on some ESP32 modules, so Motor 1 may twitch briefly on reset. This is why motor power should be off or E-stopped while flashing or resetting.

**Current rating:** treat the DRV8871 as a roughly 3.6 A peak-class driver (TI datasheet), not the "10 A" figure some sellers quote. The actual usable current depends on the module's current-limit resistor (`ILIM`; the datasheet gives I<sub>TRIP</sub> ≈ 64 / R<sub>ILIM</sub> in kΩ), its PCB copper, and temperature. It also has built-in over-current, over-temperature, and under-voltage protection. The driver's headroom is **not** permission for the motors to draw more: the whole prototype must still stay within the 12 V / 2 A limit below.

### Motor calibration

Because Motor 1–4 above are just wiring positions, first power-up (or any time a winch gets re-plugged into a different DRV8871) needs a short calibration pass from the phone, not a firmware change:

1. Open the **CALIBRATE** tab.
2. For each of the four motor cards, hold **OUT** or **IN** — it spins only that one physical motor, at a fixed gentle speed independent of the drive speed slider — and watch which corner of the prototype actually moves.
3. Tap that corner's button (FL/FR/RL/RR) to label the motor. If it turned the wrong way (paid out on IN, or vice versa), tap **INVERT**.
4. Repeat for all four motors, then press **SAVE CALIBRATION**. The mapping applies immediately and is written to the ESP32's flash (NVS), so it survives reboots and re-flashing until it's recalibrated. **RESET TO DEFAULT** restores Motor 1→front-left, Motor 2→rear-left, Motor 3→front-right, Motor 4→rear-right, none inverted.

Calibration spins use the same hold-to-run/dead-man safety behaviour as every other motor control: releasing the button, losing the Wi-Fi connection, or hitting the emergency stop all stop it immediately.

Mount the GY-521 rigidly above the water line, component side upward. Power it from **3.3 V only**. Do not connect it to 5 V or 12 V.

## Electrical power wiring

The ESP32 and every motor driver must share ground, but the ESP32 must receive regulated 5 V (or, on the current bench test, USB power only) — never the raw motor supply.

**Current temporary bench test wiring** (no fuse yet): an 8xAA battery holder feeds a female XT30 pigtail, which mates to a male XT30 pigtail on the harness. Male XT30 red/black go to a positive/negative Wago each; the positive Wago feeds `VM` (motor supply +) on all four DRV8871 modules, and the negative Wago feeds `GND` on all four. ESP32 `GND` shares that same negative rail. The ESP32 itself is powered only through USB, separately from the AA pack. Because there is no fuse in this temporary setup, keep first tests brief, supervised, and one motor at a time — see the first-power procedure below.

**Final prototype wiring** (not yet built): a fixed 12 V / 2 A supply through XT30, fused, feeding the `VM` terminals of all four DRV8871 modules through the master switch and normally-closed motor E-stop, with a separate 5 V buck converter powering the ESP32 from the same 12 V rail and common ground throughout. The DRV8871 operates from 6.5 V to 45 V, so a nominal 12 V rail (including a supply sitting slightly above 12 V) is well within its range.

```text
SUPPLIED 12 V / 2 A  (final prototype; bench test uses 8xAA instead, no fuse yet)
        +
        +---- 2 A fuse ---- master switch ---- normally-closed motor E-STOP ----+
        |                                                                    |
        |                                                                    +--> DRV8871 #1 VM
        |                                                                    +--> DRV8871 #2 VM
        |                                                                    +--> DRV8871 #3 VM
        |                                                                    +--> DRV8871 #4 VM
        |
        +---- fused branch ---- 12 V to regulated 5 V buck converter --------+--> ESP32 5V/VIN

SUPPLY NEGATIVE -------------------------------------------------------------+--> ESP32 GND
                                                                             +--> DRV8871 #1 GND
                                                                             +--> DRV8871 #2 GND
                                                                             +--> DRV8871 #3 GND
                                                                             +--> DRV8871 #4 GND
                                                                             +--> buck converter GND

ESP32 3V3 ----------------------------------------------------------------------> GY-521 VCC
ESP32 GND ----------------------------------------------------------------------> GY-521 GND
```

Never connect 12 V (or the AA pack) to an ESP32 GPIO, `3V3`, `5V`, or USB pin, and never to a DRV8871 `IN1`/`IN2`. Size the wiring for each motor's stall current. The four corner winches, ESP32, sensor, and converter together must remain within the project-wide 12 V / 2 A limit. Check the worst-case current before putting the prototype in water.

## Build and upload

Four PlatformIO environments are defined in `platformio.ini`:

| Environment | Board | `TEST_MODE` | Use |
|---|---|---|---|
| `esp32-test` (default) | Standard ESP32 | 1 | Hardware-free; motors and MPU6050 simulated |
| `esp32-hardware` | Standard ESP32 | 0 | The board currently on the bench, four physical DRV8871 modules + optional GY-521 |
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

At startup the serial monitor reports the firmware mode, board type, all 8 motor pin assignments (as DRV8871 #1–#4 `IN1`/`IN2`), the saved slot→corner calibration and inversion, the MPU6050 detection result, the Wi-Fi SSID/password, the IP address, emergency-stop state, and a confirmation that all motors are STOPPED.

## First power-up and hardware test

Use this order after first wiring the DRV8871 modules, and after any rewiring:

1. **Unpowered checks.** With the XT30 motor supply disconnected and USB unplugged, confirm continuity from supply negative to every DRV8871 `GND`, the buck converter `GND`, and ESP32 `GND`. Confirm no continuity between the motor supply positive and ESP32 `3V3`, `5V`/`VIN`, or any GPIO. Confirm each motor is on its own module's `OUT1`/`OUT2`, and that `VM`/`GND` polarity on every module is correct: reversed supply polarity can destroy the DRV8871.
2. **Logic only.** Power the ESP32 alone (USB), with motor power still disconnected. Upload `esp32-hardware`, open the serial monitor, and check it reports `IN1`/`IN2` GPIO 13/14, 16/17, 18/19, 25/26 for Motor 1–4 and that all motors are STOPPED.
3. **Check the stopped state.** With motor power still disconnected, measure each `IN1`/`IN2` to `GND`. All eight should read about 0 V.
4. **Motor power, supervised.** Support the prototype so no winch is loaded, fit the lines loose or off, keep a hand on the master switch/E-stop, then connect motor power. No motor should move. If any does, switch off immediately and recheck wiring.
5. **One motor at a time.** On the **CALIBRATE** tab, briefly hold **OUT** then **IN** on each motor card. Only that one motor should turn, in both directions. Label its corner and set **INVERT** as needed, then **SAVE CALIBRATION**.
6. **Check stopping.** While holding a control, test that each of these stops the motor: releasing the button, **STOP**, **EMERGENCY STOP** (motors stay stopped after **CLEAR E-STOP**), turning off the phone's Wi-Fi (connection-loss and dead-man stop), and opening the physical motor E-stop.
7. **Speed and current.** On the **WINCHES** tab, check each corner at low and then higher speed. Measure supply current with all four winches running and while one is briefly stalled, and confirm the whole system stays within 12 V / 2 A. Check that no DRV8871 module gets hot.

## Operating instructions

1. Power on with the prototype supported and motor power (XT30) disconnected.
2. Join `ENGG1100-Lavender` on the iPhone using password `station1100`. Accept the no-internet warning and stay connected.
3. Open `http://192.168.4.1/`. Confirm **ONLINE** and **HARDWARE**; **TEST MODE** means motor outputs are simulated.
4. Float the house level and stationary, then press **SET LEVEL**. Nose-up pitch now reads positive.
5. Use the **SPEED** slider to set the commanded motor speed (20–100%, default 80%). The last setting is saved to flash and restored after a reboot. For first bench tests, lower it before driving.
6. Select **DRIVE**. Hold and drag the joystick to pull the house across the water: each corner winch retrieves or pays out its tether to produce the requested direction. Release the joystick to stop. Hold either rotate button to turn in place via diagonal corner pairs. The four motor-status cards show each corner's live commanded direction (IN / OUT / STOPPED) and power.
7. Select **WINCHES**. Hold **ALL OUT** as water rises and the tethers become tight. Hold **ALL IN** as water falls to remove slack without pulling the house down. Use the FL/FR/RL/RR controls to correct a single corner with unequal rope length, and to test each motor independently on first power-up.
8. Treat ↑ **RISING**, ↓ **FALLING**, and ● **STEADY** as short-term motion guidance only. Rocking, driving, or abrupt tilting can affect the estimate.
9. Press **STOP** for a normal stop. Press **EMERGENCY STOP** to latch all outputs off; remove the hazard before pressing **CLEAR E-STOP**. Clearing does not restart a motor.

All movement is hold-to-run. Release, pointer cancellation, tab switching, page hiding, Wi-Fi disconnection, invalid commands, STOP, E-stop, or a missing command heartbeat (~500 ms) stops the motors. The independent firmware dead-man timeout is 600 ms.

Before attaching lines, test each corner winch with the platform supported. Verify that **OUT** releases fishing line and **IN** winds it in. Fit spool end stops or limit switches and retain a physical motor-power E-stop because this firmware cannot detect line tension, jams, or end of travel.
