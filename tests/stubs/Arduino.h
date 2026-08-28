#pragma once
#include <cstdint>
#include <cstdlib>
#include <cstring>
using byte = uint8_t;
#define HIGH 1
#define LOW 0
#define INPUT_PULLUP 2
#define OUTPUT 1
#define F(x) x
#define lowByte(x) ((byte)((x) & 0xff))
#define highByte(x) ((byte)(((x) >> 8) & 0xff))
static uint32_t testMillis = 0;
static int testPins[64];
static bool toneActive = false;
inline unsigned long millis() { return testMillis; }
inline void delay(unsigned long ms) { testMillis += ms; }
inline void pinMode(int pin, int mode) { if (mode == INPUT_PULLUP) testPins[pin] = HIGH; }
inline int digitalRead(int pin) { return testPins[pin]; }
inline void digitalWrite(int pin, int value) { testPins[pin] = value; }
inline void tone(int, int) { toneActive = true; }
inline void noTone(int) { toneActive = false; }
struct SerialMock {
  void begin(int) {}
  template<class T> void print(T) {}
  template<class T> void println(T) {}
  void println() {}
};
static SerialMock Serial;

