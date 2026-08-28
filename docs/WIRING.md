# Wiring and mechanical contract

Power off before changing connections. Use the motor label and breakout specifications to select supply voltages. The motor drives through a ULN2003 board, never directly from GPIO.

## Arduino Mega 2560 mapping

| Device signal | Mega connection | Notes |
| --- | --- | --- |
| LCD RS | D7 | Parallel LiquidCrystal interface |
| LCD E | D6 | Not an I2C LCD backpack |
| LCD D4 / D5 / D6 / D7 | D5 / D4 / D3 / D2 | 4-bit data mode |
| LCD RW | GND | Write-only operation |
| LCD VSS / VDD | GND / rated supply | Typical 5 V character LCD; check module |
| LCD VO | Contrast potentiometer wiper | Adjust contrast; backlight circuit depends on module |
| ULN2003 IN1 | D8 | Physical input order |
| ULN2003 IN2 | D9 | Physical input order |
| ULN2003 IN3 | D10 | Physical input order |
| ULN2003 IN4 | D11 | Physical input order |
| Buzzer control | D12 | Existing tone-driven interface; electrical type/current not confirmed |
| MENU button | D22 to GND | INPUT_PULLUP; pressed LOW |
| OK button | D23 to GND | INPUT_PULLUP; pressed LOW |
| UP button | D24 to GND | INPUT_PULLUP; pressed LOW |
| DOWN button | D25 to GND | INPUT_PULLUP; pressed LOW |
| DS3231 SDA | D20 | Optional for manual hardware testing |
| DS3231 SCL | D21 | Optional for manual hardware testing |
| DS3231 VCC / GND | Rated supply / common GND | Follow breakout voltage specification |

There is no sensor attached to D26. Do not bridge an unused input to fake home detection.

## Motor power

Plug the motor's five-wire connector into its ULN2003 socket. Supply the driver motor-power terminals from a regulated source matching the motor label: many 28BYJ-48 units are 5 V, but 12 V versions exist. For a confirmed 5 V unit, a regulated 5 V supply rated at least 1 A is suitable for initial unloaded testing. A supply's current rating is its capacity, not current forced into the motor.

Connect motor-supply negative, ULN2003 GND and Mega GND together. The Mega may remain USB powered. Do not join an external supply positive to the Mega USB/5 V rail without a reviewed power arrangement. Do not drive a high-current or inductive buzzer directly from a GPIO; use an appropriate driver and protection as required.

The Stepper constructor uses `(2048, 8, 10, 9, 11)`: the logical coil order is IN1, IN3, IN2, IN4. This does **not** mean changing physical IN2 from D9 to D10. Keep the wiring table above.

## Position and calibration

The supplied code uses position 0 as the first empty compartment above the cup, and selectable positions 1 through 29. There is no home switch, encoder or position sensor. Software position is only an estimate based on commanded steps.

The default 260 full steps per compartment implies 7,800 motor steps for 30 positions, whereas the Stepper constructor uses a nominal 2,048 steps per motor-output revolution for speed calculation. This requires checking actual external gearing, tray geometry, motor step mode and calibration; the numbers do not describe a direct-drive 30-position tray without additional explanation. Do not silently substitute another value or claim it is calibrated.

Check return-path behavior using an empty tray: intermediate compartments may pass the cup outlet. A gate or other verified mechanical arrangement is needed if that path would otherwise release additional pills. Avoid forcing the geared motor shaft by hand; use the mechanism's safe alignment method with power off.

## RTC battery caution

An oscillator-stop flag does not measure battery voltage. Check the actual battery marking and whether the module has a charging circuit. A non-rechargeable CR2032 must not be charged. Do not replace it with a rechargeable cell unless the module and cell specifications support that arrangement.
