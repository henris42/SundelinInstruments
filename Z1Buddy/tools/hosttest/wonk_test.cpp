#include <deque>
#include <vector>
#include <cstdint>
#include <cstdio>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <string>
#include <map>
#include <set>
static std::string out; static uint32_t now_ms = 1000;
static uint32_t lastNoteEventAt = 0; static int txRoom = 96;
struct Ser { int availableForWrite() { return txRoom; } void write(uint8_t b) { char t[4]; snprintf(t, 4, "%02x ", b); out += t; } } Serial1;
struct EditableValue { int v; int getIval() { return v; } };
std::map<int, EditableValue*> editables;
static bool mpe = false; static std::map<int, int> lo, hi, nsent;
struct Z1Edit { static Z1Edit* getInstance() { static Z1Edit z; return &z; }
  bool inMPE() { return mpe; } void getCurrPatch() { out += "[dump?] "; }
  void sendZ1ProgParm(uint8_t ch, uint8_t grp, int param, int val) { char t[40]; snprintf(t, 40, "[g%d p%d=%d] ", grp, param, val); out += t;
    if (grp == 1) { nsent[param]++; if (!lo.count(param) || val < lo[param]) lo[param] = val; if (!hi.count(param) || val > hi[param]) hi[param] = val; } } };
static long random(long a, long b) { return a + rand() % (b - a); }
static void randomSeed(unsigned) {} static unsigned micros() { return 1; } static uint32_t millis() { return now_ms; }
static std::deque<int> txEv, txSx; static bool txInSysEx = false; static int txInFlight() { return 96 - txRoom; }
static void txFlushExpression(uint8_t) {}
static void txEvent3(uint8_t a, uint8_t b, uint8_t c) { Serial1.write(a); Serial1.write(b); Serial1.write(c); }
#include "block.inc"
static int fails = 0;
static void check(const char* label, bool ok) { bgFlush(); if (!ok) fails++; printf("%s %s%s%s\n", ok ? "ok  " : "FAIL", label, ok ? "" : "\n   got  ", ok ? "" : out.c_str()); out.clear(); }
static void E(int k, int v) { editables[k] = new EditableValue{v}; }
static void run(int steps) { for (int i = 0; i < steps; i++) { now_ms += 150; wonkTick(now_ms); bgFlush(); } }
int main() {
  srand(3);
  E(174, 9); E(188, 0); E(177, 0); E(191, 5); E(204, 0); E(263, 60); E(290, 40);
  for (int ex : {0x800, 0x1000}) for (int id : {465, 470, 626, 634, 648, 702, 708, 700, 719}) E(id | ex, 50);
  wonkPot(99); wonkProgramLoaded(); out.clear(); nsent.clear(); lo.clear(); hi.clear();
  run(2000);
  std::set<int> want = {177, 191, 204, 263, 290, 626 | 0x800, 634 | 0x800, 648 | 0x800, 465 | 0x1000, 470 | 0x1000}; std::set<int> got; bool inrange = true;
  for (auto& kv : nsent) { if (kv.first == 30) continue; got.insert(kv.first); }
  printf("     brass on OSC1, standard on OSC2, 5 minutes at full pot:\n");
  struct { int k; const char* n; int base, depth; } rows[] = { {177, "OSC1 fine", 0, 6}, {191, "OSC2 fine", 5, 6}, {263, "filter 1 cutoff", 60, 8}, {626 | 0x800, "lip character", 50, 5}, {634 | 0x800, "brass noise", 50, 4}, {465 | 0x1000, "OSC2 wave edge", 50, 5} };
  for (auto& r : rows) { printf("       %-16s %d..%d around %d (%d sends)\n", r.n, lo[r.k], hi[r.k], r.base, nsent[r.k]); if (lo[r.k] < r.base - r.depth || hi[r.k] > r.base + r.depth || lo[r.k] == hi[r.k]) inrange = false; }
  int total = 0; for (auto& kv : nsent) total += kv.second; printf("       %d messages in 300 s = %.0f bytes/s of 3125\n", total, total * 11 / 300.0);
  check("only the parameters of these oscillator types drift", got == want);
  check("each stays within its depth and moves", inrange);
  editables[174]->v = 11; out.clear(); nsent.clear(); run(400);
  check("OSC1 changed to plucked: its parameters take over", nsent.count(702 | 0x800) && nsent.count(719 | 0x800) && !nsent.count(626 | 0x800));
  wonkPause(false, true); run(2); check("program change: nothing sent", out.empty());
  run(3); check("asks for the dump after 400 ms", out == "[dump?] ");
  editables[263]->v = 80; wonkProgramLoaded(); out.clear(); lo.clear(); hi.clear(); run(300);
  check("drifts around the new patch value", lo[263] >= 72 && hi[263] <= 88);
  wonkPause(true, false); bool back = true; for (int i = 0; i < wonkCount; i++) if (wonkTargets[i].sent != WONK_UNSENT) back = false;
  check("dump request: everything put back first", back && out.find("p263=80") != std::string::npos);
  run(20); check("held until the dump arrives", out.empty());
  wonkProgramLoaded(); run(100); out.clear(); wonkPot(0); bgFlush(); back = true; for (int i = 0; i < wonkCount; i++) if (wonkTargets[i].sent != WONK_UNSENT) back = false;
  check("pot to 0: all back at the patch values", back && out.find("p30=0") != std::string::npos);
  run(50); check("and stays quiet", out.empty());
  printf(fails ? "%d FAILED\n" : "all passed\n", fails); return fails; }
