# Evidence and implementation status

Evidence cutoff: 28 August 2026. Source: the current Arduino pill-dispenser conversation and its supplied/generated sketches. This is an engineering handoff, not the completed academic write-up.

## Scope boundary

This repository represents the **Arduino Mega pill dispenser** in the Smart Health Ecosystem. It is not the Raspberry Pi medication dashboard repository. No dashboard, cloud connection, remote-control service or clinical validation is evidenced for this Arduino implementation. The broader ecosystem also contains an AI stethoscope and a separate heart-rate monitor; their components and results are not transplanted into this project's description.

## Status ledger

| Requirement | Status | Evidence and qualification |
| --- | --- | --- |
| LCD display and buzzer in manual cycle | PASS | User confirmed the moving/ready LCD sequence and buzzer; this does not prove motor motion |
| Physical motor rotation | FAIL | Latest user report: 28BYJ-48 + ULN2003 did not rotate / appeared unpowered; root cause not established |
| Driver supply, common ground and input LEDs | NOT VERIFIED | Wiring checks were suggested, but voltage/LED observations were not supplied |
| Full four-button hardware acceptance | PARTIAL | UI interaction occurred; exhaustive physical test of every button has not been recorded |
| DS3231 communication | PARTIAL | At least one log detected address 0x68; repeated startup and incomplete logs also occurred |
| Valid RTC time and backup retention | FAIL | Oscillator-stop flag and invalid date/time reported; no successful retention test recorded |
| Reset/power stability | NOT VERIFIED | Repeated headings could reflect reset, serial reconnect or unstable supply; no measured cause established |
| No physical home-sensor dependency | PASS | Source has no GPIO homing scan; empty compartment is logical home |
| Accurate tray positioning / return | NOT VERIFIED | Sensorless commands and calibration have not been confirmed physically |
| Five-minute timeout and OK acknowledgment | PARTIAL | Covered in host simulation; timed hardware acceptance still needed |
| Desktop logic scenarios | PASS | 12 optional software-clock scenarios and 7 manual-test scenarios pass with mocks |
| Reported Arduino Settings-type compilation defect | PARTIAL | Forward declaration + explicit prototype added; check Mega-target build results separately |
| Mega-target compilation of all five sketches | NOT VERIFIED | GitHub workflow is supplied; its latest run is authoritative, not this initial ledger |
| Recovery without duplicate dose after reboot | FAIL | Scheduled variants keep trigger history only in RAM |
| Clinical / unattended operation | NOT VERIFIED | Not validated; empty-tray supervised tests only |

## File lineage

- `firmware/SmartDispenser_RTC`: derived from user-provided `pill19aug2026.ino`, with the nonexistent sensor workflow replaced by logical home. The original uploaded file had its HOME_SENSOR_PIN definition commented while reads remained. The repository copy also addresses custom-type prototype ordering. Motor, LCD and button pins and EEPROM settings format were preserved.
- `experiments/SmartDispenser_NoRTC`: optional manually set software-clock variant. The user clarified that this was more than needed for the immediate integrated test. It remains an experiment, not the default test.
- `diagnostics/Dispenser_Hardware_Test`: later manual integrated diagnostic, requested to test the whole device excluding the RTC. A second OK press starts movement, with no clock setup or EEPROM writes.
- `diagnostics/SmartDispenser_Button_Test`: standalone Serial button diagnostic.
- `diagnostics/DS3231_RTC_Test`: standalone RTC diagnostic; time adjustment is opt-in.

These are source-code revisions and user observations, not a transcript export or a claim that all features were tested on hardware. The original licence is retained without alteration.

## Next engineering checks

1. Verify the ULN2003 supply voltage under load, common ground, physical input mapping and motor connector. Watch driver LEDs during movement, not after coils have been switched off.
2. Demonstrate unloaded rotation with the correct power supply before attaching the tray load.
3. Measure actual steps per compartment and verify the complete outbound/return path cannot release unintended doses.
4. Establish valid RTC time, test battery retention, and diagnose unexpected resets separately.
5. Replace automatic compile-time recovery and add a persistent, power-failure-safe dose record before considering unattended scheduling.
6. Record photographs, measured test results and final wiring for the later professional write-up. Do not manufacture missing evidence.
