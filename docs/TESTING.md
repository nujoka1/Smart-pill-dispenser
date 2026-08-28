# Verification guide

## Desktop tests

Run `bash scripts/run_host_tests.sh` with g++ available. Tests use small Arduino/LCD/Stepper/EEPROM mocks; they do not load the real AVR core. The 12 software-clock scenarios cover startup lockout, settings round-trip, explicit home confirmation, no immediate setup-minute alarm, scheduled movement, debounced OK acknowledgment, same-day duplicate suppression, midnight, timeout, timer rollover, elapsed-time catch-up and menu pausing. The 7 manual-test scenarios cover no startup motion, four-button/menu behavior, outbound commands and buzzer, acknowledgment and return, held-button suppression, timeout rollover and stop lockout.

Mock EEPROM does not validate AVR memory layout. Mock Stepper calls cannot detect missing power, wrong coil order, a stall or inadequate torque. Desktop C++ compilation does not run Arduino IDE's prototype generator.

## Actual Arduino compilation

With Arduino CLI installed:

```bash
arduino-cli core update-index
arduino-cli core install arduino:avr@1.8.6
arduino-cli lib install "LiquidCrystal@1.0.7" "Stepper@1.1.3" "RTClib@2.1.4"
bash scripts/compile_all.sh
```

This reproducible target uses AVR core 1.8.6; the user-reported environment was Arduino IDE 1.8.19 with AVR core 1.8.8 and a system compiler override. A CI pass does not establish equivalence with that mixed local installation. Recompile locally as well.

The workflow `.github/workflows/verify.yml` performs the above Mega build plus host tests without deploying or flashing hardware. EEPROM comes from the selected AVR core. RTClib installs its library dependencies through Arduino CLI. Manual testing needs only LiquidCrystal and Stepper.

## Settings prototype regression

The previous no-RTC sketch defined software-clock functions before `struct Settings`. Arduino IDE 1.x generated a prototype for `calculateChecksum(const Settings &s)` before seeing the type. The repository places `struct Settings;` and the explicit checksum declaration before any function definition in both scheduled sketches. Keep them there or move user-defined types into a header included before generated prototypes. The Mega build of the `.ino` sketches, rather than a desktop include alone, is the regression check.

## Physical tests (record results, do not assume PASS)

Use an empty tray, stable correctly rated power, and a way to remove power immediately.

- Confirm all four buttons individually, including release and held-button behavior.
- Confirm LCD characters and contrast; verify pin mapping matches the parallel display.
- Run the manual cycle at compartment 1. Record measured motor voltage during motion, driver LED pattern, rotation direction and physical displacement.
- Confirm the return aligns with the empty home compartment. Test higher positions only after proving the direction and gearing.
- Press OK while buzzing and check immediate silence. Separately measure the full five-minute timeout.
- Test MENU stop during outbound and return motion; after stopping, realign safely and reset before another cycle.
- Verify RTC time advances, then remove all main power for one minute with its battery installed; after reconnecting, verify elapsed time. Keep the diagnostic's adjustment flag false for this test.
- Check reboot behavior without medication. Neither firmware variant has durable duplicate-dose history.

Upload only one sketch at a time. Tests overwrite the board's currently flashed program, but the standalone diagnostics intentionally do not change stored EEPROM settings. The full scheduled sketches may write their configuration.
