#include <Arduino.h>
#include <LiquidCrystal.h>
#include <Stepper.h>
#include <EEPROM.h>
// Declare the custom type before Arduino IDE 1.x generates function prototypes.
struct Settings;
byte calculateChecksum(const Settings &s);


// ======================================================
// SMART DISPENSER - NO RTC / SUPERVISED PROTOTYPE
// Board: Arduino Mega 2560
// Serial Monitor Baud: 115200
// ======================================================

#define DEBUG_MODE 1

#if DEBUG_MODE
  #define DBG(x) Serial.print(x)
  #define DBGLN(x) Serial.println(x)
#else
  #define DBG(x)
  #define DBGLN(x)
#endif

// ---------------- LCD ----------------
LiquidCrystal lcd(7, 6, 5, 4, 3, 2);

// ---------------- SOFTWARE CLOCK ----------------
// No DS3231, I2C or RTClib is used.
// At EVERY reset, set current time and confirm the empty home position.
// A software clock drifts and cannot retain time while power is off.
// Alarm configuration persists in EEPROM; current time and dose history do not.
// NOT suitable for unattended medication administration.
// Existing tray direction, 260 steps/compartment and 30 positions are retained.
// Validate the gearing and dispensing path with an EMPTY tray before use.
// Alarms remain paused while editing menus; missed minutes are not replayed.
// Normal screen: MENU edits alarm; UP edits current clock time.
// Boot/clock editing: UP/DOWN change value; OK advances/accepts.
// Home confirmation: physically align empty compartment over cup, then OK.
uint32_t clockSeconds = 0;
uint32_t softwareDay = 0;
uint32_t lastClockMillis = 0;
bool clockReady = false;
bool startupSetup = true;
byte editClockHour = 0;
byte editClockMinute = 0;
bool skipArmingMinute = true;
uint32_t armingDay = 0;
uint16_t armingMinute = 0;

byte clockHour() { return clockSeconds / 3600UL; }
byte clockMinute() { return (clockSeconds / 60UL) % 60; }
byte clockSecond() { return clockSeconds % 60; }

void updateSoftwareClock() {
  if (!clockReady) return;
  uint32_t now = (uint32_t)millis();
  uint32_t elapsedSeconds = (uint32_t)(now - lastClockMillis) / 1000UL;
  if (!elapsedSeconds) return;
  // Retain subsecond remainder and handle the 32-bit millis rollover.
  lastClockMillis += elapsedSeconds * 1000UL;
  uint32_t total = clockSeconds + elapsedSeconds;
  softwareDay += total / 86400UL;
  clockSeconds = total % 86400UL;
}

void armAfterCurrentMinute() {
  skipArmingMinute = true;
  armingDay = softwareDay;
  armingMinute = clockSeconds / 60UL;
}

// ---------------- MOTOR ----------------
const int MOTOR_IN1 = 8;
const int MOTOR_IN2 = 9;
const int MOTOR_IN3 = 10;
const int MOTOR_IN4 = 11;

const int STEPS_PER_REVOLUTION = 2048;
Stepper motor(STEPS_PER_REVOLUTION, MOTOR_IN1, MOTOR_IN3, MOTOR_IN2, MOTOR_IN4);

const int DEFAULT_STEPS_PER_COMPARTMENT = 260;
const int TOTAL_POSITIONS = 30;
const int MIN_COMPARTMENT = 1;
const int MAX_COMPARTMENT = 29;
const int MOTOR_SPEED_RPM = 8;

// ---------------- BUTTONS ----------------
// Button 1 = MENU
// Button 2 = OK
// Button 3 = UP
// Button 4 = DOWN
const int BTN_MENU = 22;
const int BTN_OK   = 23;
const int BTN_UP   = 24;
const int BTN_DOWN = 25;

const unsigned long DEBOUNCE_MS = 80;

// ---------------- BUZZER ----------------
const int BUZZER_PIN = 12;
const unsigned long ALARM_DURATION_MS = 5UL * 60UL * 1000UL;

// ---------------- LOGICAL HOME ----------------
// Position 0 is the first EMPTY compartment directly above the cup.
// There is no electrical home sensor in this version of the dispenser.
// Before switching the dispenser on, physically align the empty home
// compartment above the cup. The controller then tracks tray movement
// from that known position by counting motor steps.
const int HOME_POSITION = 0;

