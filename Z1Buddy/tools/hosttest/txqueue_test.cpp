#include <deque>
#include <vector>
// The output queue (txPump) against a simulated DIN cable: 3.125 bytes per ms.
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>
#include <string>
static uint32_t now_ms = 0; static uint32_t millis() { return now_ms; }
static const int CAP = 96; static std::vector<uint8_t> hw; static std::vector<uint8_t> wire; static std::vector<uint32_t> wireAt; static double credit = 0; static int maxInFlight = 0;

static void tick();
static int spins = 0;   // on the Teensy the cable drains by itself while the code waits; here waiting has to move time
struct Ser { int availableForWrite() { if (++spins > 200) tick(); return CAP - (int)hw.size(); }
  void write(uint8_t b) { hw.push_back(b); if ((int)hw.size() > maxInFlight) maxInFlight = hw.size(); }
  void write(const uint8_t* d, size_t n) { for (size_t i = 0; i < n; i++) write(d[i]); } } Serial1;
static void tick() { spins = 0; now_ms++; credit += 3.125; while (credit >= 1 && !hw.empty()) { wire.push_back(hw.front()); wireAt.push_back(now_ms); hw.erase(hw.begin()); credit -= 1; } if (hw.empty() && credit > 1) credit = 1; }
static int exprPending = 0;      // stands in for the expression scheduler: sends CC messages while there is room
#include "txblock.inc"
static void drainUSBCoalescedToDIN() { while (exprPending > 0 && txRoomForExpression() >= 3) { uint8_t m[3] = { 0xB1, 74, 1 }; Serial1.write(m, 3); exprPending--; } }
static int bgPending = 0;
static void bgTick(uint32_t, bool turn) { if (bgPending == 0 || !txEv.empty() || !txSx.empty() || txInSysEx) return; if (!turn && txInFlight() > 3) return; uint8_t m[11] = { 0xF0, 0x42, 0x30, 0x46, 0x41, 1, 30, 0, 5, 0, 0xF7 }; txSysEx(m, 11); bgPending--; }
static int fails = 0; static void check(const char* l, bool ok) { if (!ok) fails++; printf("%s %s\n", ok ? "ok  " : "FAIL", l); }
static void run(int ms) { for (int i = 0; i < ms; i++) { txPump(); tick(); } }
static int wirePos(uint8_t a, uint8_t b) { for (size_t i = 0; i + 1 < wire.size(); i++) if (wire[i] == a && wire[i + 1] == b) return i; return -1; }
int main() {
  txCapacity = CAP;
  // 1. a note that arrives while a lot of expression is waiting
  exprPending = 400; run(50); size_t before = wire.size();
  uint32_t t0 = now_ms; txEvent3(0x91, 60, 100); run(20); int p = wirePos(0x91, 60);
  printf("     note sent %u ms after it arrived, with %d expression messages still waiting; send buffer never held more than %d bytes\n", wireAt[p] - t0, exprPending, maxInFlight);
  check("a note goes ahead of waiting expression within 8 ms", p >= 0 && wireAt[p] - t0 <= 8);
  check("the Teensy's send buffer stays nearly empty", maxInFlight <= TX_WATERMARK + 3);
  // 2. nothing is lost: 1000 note messages pushed far faster than the cable carries
  exprPending = 0; run(100); wire.clear(); wireAt.clear();
  for (int i = 0; i < 1000; i++) txEvent3(0x90, i % 128, 1 + i % 100);
  run(2000); bool order = wire.size() == 3000; for (int i = 0; order && i < 1000; i++) order = wire[3 * i] == 0x90 && wire[3 * i + 1] == i % 128;
  check("1000 notes pushed at once all arrive, in order", order);
  // 3. sysex is never broken up; a note waits for the message to end, then goes before the next one
  wire.clear(); wireAt.clear(); uint8_t sx[245]; memset(sx, 1, sizeof sx); sx[0] = 0xF0; sx[244] = 0xF7;
  txSysEx(sx, 245); txSysEx(sx, 245); run(10); txEvent3(0x92, 61, 100); run(400);
  int n1 = wirePos(0x92, 61); bool whole = true; for (int i = 1; i < 244; i++) if (wire[i] != 1) whole = false;
  check("a sysex message goes out in one piece", whole && wire[0] == 0xF0 && wire[244] == 0xF7);
  check("the note goes between the two messages, not after both", n1 == 245);
  // 3b. sysex is not starved by constant expression, and does not starve it either
  wire.clear(); wireAt.clear(); exprPending = 100000; int e0 = exprPending; uint8_t pc[11] = { 0xF0, 0x42, 0x30, 0x46, 0x41, 1, 7, 2, 60, 0, 0xF7 };
  for (int i = 0; i < 10; i++) txSysEx(pc, 11); run(400); int sxBytes = 0; for (uint8_t b : wire) if (b == 0xF0) sxBytes++;
  printf("     under constant expression: %d of 10 editor changes sent in 400 ms, expression kept %d%% of the cable\n", sxBytes, (int)((e0 - exprPending) * 3 * 100 / wire.size()));
  check("editor changes get through while expression is constant", sxBytes == 10);
  exprPending = 0; run(50);
  // 4. a sysex message from USB whose end never arrives does not block the notes for ever
  wire.clear(); wireAt.clear(); uint8_t half[20]; memset(half, 2, sizeof half); half[0] = 0xF0; txSysEx(half, 20); run(20); txEvent3(0x93, 62, 100); run(600);
  check("a broken-off sysex is closed and the note still goes", wirePos(0x93, 62) > 0 && wire[20] == 0xF7);
  // 5. background changes are not starved by constant expression
  wire.clear(); wireAt.clear(); exprPending = 100000; bgPending = 5; run(1000);
  check("background changes get through under constant expression", bgPending == 0);
  printf(fails ? "%d FAILED\n" : "all passed\n", fails); return fails; }
