#include <cassert>
#include <iostream>
#include "../diagnostics/Dispenser_Hardware_Test/Dispenser_Hardware_Test.ino"

void press(byte index) {
  testPins[BUTTON_PINS[index]] = LOW; loop();
  testMillis += DEBOUNCE_MS + 1; loop();
  testPins[BUTTON_PINS[index]] = HIGH; loop();
  testMillis += DEBOUNCE_MS + 1; loop();
}
void finishMotion() {
  unsigned guard = 20000;
  while ((state == MOVING_OUT || state == RETURNING_HOME) && guard--) loop();
  assert(guard > 0);
}
int main() {
  setup();
  assert(state == CONFIRM_HOME && motor.calls == 0 && !toneActive);
  press(OK_BUTTON); assert(state == READY && motor.calls == 0);
  std::cout << "PASS boot confirmation never moves motor\n";
  press(MENU_BUTTON); assert(state == EDIT_COMPARTMENT);
  press(UP_BUTTON); assert(compartment == 2);
  press(DOWN_BUTTON); assert(compartment == 1);
  press(MENU_BUTTON); assert(state == EDIT_STEPS);
  press(UP_BUTTON); assert(stepsPerCompartment == 261);
  press(OK_BUTTON); assert(state == READY);
  std::cout << "PASS all four buttons and both configuration screens\n";
  press(OK_BUTTON); finishMotion();
  assert(state == BUZZING && commandedPosition == -261 && toneActive);
  std::cout << "PASS outbound motor and buzzer\n";
  press(OK_BUTTON); assert(!toneActive); finishMotion();
  assert(state == READY && commandedPosition == 0 && motor.netSteps == 0);
  std::cout << "PASS acknowledgment stops sound and reverses exact steps\n";
  // Hold OK through capture completion: it must not acknowledge the alarm.
  testPins[BUTTON_PINS[OK_BUTTON]] = LOW; loop();
  testMillis += DEBOUNCE_MS + 1; loop(); finishMotion();
  assert(state == BUZZING);
  loop(); assert(state == BUZZING);
  testPins[BUTTON_PINS[OK_BUTTON]] = HIGH; loop(); testMillis += DEBOUNCE_MS+1; loop();
  std::cout << "PASS held start button cannot acknowledge automatically\n";
  alarmStarted = UINT32_MAX - 200000;
  testMillis = (uint32_t)(alarmStarted + ALARM_DURATION_MS - 1); loop();
  assert(state == BUZZING);
  testMillis++; loop(); assert(state == RETURNING_HOME && !toneActive);
  finishMotion(); assert(state == READY && motor.netSteps == 0);
  std::cout << "PASS five-minute timeout across 32-bit timer rollover\n";
  press(OK_BUTTON); press(MENU_BUTTON);
  assert(state == STOPPED && !toneActive);
  unsigned steps = motor.calls;
  press(OK_BUTTON); loop(); assert(motor.calls == steps && state == STOPPED);
  for (int pin=8;pin<=11;pin++) assert(testPins[pin] == LOW);
  std::cout << "PASS motor stop releases coils and locks restart\n";
}