// ---------------- EEPROM ----------------
const int EEPROM_ADDR = 0;
const uint32_t EEPROM_MAGIC = 0x53504431; // SPD1

struct Settings {
  uint32_t magic;
  byte version;
  byte alarmHour;
  byte alarmMinute;
  byte alarmCompartment;
  int stepsPerCompartment;
  byte checksum;
};

Settings settings;

// ---------------- SYSTEM STATE ----------------
enum SystemState {
  STATE_CLOCK_HOUR,
  STATE_CLOCK_MINUTE,
  STATE_CONFIRM_HOME,
  STATE_NORMAL,
  STATE_SET_HOUR,
  STATE_SET_MINUTE,
  STATE_SET_COMPARTMENT,
  STATE_SET_STEPS,
  STATE_ALARM_ACTIVE
};

SystemState state = STATE_CLOCK_HOUR;

int currentPosition = 0;
bool systemHomed = false;

unsigned long lastDisplayUpdate = 0;
unsigned long alarmStartMillis = 0;
unsigned long lastBuzzerToggle = 0;
bool buzzerOn = false;

uint32_t lastAlarmDay = UINT32_MAX;
// Sample all buttons every loop so old/held presses cannot leak into another screen.
bool menuPressed = false;
bool okPressed = false;
bool upPressed = false;
bool downPressed = false;

// ======================================================
// BUTTON CLASS
// ======================================================
class DebouncedButton {
  private:
    int pin;
    bool lastReading;
    bool stableState;
    unsigned long lastChangeTime;

  public:
    void begin(int buttonPin) {
      pin = buttonPin;
      pinMode(pin, INPUT_PULLUP);
      lastReading = digitalRead(pin);
      stableState = lastReading;
      lastChangeTime = millis();
    }

    bool pressed() {
      bool reading = digitalRead(pin);

      if (reading != lastReading) {
        lastChangeTime = millis();
        lastReading = reading;
      }

      if ((millis() - lastChangeTime) > DEBOUNCE_MS) {
        if (reading != stableState) {
          stableState = reading;

          if (stableState == LOW) {
            return true;
          }
        }
      }

      return false;
    }
};

DebouncedButton menuButton;
DebouncedButton okButton;
DebouncedButton upButton;
DebouncedButton downButton;

// ======================================================
// EEPROM SETTINGS
// ======================================================
byte calculateChecksum(const Settings &s) {
  byte c = 0;
  c ^= byte(s.magic & 0xFF);
  c ^= byte((s.magic >> 8) & 0xFF);
  c ^= byte((s.magic >> 16) & 0xFF);
  c ^= byte((s.magic >> 24) & 0xFF);
  c ^= s.version;
  c ^= s.alarmHour;
  c ^= s.alarmMinute;
  c ^= s.alarmCompartment;
  c ^= lowByte(s.stepsPerCompartment);
  c ^= highByte(s.stepsPerCompartment);
  return c;
}

void loadDefaultSettings() {
  settings.magic = EEPROM_MAGIC;
  settings.version = 1;
  settings.alarmHour = 8;
  settings.alarmMinute = 0;
  settings.alarmCompartment = 1;
  settings.stepsPerCompartment = DEFAULT_STEPS_PER_COMPARTMENT;
  settings.checksum = calculateChecksum(settings);
}

bool settingsValid() {
  if (settings.magic != EEPROM_MAGIC) return false;
  if (settings.version != 1) return false;
  if (settings.alarmHour > 23) return false;
  if (settings.alarmMinute > 59) return false;
  if (settings.alarmCompartment < MIN_COMPARTMENT || settings.alarmCompartment > MAX_COMPARTMENT) return false;
  if (settings.stepsPerCompartment < 100 || settings.stepsPerCompartment > 600) return false;
  if (settings.checksum != calculateChecksum(settings)) return false;
  return true;
}

void saveSettings() {
  settings.magic = EEPROM_MAGIC;
  settings.version = 1;
  settings.checksum = calculateChecksum(settings);
  EEPROM.put(EEPROM_ADDR, settings);

  DBGLN("[EEPROM] Settings saved");
}

