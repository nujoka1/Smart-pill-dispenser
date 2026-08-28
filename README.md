# Smart Pill Dispenser

Arduino Mega 2560 pill-dispenser prototype developed by Joseph Nuhu Kalba as part of the Smart Health Ecosystem. This repository contains the Arduino system discussed in the project conversation, **not** the separate Raspberry Pi `Smart_Medication_System` project.

**Development prototype: use an empty tray for testing. Do not use for unattended medication administration.** The latest hardware report confirms LCD/buzzer activity but no motor rotation. RTC detection was intermittent and its stored date/time was invalid. A software success message is not proof of physical dispensing.

## Start here

Use [Dispenser_Hardware_Test](diagnostics/Dispenser_Hardware_Test/Dispenser_Hardware_Test.ino) to test the system without the RTC. It does not require clock setup or modify EEPROM.

1. With power off, ensure the empty home compartment is directly above the cup. Remove all medication from the tray.
2. Open the sketch in its own Arduino IDE window; select **Arduino Mega or Mega 2560 / ATmega2560**.
3. Compile and upload. Open Serial Monitor at **115200 baud**.
4. Press **OK** once to confirm home, then release and press **OK again** to start a test.
5. The motor is commanded to the selected compartment; the LCD then displays the test-ready message and the buzzer sounds.
6. Press **OK** to silence the alarm and command the return to home. Otherwise the alarm times out after five minutes and commands a return.
7. Visually check actual movement and alignment. The firmware has no position feedback.

When idle, **MENU** opens compartment selection and then steps-per-compartment calibration; **UP/DOWN** change values; **OK** accepts and exits. During movement, **MENU** stops motion and releases the coils. Power off, realign the empty home compartment and restart after a stop. This button is a software stop, not a safety-rated emergency stop.

## Sketches

| Folder | Purpose | Important distinction |
| --- | --- | --- |
| `diagnostics/Dispenser_Hardware_Test` | Manual integrated LCD, button, motor and buzzer test | Recommended starting point; no clock or EEPROM writes |
| `diagnostics/SmartDispenser_Button_Test` | Serial diagnostic for four buttons | Press/release and held-button reporting |
| `diagnostics/DS3231_RTC_Test` | Standalone clock diagnostic | Does not set time unless the explicit configuration flag is enabled |
| `firmware/SmartDispenser_RTC` | RTC-based daily alarm prototype | Sensorless logical home; inherited startup/retention limitations apply |
| `experiments/SmartDispenser_NoRTC` | Optional software-clock experiment | Not required for manual testing; clock must be set after every restart |

Each `.ino` belongs in its same-named folder. **Do not combine these sketches as tabs in one Arduino project.** They each define their own `setup()` and `loop()`.

## Hardware and design scope

Arduino Mega 2560, parallel 16x2 LCD, 28BYJ-48 stepper with ULN2003 driver, four momentary buttons, buzzer and optional DS3231 RTC. Logical position **0** is the empty compartment above the cup; there is **no home sensor**. The code retains 29 selectable medicine positions plus home, 260 configured full steps per position, and an 8 RPM motor setting. These counts are inherited configuration, not a verified mechanical measurement.

See [wiring](docs/WIRING.md), [implementation status](docs/STATUS.md) and [verification guide](docs/TESTING.md). This Arduino version contains no web dashboard, Wi-Fi integration or backend; features from other Health Ecosystem projects must not be attributed to it.

## Build and tests

Arduino IDE libraries: **LiquidCrystal**, **Stepper**, and **RTClib by Adafruit** for the two RTC sketches (accept its dependencies). EEPROM is provided by the AVR board package. The manual hardware test only needs LiquidCrystal and Stepper.

The `Settings` declaration-order error reported with Arduino IDE 1.8.19 is addressed in both full firmware variants by an explicit forward declaration and checksum prototype before any function definition. Desktop C++ tests alone cannot verify Arduino sketch preprocessing.

```bash
bash scripts/run_host_tests.sh
bash scripts/compile_all.sh
```

The first command uses desktop mocks and checks 19 behavioral scenarios. The second requires Arduino CLI and the board/libraries installed as described in the verification guide. GitHub Actions is configured to run both host checks and actual Mega-target compilation; consult its latest run for results. Neither replaces physical testing.

## Known limits

- No sensor confirms alignment, motor movement, cup presence, pill passage or dose collection.
- Manual alignment is necessary after reset, loss of power, a stall or manual tray movement.
- Open-loop return paths may pass other compartments over the outlet; the mechanical design must prevent unintended release.
- The RTC firmware currently sets compile time automatically if its oscillator-stop flag is set; this is not reliable recovery for medication scheduling.
- Daily trigger history is RAM-only and can be lost on reset. There is no persistent dose ledger or reliable duplicate prevention across power loss.
- Alarm checking is paused in configuration menus; missed minutes are not replayed. The scheduled variants also use blocking motor movements.
- No clinical validation or production-readiness claim is made.

## Licence

MIT, as supplied by the repository owner. See [LICENSE](LICENSE). The licence does not establish medical safety or regulatory approval.
