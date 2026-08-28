#include <Arduino.h>
#include <LiquidCrystal.h>
#include <Stepper.h>

// MEGA 2560 INTEGRATED HARDWARE TEST -- EMPTY TRAY ONLY.
// No RTC, clock setup, scheduled dispensing, home sensor or EEPROM writes.
// LCD: RS=7, E=6, D4=5, D5=4, D6=3, D7=2 (parallel 16x2).
// Motor driver: IN1=8, IN2=9, IN3=10, IN4=11.
// Buttons: MENU=22, OK=23, UP=24, DOWN=25; each connects to GND.
// Buzzer: pin 12. Serial Monitor: 115200.
// Keep the existing motor driver/power wiring. Do not power a motor from GPIO.
// Align the empty home compartment BEFORE power-up; OK confirms alignment.
// READY: OK starts one test; MENU edits compartment then steps/compartment.
// EDIT: UP/DOWN change value, MENU advances, OK accepts and exits.
// BUZZER: OK stops it and returns the tray, or wait five minutes.
// MOVING: MENU stops motion, releases coils, and requires realignment/reset.
// Motor step counts are open-loop commands, not measured position feedback.
// The inherited 260 steps/position is NOT independently calibrated here.

LiquidCrystal lcd(7, 6, 5, 4, 3, 2);
Stepper motor(2048, 8, 10, 9, 11);
const byte BUTTON_PINS[4] = {22, 23, 24, 25};
const byte MENU_BUTTON = 0, OK_BUTTON = 1, UP_BUTTON = 2, DOWN_BUTTON = 3;
const byte BUZZER_PIN = 12;
const uint32_t DEBOUNCE_MS = 80;
const uint32_t ALARM_DURATION_MS = 5UL * 60UL * 1000UL;

enum TestState { CONFIRM_HOME, READY, EDIT_COMPARTMENT, EDIT_STEPS,
                 MOVING_OUT, BUZZING, RETURNING_HOME, STOPPED };
TestState state = CONFIRM_HOME;
int compartment = 1;
int stepsPerCompartment = 260;
long commandedPosition = 0;
long targetPosition = 0;
bool rawButton[4], stableButton[4], pressedEvent[4];
uint32_t changedAt[4];
uint32_t alarmStarted = 0, buzzerChanged = 0;
bool buzzerOn = false;

// Explicit prototypes avoid depending on Arduino's generated declarations.
void motorOff();
void buzzerStop();
void showReady();
void showEditor();
void pollButtons();
void startTest();
void startReturn();
void updateMotion();
void stopMotion();
void updateBuzzer();
void setup();
void loop();

void motorOff() {
  for (byte pin = 8; pin <= 11; pin++) digitalWrite(pin, LOW);
}

void buzzerStop() {
  noTone(BUZZER_PIN);
  buzzerOn = false;
}

void showReady() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F("Test Comp: "));
  lcd.print(compartment);
  lcd.setCursor(0, 1);
  lcd.print(F("OK=Run MENU=Set"));
  Serial.println(F("[READY] OK starts one cycle. MENU edits settings."));
}

void showEditor() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(state == EDIT_COMPARTMENT ? F("Compartment 1-29") : F("Steps/Compartment"));
  lcd.setCursor(0, 1);
  lcd.print(state == EDIT_COMPARTMENT ? compartment : stepsPerCompartment);
  lcd.print(F(" U/D OK=Done"));
}

void pollButtons() {
  const uint32_t now = (uint32_t)millis();
  for (byte i = 0; i < 4; i++) {
    pressedEvent[i] = false;
    const bool reading = digitalRead(BUTTON_PINS[i]);
    if (reading != rawButton[i]) {
      rawButton[i] = reading;
      changedAt[i] = now;
    }
    if ((uint32_t)(now - changedAt[i]) >= DEBOUNCE_MS &&
        stableButton[i] != rawButton[i]) {
      stableButton[i] = rawButton[i];
      pressedEvent[i] = (stableButton[i] == LOW);
      if (pressedEvent[i]) {
        Serial.print(F("[BUTTON] Pressed GPIO "));
        Serial.println(BUTTON_PINS[i]);
      }
    }
  }
}

void startTest() {
  if (state != READY || commandedPosition != 0) return;
  targetPosition = -(long)compartment * (long)stepsPerCompartment;
  state = MOVING_OUT;
  lcd.clear();
  lcd.print(F("Moving to C"));
  lcd.print(compartment);
  lcd.setCursor(0, 1);
  lcd.print(F("MENU=Stop motor"));
  Serial.print(F("[MOTOR] Outbound target steps: "));
  Serial.println(targetPosition);
}

