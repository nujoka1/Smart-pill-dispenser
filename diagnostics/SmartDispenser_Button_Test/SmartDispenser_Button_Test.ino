// =============================================================
// SMART PILL DISPENSER - FOUR BUTTON DIAGNOSTIC
// Board: Arduino Mega 2560
// Wiring: Each button connects its assigned pin to GND when pressed.
// Serial Monitor: 115200 baud
// =============================================================

const byte BTN_MENU = 22;
const byte BTN_OK   = 23;
const byte BTN_UP   = 24;
const byte BTN_DOWN = 25;

const unsigned long DEBOUNCE_MS = 50;
const unsigned long STUCK_WARNING_MS = 5000;

struct TestButton {
  const char *name;
  byte pin;
  bool rawState;
  bool stableState;
  unsigned long rawChangedAt;
  unsigned long pressedAt;
  bool stuckWarningShown;
};

TestButton buttons[] = {
  {"MENU", BTN_MENU, HIGH, HIGH, 0, 0, false},
  {"OK",   BTN_OK,   HIGH, HIGH, 0, 0, false},
  {"UP",   BTN_UP,   HIGH, HIGH, 0, 0, false},
  {"DOWN", BTN_DOWN, HIGH, HIGH, 0, 0, false}
};

const byte BUTTON_COUNT = sizeof(buttons) / sizeof(buttons[0]);
unsigned long lastStatusAt = 0;

void printButtonStates() {
  Serial.print(F("[STATUS] "));

  for (byte i = 0; i < BUTTON_COUNT; i++) {
    Serial.print(buttons[i].name);
    Serial.print('=');
    Serial.print(buttons[i].stableState == LOW ? F("PRESSED") : F("released"));

    if (i < BUTTON_COUNT - 1) {
      Serial.print(F(" | "));
    }
  }

  Serial.println();
}

byte countPressedButtons() {
  byte count = 0;

  for (byte i = 0; i < BUTTON_COUNT; i++) {
    if (buttons[i].stableState == LOW) {
      count++;
    }
  }

  return count;
}

void updateButton(TestButton &button) {
  bool reading = digitalRead(button.pin);
  unsigned long now = millis();

  if (reading != button.rawState) {
    button.rawState = reading;
    button.rawChangedAt = now;
  }

  if ((now - button.rawChangedAt >= DEBOUNCE_MS) &&
      (button.stableState != button.rawState)) {
    button.stableState = button.rawState;

    if (button.stableState == LOW) {
      button.pressedAt = now;
      button.stuckWarningShown = false;

      Serial.print(F("[PASS] "));
      Serial.print(button.name);
      Serial.print(F(" button PRESSED on pin "));
      Serial.println(button.pin);
    } else {
      unsigned long heldFor = now - button.pressedAt;

      Serial.print(F("[PASS] "));
      Serial.print(button.name);
      Serial.print(F(" button RELEASED after "));
      Serial.print(heldFor);
      Serial.println(F(" ms"));
    }
  }

  if (button.stableState == LOW &&
      !button.stuckWarningShown &&
      (now - button.pressedAt >= STUCK_WARNING_MS)) {
    button.stuckWarningShown = true;

    Serial.print(F("[WARNING] "));
    Serial.print(button.name);
    Serial.println(F(" has remained pressed for 5 seconds. Check for a stuck button or short to GND."));
  }
}

void setup() {
  Serial.begin(115200);

  for (byte i = 0; i < BUTTON_COUNT; i++) {
    pinMode(buttons[i].pin, INPUT_PULLUP);
    buttons[i].rawState = digitalRead(buttons[i].pin);
    buttons[i].stableState = buttons[i].rawState;
    buttons[i].rawChangedAt = millis();

    if (buttons[i].stableState == LOW) {
      buttons[i].pressedAt = millis();
    }
  }

  delay(300);
  Serial.println();
  Serial.println(F("============================================"));
  Serial.println(F("SMART DISPENSER BUTTON TEST"));
  Serial.println(F("Board: Arduino Mega 2560"));
  Serial.println(F("Serial Monitor: 115200 baud"));
  Serial.println(F("============================================"));
  Serial.println(F("Wiring:"));
  Serial.println(F("MENU button: pin 22 to GND"));
  Serial.println(F("OK button:   pin 23 to GND"));
  Serial.println(F("UP button:   pin 24 to GND"));
  Serial.println(F("DOWN button: pin 25 to GND"));
  Serial.println(F("No external pull-up resistor is required."));
  Serial.println(F("Press and release every button individually."));
  Serial.println(F("============================================"));

  printButtonStates();

  for (byte i = 0; i < BUTTON_COUNT; i++) {
    if (buttons[i].stableState == LOW) {
      Serial.print(F("[WARNING] "));
      Serial.print(buttons[i].name);
      Serial.println(F(" is active during startup. Check its wiring."));
    }
  }
}

void loop() {
  for (byte i = 0; i < BUTTON_COUNT; i++) {
    updateButton(buttons[i]);
  }

  byte pressedCount = countPressedButtons();
  static byte previousPressedCount = 0;

  if (pressedCount > 1 && previousPressedCount <= 1) {
    Serial.println(F("[INFO] Multiple buttons are being pressed simultaneously."));
  }

  previousPressedCount = pressedCount;

  if (millis() - lastStatusAt >= 3000) {
    lastStatusAt = millis();
    printButtonStates();
  }
}