void loadSettings() {
  DBGLN("[EEPROM] Loading settings...");
  EEPROM.get(EEPROM_ADDR, settings);

  if (!settingsValid()) {
    DBGLN("[EEPROM] Invalid settings. Loading defaults...");
    loadDefaultSettings();
    saveSettings();
  } else {
    DBGLN("[EEPROM] Settings valid");
  }

  DBG("[SETTINGS] Alarm: ");
  DBG(settings.alarmHour);
  DBG(":");
  DBG(settings.alarmMinute);
  DBG(" Compartment: ");
  DBG(settings.alarmCompartment);
  DBG(" Steps/Comp: ");
  DBGLN(settings.stepsPerCompartment);
}

// ======================================================
// LCD HELPERS
// ======================================================
void printTwoDigits(int value) {
  if (value < 10) lcd.print("0");
  lcd.print(value);
}

void showSplashScreen() {
  DBGLN("[LCD] Showing splash screen");

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Smart Dispenser");
  lcd.setCursor(0, 1);
  lcd.print("Initializing...");
  delay(1500);
}

void showNormalScreen() {
  lcd.setCursor(0, 0);
  lcd.print("Soft ");
  printTwoDigits(clockHour());
  lcd.print(":");
  printTwoDigits(clockMinute());
  lcd.print(":");
  printTwoDigits(clockSecond());
  lcd.print(" ");

  lcd.setCursor(0, 1);
  lcd.print("Alm ");
  printTwoDigits(settings.alarmHour);
  lcd.print(":");
  printTwoDigits(settings.alarmMinute);
  lcd.print(" C");
  lcd.print(settings.alarmCompartment);
  lcd.print("  ");
}

void showSavedMessage() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Settings Saved");
  lcd.setCursor(0, 1);
  lcd.print("Ready");
  delay(1000);
  lcd.clear();
}

void showMenuScreen() {
  lcd.clear();

  if (state == STATE_CLOCK_HOUR || state == STATE_CLOCK_MINUTE) {
    lcd.setCursor(0, 0);
    lcd.print(state == STATE_CLOCK_HOUR ? "Set Current Hour" : "Set Current Min");
    lcd.setCursor(0, 1);
    printTwoDigits(editClockHour);
    lcd.print(":");
    printTwoDigits(editClockMinute);
    lcd.print("  OK=Next");
  }

  else if (state == STATE_SET_HOUR) {
    lcd.setCursor(0, 0);
    lcd.print("Set Alarm Hour");
    lcd.setCursor(0, 1);
    lcd.print("Hour: ");
    printTwoDigits(settings.alarmHour);
  }

  else if (state == STATE_SET_MINUTE) {
    lcd.setCursor(0, 0);
    lcd.print("Set Alarm Min");
    lcd.setCursor(0, 1);
    lcd.print("Minute: ");
    printTwoDigits(settings.alarmMinute);
  }

  else if (state == STATE_SET_COMPARTMENT) {
    lcd.setCursor(0, 0);
    lcd.print("Set Compartment");
    lcd.setCursor(0, 1);
    lcd.print("Comp: ");
    lcd.print(settings.alarmCompartment);
  }

  else if (state == STATE_SET_STEPS) {
    lcd.setCursor(0, 0);
    lcd.print("Calib Steps/Pos");
    lcd.setCursor(0, 1);
    lcd.print("Steps: ");
    lcd.print(settings.stepsPerCompartment);
  }
}

// ======================================================
// MOTOR CONTROL
// ======================================================
void motorOff() {
  digitalWrite(MOTOR_IN1, LOW);
  digitalWrite(MOTOR_IN2, LOW);
  digitalWrite(MOTOR_IN3, LOW);
  digitalWrite(MOTOR_IN4, LOW);
}

void moveSteps(long steps) {
  DBG("[MOTOR] Moving steps: ");
  DBGLN(steps);

  motor.step(steps);
  motorOff();
  delay(250);

  DBGLN("[MOTOR] Movement complete. Motor coils OFF");
}

void establishLogicalHome() {
  // No motor movement occurs here. The tray must be physically aligned
  // with the empty home compartment above the cup before power-up.
  currentPosition = HOME_POSITION;
  systemHomed = true;
  motorOff();

  DBGLN("[HOME] Physical sensor: NOT INSTALLED");
  DBGLN("[HOME] Empty compartment is logical position 0");
  DBGLN("[HOME] Startup position accepted as HOME");

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Home: Empty Pos");
  lcd.setCursor(0, 1);
  lcd.print("Position: 0");
  delay(1200);
}