void startReturn() {
  buzzerStop();
  targetPosition = 0;
  state = RETURNING_HOME;
  lcd.clear();
  lcd.print(F("Returning home"));
  lcd.setCursor(0, 1);
  lcd.print(F("MENU=Stop motor"));
  Serial.println(F("[MOTOR] Reversing commanded steps to logical home."));
}

void stopMotion() {
  motorOff();
  buzzerStop();
  state = STOPPED;
  lcd.clear();
  lcd.print(F("Motor stopped"));
  lcd.setCursor(0, 1);
  lcd.print(F("Realign & reset"));
  Serial.println(F("[STOP] Power off, align EMPTY home, then restart."));
}

void updateMotion() {
  if (commandedPosition != targetPosition) {
    int direction = targetPosition > commandedPosition ? 1 : -1;
    motor.step(direction);  // One step per loop; buttons remain serviced.
    commandedPosition += direction;
  }
  if (commandedPosition != targetPosition) return;
  motorOff();
  if (state == MOVING_OUT) {
    state = BUZZING;
    alarmStarted = buzzerChanged = (uint32_t)millis();
    buzzerOn = true;
    tone(BUZZER_PIN, 1200);
    lcd.clear();
    lcd.print(F("Test item ready"));
    lcd.setCursor(0, 1);
    lcd.print(F("OK=Stop & return"));
    Serial.println(F("[ALARM] Buzzer started. OK or 5-minute timeout returns home."));
  } else {
    state = READY;
    Serial.println(F("[MOTOR] Return command finished. Visually verify home alignment."));
    showReady();
  }
}

void updateBuzzer() {
  uint32_t now = (uint32_t)millis();
  if (pressedEvent[OK_BUTTON] || (uint32_t)(now - alarmStarted) >= ALARM_DURATION_MS) {
    Serial.println(pressedEvent[OK_BUTTON] ? F("[ALARM] OK acknowledged.") : F("[ALARM] Five minutes elapsed."));
    startReturn();
    return;
  }
  if ((uint32_t)(now - buzzerChanged) >= 500) {
    buzzerChanged = now;
    buzzerOn = !buzzerOn;
    if (buzzerOn) tone(BUZZER_PIN, 1200);
    else noTone(BUZZER_PIN);
  }
}

void setup() {
  Serial.begin(115200);
  for (byte pin = 8; pin <= 11; pin++) pinMode(pin, OUTPUT);
  motorOff();
  motor.setSpeed(8);
  pinMode(BUZZER_PIN, OUTPUT);
  buzzerStop();
  for (byte i = 0; i < 4; i++) {
    pinMode(BUTTON_PINS[i], INPUT_PULLUP);
    rawButton[i] = stableButton[i] = digitalRead(BUTTON_PINS[i]);
    changedAt[i] = (uint32_t)millis();
  }
  lcd.begin(16, 2);
  lcd.clear();
  lcd.print(F("Empty above cup?"));
  lcd.setCursor(0, 1);
  lcd.print(F("OK=Confirm home"));
  Serial.println(F("\nDISPENSER HARDWARE TEST - NO RTC"));
  Serial.println(F("No clock or scheduling. No EEPROM data is changed."));
  Serial.println(F("Confirm empty home alignment with OK; press OK AGAIN to test."));
}

void loop() {
  pollButtons();
  if (state == STOPPED) return;
  if (state == MOVING_OUT || state == RETURNING_HOME) {
    if (pressedEvent[MENU_BUTTON]) stopMotion();
    else updateMotion();
    return;
  }
  if (state == BUZZING) {
    updateBuzzer();
    return;
  }
  if (state == CONFIRM_HOME) {
    if (pressedEvent[OK_BUTTON]) {
      commandedPosition = 0;
      state = READY;
      showReady();
    }
    return;
  }
  if (state == READY) {
    if (pressedEvent[MENU_BUTTON]) {
      state = EDIT_COMPARTMENT;
      showEditor();
    } else if (pressedEvent[OK_BUTTON]) startTest();
    return;
  }
  if (pressedEvent[OK_BUTTON]) {
    state = READY;
    showReady();
    return;
  }
  if (pressedEvent[MENU_BUTTON]) {
    if (state == EDIT_COMPARTMENT) {
      state = EDIT_STEPS;
      showEditor();
    } else {
      state = READY;
      showReady();
    }
    return;
  }
  if (pressedEvent[UP_BUTTON] || pressedEvent[DOWN_BUTTON]) {
    int delta = pressedEvent[UP_BUTTON] ? 1 : -1;
    if (state == EDIT_COMPARTMENT) {
      compartment += delta;
      if (compartment > 29) compartment = 1;
      if (compartment < 1) compartment = 29;
    } else {
      stepsPerCompartment += delta;
      if (stepsPerCompartment > 600) stepsPerCompartment = 600;
      if (stepsPerCompartment < 100) stepsPerCompartment = 100;
    }
    showEditor();
  }
}

