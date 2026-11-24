#include <deque>
#include <vector>
#include <cstdint>
#include <cstdio>
#include <cmath>
#include <string>
#include <vector>
static std::string out;
static uint32_t lastNoteEventAt = 0; static int txRoom = 96;
struct Ser { int availableForWrite() { return txRoom; } void write(uint8_t b) { char t[4]; snprintf(t, 4, "%02x ", b); out += t; } } Serial1;
struct Z1Edit { static Z1Edit* getInstance() { static Z1Edit z; return &z; } bool inMPE() { return true; } void getCurrPatch() {}
  void sendZ1ProgParm(uint8_t ch, uint8_t grp, int param, int val) { char t[40]; snprintf(t, 40, "[g%d p%d=%d] ", grp, param, val); out += t; } };
static long random(long a, long b) { return a; } static void randomSeed(unsigned) {} static unsigned micros() { return 1; } static uint32_t millis() { return 0; }
#include <map>
struct EditableValue { int v; int getIval() { return v; } }; std::map<int, EditableValue*> editables;
static std::deque<int> txEv, txSx; static bool txInSysEx = false; static int notesSounding = 1; /* a note is held, so the drift runs */ static int txInFlight() { return 96 - txRoom; }
static void txFlushExpression(uint8_t) {}
static void txEvent3(uint8_t a, uint8_t b, uint8_t c) { Serial1.write(a); Serial1.write(b); Serial1.write(c); }
#include "block.inc"
static int fails = 0;
static void expect(const char* label, const std::string& want) { bgFlush(); bool ok = out == want; if (!ok) fails++; printf("%s %s\n", ok ? "ok  " : "FAIL", label); if (!ok) printf("   got  %s\n   want %s\n", out.c_str(), want.c_str()); out.clear(); }
int main() {
  mpeDetuneCents = 20;
  mpeUnisonChanged(3); out.erase(0, out.find('['));
  expect("detune 3 voices", "[g3 p21=-20] [g3 p48=0] [g3 p75=20] [g3 p102=-20] [g3 p129=0] [g3 p156=20] ");
  mpeUnisonNoteOn(9, 60, 100); expect("note on -> ch 2,3,4", "91 3c 64 92 3c 64 93 3c 64 ");
  mpeUnisonNoteOn(12, 64, 90); expect("second note -> ch 5,6,7", "94 40 5a 95 40 5a 96 40 5a ");
  printf("     groups: in9->%d in12->%d, bend for in12 goes to ch %d..%d\n", mpeGroupFor(9), mpeGroupFor(12), mpeOutCh(mpeGroupFor(12), 0), mpeOutCh(mpeGroupFor(12), 2));
  mpeUnisonNoteOn(3, 67, 80); expect("third note steals the oldest", "81 3c 00 82 3c 00 83 3c 00 91 43 50 92 43 50 93 43 50 ");
  mpeUnisonNoteOff(9, 60, 0); expect("note-off of the stolen note is ignored", "");
  mpeUnisonNoteOff(12, 64, 0); expect("note off", "84 40 00 85 40 00 86 40 00 ");
  mpeUnisonNoteOn(5, 72, 70); expect("new input channel takes the free group", "94 48 46 95 48 46 96 48 46 ");
  mpeUnisonChanged(6); expect("switch to 6: releases, all notes off, detune", "81 43 00 82 43 00 83 43 00 84 48 00 85 48 00 86 48 00 b1 7b 00 b2 7b 00 b3 7b 00 b4 7b 00 b5 7b 00 b6 7b 00 [g3 p21=-20] [g3 p48=-12] [g3 p75=-4] [g3 p102=4] [g3 p129=12] [g3 p156=20] ");
  mpeUnisonNoteOn(2, 60, 100); expect("6 voices", "91 3c 64 92 3c 64 93 3c 64 94 3c 64 95 3c 64 96 3c 64 ");
  mpeDetunePot(99); out.erase(0, 0); expect("pot full -> 50 cents", "[g3 p21=-50] [g3 p48=-30] [g3 p75=-10] [g3 p102=10] [g3 p129=30] [g3 p156=50] ");
  mpeUnisonChanged(1); out.erase(0, out.find('['));
  expect("off: detune cleared", "[g3 p21=0] [g3 p48=0] [g3 p75=0] [g3 p102=0] [g3 p129=0] [g3 p156=0] ");
  mpeUnisonChanged(2); out.clear(); mpeUnisonNoteOn(2, 60, 1); mpeUnisonNoteOn(3, 61, 1); mpeUnisonNoteOn(4, 62, 1);
  expect("2 voices: three notes on ch 2-3, 4-5, 6-7", "91 3c 01 92 3c 01 93 3d 01 94 3d 01 95 3e 01 96 3e 01 ");
  printf(fails ? "%d FAILED\n" : "all passed\n", fails); return fails; }
