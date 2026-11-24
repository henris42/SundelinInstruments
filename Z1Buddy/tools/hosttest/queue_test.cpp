#include <deque>
#include <vector>
#include <cstdint>
#include <cstdio>
#include <cmath>
#include <cstdlib>
#include <string>
#include <map>
static std::string out; static uint32_t now_ms = 1000; static uint32_t lastNoteEventAt = 0; static int txRoom = 96;
struct Ser { int availableForWrite() { return txRoom; } void write(uint8_t b) {} } Serial1;
struct EditableValue { int v; int getIval() { return v; } }; std::map<int, EditableValue*> editables;
struct Z1Edit { static Z1Edit* getInstance() { static Z1Edit z; return &z; } bool inMPE() { return true; } void getCurrPatch() {}
  void sendZ1ProgParm(uint8_t ch, uint8_t grp, int param, int val) { char t[40]; snprintf(t, 40, "[g%d p%d=%d] ", grp, param, val); out += t; } };
static long random(long a, long b) { return a + rand() % (b - a); } static void randomSeed(unsigned) {} static unsigned micros() { return 1; } static uint32_t millis() { return now_ms; }
static std::deque<int> txEv, txSx; static bool txInSysEx = false; static int notesSounding = 1; /* a note is held, so the drift runs */ static int txInFlight() { return 96 - txRoom; }
static void txFlushExpression(uint8_t) {}
static void txEvent3(uint8_t a, uint8_t b, uint8_t c) { Serial1.write(a); Serial1.write(b); Serial1.write(c); }
#include "block.inc"
static int fails = 0; static void check(const char* l, bool ok) { if (!ok) fails++; printf("%s %s%s%s\n", ok ? "ok  " : "FAIL", l, ok ? "" : "\n   got ", ok ? "" : out.c_str()); out.clear(); }
int main() {
  for (int v = 1; v <= 40; v++) bgSend(1, 30, v);            // a pot being turned
  check("nothing goes out by itself", out.empty() && (int)bgQueue.size() == 1);
  txRoom = 40; bgTick(now_ms, false); check("waits while the send buffer is busy", out.empty());
  txRoom = 96; lastNoteEventAt = now_ms; bgTick(now_ms + 1, false); check("waits right after a note", out.empty());
  bgTick(now_ms + 10, false); check("then sends only the newest value", out == "[g1 p30=40] " && (int)bgQueue.size() == 0);
  for (int t = 0; t < 6; t++) bgSend(3, 21 + 27 * t, t); bgSend(1, 263, 70);
  bgTick(now_ms + 20, false); check("one message per pass", out == "[g3 p21=0] ");
  bgClear(1); bgFlush(); check("clear drops one group, flush sends the rest in order", out == "[g3 p48=1] [g3 p75=2] [g3 p102=3] [g3 p129=4] [g3 p156=5] ");
  for (int i = 0; i < 200; i++) bgSend(1, 100 + i, i); check("the queue grows as needed and loses nothing", out.empty() && (int)bgQueue.size() == 200);
  printf(fails ? "%d FAILED\n" : "all passed\n", fails); return fails; }
