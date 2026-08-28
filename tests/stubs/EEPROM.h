#pragma once
#include "Arduino.h"
struct EEPROMMock {
  byte data[128] = {};
  template<class T> void get(int addr, T &value) { memcpy(&value,data+addr,sizeof(value)); }
  template<class T> void put(int addr, const T &value) { memcpy(data+addr,&value,sizeof(value)); }
};
static EEPROMMock EEPROM;

