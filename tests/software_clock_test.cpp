#include <cassert>
#include <iostream>
#include "../experiments/SmartDispenser_NoRTC/SmartDispenser_NoRTC.ino"

void setClock(byte hour, byte minute) {
  editClockHour = hour; editClockMinute = minute;
  state = STATE_CLOCK_HOUR; upPressed = downPressed = false; okPressed = true;
  handleClockSetup();
  assert(state == STATE_CLOCK_MINUTE);
  handleClockSetup();
  okPressed = false;
}

int main() {
  setup();
  assert(!clockReady && !systemHomed && motor.calls == 0);
  assert(state == STATE_CLOCK_HOUR);
  std::cout << "PASS startup locked, no homing motion\n";

  loadDefaultSettings(); saveSettings(); settings.alarmHour = 99; loadSettings();
  assert(settingsValid() && settings.alarmHour == 8);
  std::cout << "PASS settings round-trip and checksum\n";

  setClock(7,59);
  assert(clockReady && !systemHomed && state == STATE_CONFIRM_HOME);
  checkAlarmTime(); assert(motor.calls == 0);
  okPressed = true; handleHomeConfirmation(); okPressed = false;
  assert(systemHomed && state == STATE_NORMAL && motor.calls == 0);
  std::cout << "PASS time then explicit home confirmation\n";

  setClock(8,0); checkAlarmTime();
  assert(state == STATE_NORMAL && motor.calls == 0);
  std::cout << "PASS no immediate alarm in setup minute\n";

  setClock(7,59);
  testMillis += 60000; updateSoftwareClock(); checkAlarmTime();
  assert(state == STATE_ALARM_ACTIVE && currentPosition == 1 && motor.netSteps == -260);
  assert(lastAlarmDay == softwareDay);
  std::cout << "PASS next scheduled minute dispenses selected position\n";

  testMillis += 600; updateAlarmBuzzer(); assert(toneActive);
  // Exercise actual debounced OK press through the main loop.
  testPins[BTN_OK] = LOW; loop();
  testMillis += DEBOUNCE_MS + 1; loop();
  assert(state == STATE_NORMAL && currentPosition == 0 && !toneActive && motor.netSteps == 0);
  testPins[BTN_OK] = HIGH; loop(); testMillis += DEBOUNCE_MS + 1; loop();
  std::cout << "PASS OK press stops buzzer and returns to home\n";

  unsigned calls = motor.calls;
  skipArmingMinute = false; clockSeconds = 8UL*3600; checkAlarmTime();
  assert(motor.calls == calls);
  setClock(7,59); testMillis += 60000; updateSoftwareClock(); checkAlarmTime();
  assert(motor.calls == calls);
  std::cout << "PASS duplicate guard including backward clock correction\n";

  clockSeconds = 86399; lastClockMillis = testMillis;
  testMillis += 1500; updateSoftwareClock();
  assert(clockSeconds == 0 && softwareDay == 1);
  testMillis += 500; updateSoftwareClock(); assert(clockSeconds == 1);
  std::cout << "PASS midnight and fractional-second carry\n";

  clockSeconds = 8UL*3600; skipArmingMinute = false;
  checkAlarmTime(); assert(state == STATE_ALARM_ACTIVE);
  testMillis = (uint32_t)alarmStartMillis + ALARM_DURATION_MS - 1;
  okPressed = false; handleActiveAlarm(); assert(state == STATE_ALARM_ACTIVE);
  testMillis++; handleActiveAlarm();
  assert(state == STATE_NORMAL && currentPosition == 0 && !toneActive && motor.netSteps == 0);
  std::cout << "PASS next-day alarm and exact five-minute timeout\n";

  clockSeconds = 123; lastClockMillis = UINT32_MAX - 500; testMillis = 499;
  updateSoftwareClock(); assert(clockSeconds == 124);
  std::cout << "PASS 32-bit millis rollover\n";

  clockSeconds = 0; lastClockMillis = testMillis;
  testMillis += 125123; updateSoftwareClock(); assert(clockSeconds == 125);
  testMillis += 877; updateSoftwareClock(); assert(clockSeconds == 126);
  std::cout << "PASS elapsed-time catch-up after blocking movement\n";

  state = STATE_SET_HOUR; lastAlarmDay = UINT32_MAX; skipArmingMinute = false;
  clockSeconds = 8UL*3600; calls = motor.calls;
  checkAlarmTime(); assert(motor.calls == calls);
  std::cout << "PASS menu pauses dispensing\n";
}