void returnTrayToHome() {
  if (!systemHomed) {
    DBGLN("[ERROR] Position reference unavailable");
    return;
  }

  if (currentPosition == HOME_POSITION) {
    DBGLN("[HOME] Tray is already at logical home");
    return;
  }

  int movement = HOME_POSITION - currentPosition;
  long stepsToMove = (long)movement * -(long)settings.stepsPerCompartment;

  DBG("[HOME] Returning from position: ");
  DBGLN(currentPosition);
  DBG("[HOME] Steps to logical home: ");
  DBGLN(stepsToMove);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Returning Home");
  lcd.setCursor(0, 1);
  lcd.print("Empty Position");

  moveSteps(stepsToMove);
  currentPosition = HOME_POSITION;
  DBGLN("[HOME] Empty home compartment restored above cup");
}

void moveToCompartment(int targetCompartment) {
  if (targetCompartment < MIN_COMPARTMENT || targetCompartment > MAX_COMPARTMENT) {
    DBGLN("[ERROR] Invalid target compartment");
    return;
  }

  int movement = targetCompartment - currentPosition;
  long stepsToMove = (long)movement * -(long)settings.stepsPerCompartment;

  DBG("[DISPENSE] Current position: ");
  DBGLN(currentPosition);
  DBG("[DISPENSE] Target compartment: ");
  DBGLN(targetCompartment);
  DBG("[DISPENSE] Steps to move: ");
  DBGLN(stepsToMove);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Dispensing...");
  lcd.setCursor(0, 1);
  lcd.print("Compartment ");
  lcd.print(targetCompartment);

  moveSteps(stepsToMove);

  currentPosition = targetCompartment;

  DBG("[DISPENSE] New position: ");
  DBGLN(currentPosition);
}

// ======================================================
// BUZZER
// ======================================================
void buzzerStop() {
  noTone(BUZZER_PIN);
  buzzerOn = false;
}

void updateAlarmBuzzer() {
  unsigned long now = millis();

  if (now - lastBuzzerToggle >= 500) {
    lastBuzzerToggle = now;
    buzzerOn = !buzzerOn;

    if (buzzerOn) {
      tone(BUZZER_PIN, 1200);
    } else {
      noTone(BUZZER_PIN);
    }
  }
}

void shortBeep() {
  tone(BUZZER_PIN, 1200);
  delay(120);
  noTone(BUZZER_PIN);
}

// ======================================================
// ALARM LOGIC
// ======================================================
bool alarmAlreadyTriggeredToday() {
  return lastAlarmDay == softwareDay;
}

void markAlarmTriggered() {
  lastAlarmDay = softwareDay;
}

void startAlarmSequence() {
  DBGLN("[ALARM] Alarm sequence started");

  if (!systemHomed || !clockReady) {
    DBGLN("[ERROR] Time or logical position reference unavailable");
    return;
  }

  moveToCompartment(settings.alarmCompartment);

  alarmStartMillis = millis();
  lastBuzzerToggle = 0;
  buzzerOn = false;

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Item Ready");
  lcd.setCursor(0, 1);
  lcd.print("Press OK");

  state = STATE_ALARM_ACTIVE;

  DBGLN("[ALARM] Buzzer active. Waiting for OK or timeout.");
}

void finishAlarmSequence(bool acknowledged) {
  buzzerStop();

  if (acknowledged) {
    DBGLN("[ALARM] Stopped by OK button");
  } else {
    DBGLN("[ALARM] Stopped after 5-minute timeout");
  }

  lcd.clear();

  if (acknowledged) {
    lcd.setCursor(0, 0);
    lcd.print("Confirmed");
    lcd.setCursor(0, 1);
    lcd.print("Alarm Stopped");
  } else {
    lcd.setCursor(0, 0);
    lcd.print("Alarm Timeout");
    lcd.setCursor(0, 1);
    lcd.print("No OK Pressed");
  }

  delay(1500);

  DBGLN("[ALARM] Returning to home position");
  returnTrayToHome();

  lcd.clear();
  state = STATE_NORMAL;
}

