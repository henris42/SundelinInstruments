// debug.h - debug output that never holds up the music.
//
// The firmware prints a great deal to the USB serial port. When nothing on
// the computer is reading that port, a write to it can wait for up to 120 ms
// before giving up, and the whole main loop stands still for that long: notes
// and everything else are late. (Seen as "longest loop 128 ms" in the debug
// status.) So all printing goes through this port, which simply drops the
// text unless a monitor has the port open and there is room for it.
//
// Include this after <Arduino.h>; from then on "Serial" means the quiet port.
// Serial1, the MIDI port, is not affected.
#pragma once
#include <Arduino.h>

class DebugPort : public Print {
public:
  void begin(long) {}
  operator bool() {
    return (bool)Serial;
  }
  size_t write(uint8_t b) override {
    return write(&b, 1);
  }
  size_t write(const uint8_t* data, size_t n) override {
    if (Serial.dtr() && Serial.availableForWrite() >= (int)n) Serial.write(data, n);
    return n;     // dropped text counts as written
  }
};

extern DebugPort Debug;
#define Serial Debug
