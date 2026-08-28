#include <Wire.h>
#include <RTClib.h>

// Arduino Mega 2560: SDA = 20, SCL = 21, common GND.
// Power your DS3231 breakout according to its voltage specification.
// Install "RTClib by Adafruit" and its required dependencies.
// Serial Monitor: 115200 baud. No LCD, buttons or motor are used.

// Leave false for diagnostics and power-retention testing.
// To set the RTC, change to true, upload ONCE, then change back to
// false and upload again. Compile time uses the computer's clock and
// will lag by the compile/upload duration. RTClib does not apply time zones.
const bool SET_RTC_TO_COMPILE_TIME = false;

RTC_DS3231 rtc;
const byte RTC_ADDRESS = 0x68;

bool rtcResponds() {
  Wire.beginTransmission(RTC_ADDRESS);
  return Wire.endTransmission() == 0;
}

void printTwoDigits(byte value) {
  if (value < 10) Serial.print('0');
  Serial.print(value);
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Wire.begin();
  Wire.setClock(100000);
#if defined(WIRE_HAS_TIMEOUT)
  Wire.setWireTimeout(25000, true);
#endif

  Serial.println(F("\nDS3231 RTC TEST - Arduino Mega 2560"));
  Serial.println(F("SDA: pin 20 | SCL: pin 21 | Serial: 115200"));

  if (!rtcResponds() || !rtc.begin()) {
    Serial.println(F("ERROR: RTC not detected at 0x68."));
    Serial.println(F("Check power, GND, SDA and SCL. Then reset the Mega."));
    while (true) delay(1000);
  }

  Serial.println(F("RTC detected at 0x68."));
  if (rtc.lostPower()) {
    Serial.println(F("WARNING: Oscillator-stop flag is set; time may be invalid."));
    Serial.println(F("Possible first use, interrupted power or backup-battery issue."));
  } else {
    Serial.println(F("Oscillator-stop flag is clear. Check displayed time yourself."));
  }

  if (SET_RTC_TO_COMPILE_TIME) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    Serial.println(F("RTC set to compile time. Disable SET_RTC_TO_COMPILE_TIME now."));
  } else {
    Serial.println(F("Diagnostic mode: stored clock time has NOT been changed."));
  }

  Serial.println(F("The oscillator flag does NOT measure battery voltage."));
}

void loop() {
  static unsigned long lastRead = 0;
  if (millis() - lastRead < 1000) return;
  lastRead = millis();

  if (!rtcResponds()) {
    Serial.println(F("ERROR: RTC stopped responding. Check wiring/power."));
    return;
  }

  DateTime now = rtc.now();
  if (!now.isValid()) {
    Serial.println(F("ERROR: Invalid date/time. Check wiring and set the RTC."));
    return;
  }

  Serial.print(now.year());
  Serial.print('-');
  printTwoDigits(now.month());
  Serial.print('-');
  printTwoDigits(now.day());
  Serial.print(F("  "));
  printTwoDigits(now.hour());
  Serial.print(':');
  printTwoDigits(now.minute());
  Serial.print(':');
  printTwoDigits(now.second());
  Serial.print(F(" | Oscillator-stop flag: "));
  Serial.println(rtc.lostPower() ? F("SET - time untrusted") : F("clear"));
}