void checkAlarmTime() {
  if (state != STATE_NORMAL || !clockReady || !systemHomed) return;

  // Never dispense immediately on startup or after saving a time/alarm.
  if (skipArmingMinute) {
    if (softwareDay == armingDay && clockSeconds / 60UL == armingMinute) return;
    skipArmingMinute = false;
  }

  if (clockHour() == settings.alarmHour &&
      clockMinute() == settings.alarmMinute &&
      !alarmAlreadyTriggeredToday()) {

    DBGLN("[CLOCK] Alarm time matched");
    markAlarmTriggered();
    startAlarmSequence();
  }
}

void handleActiveAlarm() {
  updateAlarmBuzzer();

  if (okPressed) {
    finishAlarmSequence(true);
    return;
  }

  if (millis() - alarmStartMillis >= ALARM_DURATION_MS) {
    finishAlarmSequence(false);
    return;
  }
}

// ======================================================
// MENU
// ======================================================
void enterMenu() {
  DBGLN("[MENU] Enter menu");
  state = STATE_SET_HOUR;
  showMenuScreen();
}

void nextMenuItem() {
  if (state == STATE_SET_HOUR) {
    state = STATE_SET_MINUTE;
  } else if (state == STATE_SET_MINUTE) {
    state = STATE_SET_COMPARTMENT;
  } else if (state == STATE_SET_COMPARTMENT) {
    state = STATE_SET_STEPS;
  } else if (state == STATE_SET_STEPS) {
    saveSettings();
    armAfterCurrentMinute();
    showSavedMessage();
    state = STATE_NORMAL;
    return;
  }

  showMenuScreen();
}

void saveAndExitMenu() {
  saveSettings();
  armAfterCurrentMinute();
  shortBeep();
  showSavedMessage();
  state = STATE_NORMAL;
}

void handleMenuButtons() {
  if (state == STATE_NORMAL) {
    if (menuPressed) {
      enterMenu();
    } else if (upPressed) {
      editClockHour = clockHour();
      editClockMinute = clockMinute();
      state = STATE_CLOCK_HOUR;
      showMenuScreen();
    }
    return;
  }

  if (state == STATE_SET_HOUR ||
      state == STATE_SET_MINUTE ||
      state == STATE_SET_COMPARTMENT ||
      state == STATE_SET_STEPS) {

    if (menuPressed) {
      nextMenuItem();
      return;
    }

    if (okPressed) {
      saveAndExitMenu();
      return;
    }

    if (upPressed) {
      if (state == STATE_SET_HOUR) {
        settings.alarmHour++;
        if (settings.alarmHour > 23) settings.alarmHour = 0;
      }

      else if (state == STATE_SET_MINUTE) {
        settings.alarmMinute++;
        if (settings.alarmMinute > 59) settings.alarmMinute = 0;
      }

      else if (state == STATE_SET_COMPARTMENT) {
        settings.alarmCompartment++;
        if (settings.alarmCompartment > MAX_COMPARTMENT) {
          settings.alarmCompartment = MIN_COMPARTMENT;
        }
      }

      else if (state == STATE_SET_STEPS) {
        settings.stepsPerCompartment++;
        if (settings.stepsPerCompartment > 600) {
          settings.stepsPerCompartment = 600;
        }
      }

      showMenuScreen();
    }

    if (downPressed) {
      if (state == STATE_SET_HOUR) {
        if (settings.alarmHour == 0) settings.alarmHour = 23;
        else settings.alarmHour--;
      }

      else if (state == STATE_SET_MINUTE) {
        if (settings.alarmMinute == 0) settings.alarmMinute = 59;
        else settings.alarmMinute--;
      }

      else if (state == STATE_SET_COMPARTMENT) {
        if (settings.alarmCompartment <= MIN_COMPARTMENT) {
          settings.alarmCompartment = MAX_COMPARTMENT;
        } else {
          settings.alarmCompartment--;
        }
      }

      else if (state == STATE_SET_STEPS) {
        settings.stepsPerCompartment--;
        if (settings.stepsPerCompartment < 100) {
          settings.stepsPerCompartment = 100;
        }
      }

      showMenuScreen();
    }
  }
}

