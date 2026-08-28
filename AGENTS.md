# Arduino pill dispenser project contract

- This is the Arduino Mega 2560 system from the current project conversation, not the Raspberry Pi Smart_Medication_System repository.
- The immediate starting point is diagnostics/Dispenser_Hardware_Test: a manual hardware test with no RTC, software clock or EEPROM writes.
- Do not invent a home sensor. Home is the empty compartment over the cup, established by manual alignment. Never claim position is measured from a commanded step count.
- Preserve LCD pins 7,6,5,4,3,2; ULN2003 inputs 8,9,10,11; buttons 22,23,24,25; buzzer 12 unless the user approves rewiring.
- Treat 260 steps per compartment, 30 positions and motor gearing as unverified calibration, not physical evidence.
- RTC communication/time and physical motor movement are unresolved. Read docs/STATUS.md before making completion claims.
- Do not describe desktop mock checks as real Arduino compilation or hardware validation. Run both scripts for source changes when their dependencies are available; report missing toolchains plainly.
- Keep custom-type forward declarations before Arduino-generated function prototypes.
- Keep tests empty-tray and supervised. Do not flash a device without user authorization or add clinical claims.
- Do not overwrite unrelated user changes. Keep firmware variants in separate matching Arduino sketch folders.
- Never add credentials, patient data, binary builds or full private chat transcripts to this repository.