// ======================================================
// SOFTWARE CLOCK / STARTUP CONTROLS
// ======================================================
void handleClockSetup() {
  if (upPressed || downPressed) {
    if (state == STATE_CLOCK_HOUR) {
      editClockHour = (editClockHour + (upPressed ? 1 : 23)) % 24;
    } else {
      editClockMinute = (editClockMinute + (upPressed ? 1 : 59)) % 60;
    }
    showMenuScreen();
  }

  if (!okPressed) return;
  if (state == STATE_CLOCK_HOUR) {
    state = STATE_CLOCK_MINUTE;
    showMenuScreen();
    return;
  }

  clockSeconds = (uint32_t)editClockHour * 3600UL +
                 (uint32_t)editClockMinute * 60UL;
  lastClockMillis = (uint32_t)millis();
  clockReady = true;
  // Retain today's trigger guard if the time is corrected while powered.
  armAfterCurrentMinute();
  DBG("[CLOCK] Current time set: ");
  DBG(editClockHour);
  DBG(":");
  DBGLN(editClockMinute);

  if (startupSetup) {
    state = STATE_CONFIRM_HOME;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Empty above cup?");
    lcd.setCursor(0, 1);
    lcd.print("Align, press OK");
    DBGLN("[HOME] Confirm EMPTY compartment above cup. No motor search.");
  } else {
    state = STATE_NORMAL;
    lcd.clear();
  }
}

void handleHomeConfirmation() {
  if (!okPressed) return;
  establishLogicalHome();
  startupSetup = false;
  updateSoftwareClock();
  armAfterCurrentMinute();
  state = STATE_NORMAL;
  lcd.clear();
  DBGLN("[READY] Software clock running. MENU=alarm, UP=current time.");
}

// ======================================================
// SETUP
// ======================================================
void setup() {
  Serial.begin(115200);
  delay(500);

  DBGLN("");
  DBGLN("====================================");
  DBGLN("SMART DISPENSER BOOT STARTED");
  DBGLN("Board: Arduino Mega 2560");
  DBGLN("Serial Debug: Enabled");
  DBGLN("====================================");

  pinMode(MOTOR_IN1, OUTPUT);
  pinMode(MOTOR_IN2, OUTPUT);
  pinMode(MOTOR_IN3, OUTPUT);
  pinMode(MOTOR_IN4, OUTPUT);
  motorOff();
  DBGLN("[INIT] Motor pins configured");

  pinMode(BUZZER_PIN, OUTPUT);
  buzzerStop();
  DBGLN("[INIT] Buzzer configured");

  DBGLN("[INIT] Home sensor not used");
  DBGLN("[INIT] Align empty compartment above cup before power-up");

  menuButton.begin(BTN_MENU);
  okButton.begin(BTN_OK);
  upButton.begin(BTN_UP);
  downButton.begin(BTN_DOWN);
  DBGLN("[INIT] Buttons configured");

  lcd.begin(16, 2);
  DBGLN("[INIT] LCD started");
  showSplashScreen();

  DBGLN("[INIT] Software clock selected. RTC not required.");

  loadSettings();

  motor.setSpeed(MOTOR_SPEED_RPM);
  DBGLN("[INIT] Motor speed configured");

  clockReady = false;
  systemHomed = false;
  startupSetup = true;
  state = STATE_CLOCK_HOUR;
  DBGLN("[BOOT] Set current hour/minute using UP/DOWN and OK.");
  DBGLN("[BOOT] Dispensing locked until time and empty home are confirmed.");
  showMenuScreen();
}

// ======================================================
// LOOP
// ======================================================
void loop() {
  updateSoftwareClock();
  menuPressed = menuButton.pressed();
  okPressed = okButton.pressed();
  upPressed = upButton.pressed();
  downPressed = downButton.pressed();

  if (state == STATE_CLOCK_HOUR || state == STATE_CLOCK_MINUTE) {
    handleClockSetup();
    return;
  }

  if (state == STATE_CONFIRM_HOME) {
    handleHomeConfirmation();
    return;
  }

  if (state == STATE_ALARM_ACTIVE) {
    handleActiveAlarm();
    return;
  }

  handleMenuButtons();

  if (state == STATE_NORMAL) {
    if (millis() - lastDisplayUpdate >= 500) {
      lastDisplayUpdate = millis();
      showNormalScreen();
    }

    checkAlarmTime();
  }
}


