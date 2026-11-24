//#include <i2c_device.h>
#include <i2c_driver.h>
#include <i2c_driver_wire.h>
//#include <i2c_register_slave.h>
//#include <string>

//#include <iomanip>
//#include <sstream>
//#include "EventManager.h"
#include <EEPROM.h>
#include <deque>
#include <vector>
#include <Bounce.h>
#include <LiquidCrystal.h>

#include "Z1.h"
#include "SynthBuddy.h"
#include "debug.h"

DebugPort Debug;
// The Multi setup for MPE mode (six timbres on channels 2-7, all on program
// A127) as one sysex message. Made by tools/pull_multi.sh; do not edit by hand.
#include "mpe_multi.h"

//MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI);

//
//using namespace std;

asm(".global _printf_float");

enum CandType : uint8_t { C_CC,
                          C_POLY,
                          C_CHPR,
                          C_PB };

struct Candidate {
  CandType t;
  uint8_t ch;    // 0..15 internal
  uint8_t a;     // CC# or note (for CC/POLY); unused for CHPR/PB
  uint16_t v;    // value (7-bit or 14-bit)
  uint8_t size;  // MIDI bytes (2 or 3)
  float score;   // higher = earlier
};

// Types of the wonkiness code further down. They are up here because the
// Arduino build puts the function prototypes above everything else.
struct WonkSpec {
  int16_t id;       // program parameter ID (without the OSC1/OSC2 ExID)
  int8_t lo, hi;    // range of the parameter
  uint8_t depth;    // how far it may wander at full pot, in parameter steps
};

// A parameter change waiting in the background queue.
struct BgParam {
  uint8_t group;    // 1 = program, 3 = multi
  int16_t id;
  int16_t val;
};

// A short message waiting in the output queue.
struct TxEvent {
  uint8_t n;
  uint8_t b[3];
  uint32_t at;      // when it was put in the queue, for the debug status
};

// Counters for the debug status (Buddy sysex 05).
struct DebugStats {
  uint32_t noteOnIn, noteOffIn;        // from the computer
  uint32_t noteOnOut, noteOffOut;      // written to the cable for the Z1
  uint32_t expressionOut;              // bend, controller, pressure messages written
  uint32_t sysexOut;                   // sysex messages written
  uint32_t sysexStalls;                // sysex messages closed because their end never came
  uint16_t maxEvents;                  // most events waiting at once   } since the
  uint16_t maxEventWaitMs;             // longest an event waited       } last report
  uint16_t maxLoopMs;                  // longest pass of loop()        }
};

struct WonkTarget {
  int key;          // key in the editor = parameter ID with ExID
  int lo, hi;
  float depth;
  float walk;       // where the drift is, -1..1
  int sent;         // value last sent, or WONK_UNSENT = the patch value
};

// MPE voice range: channels [2..MPE_MAX_CHANNEL] receive master PB/CC fan-out
#ifndef MPE_MAX_CHANNEL
#define MPE_MAX_CHANNEL 7  // clamp 2..16
#endif

// ---- Note precedence window (ms) ----
#ifndef NOTE_PRECEDENCE_PAUSE_MS
#define NOTE_PRECEDENCE_PAUSE_MS 2  // pause MPE drains briefly after Note On/Off
#endif

// ---------------- Adaptive Quantization / Drop Policy ----------------
// These are *base* deadbands; the scheduler shrinks them when link has slack.
#define BASE_Q_CC 4    // 7-bit
#define BASE_Q_POLY 5  // 7-bit
#define BASE_Q_CHPR 5  // 7-bit
#define BASE_Q_PB 64   // 14-bit (0..16383)

// Age booster: how fast score grows with age (ms -> multiplier)
#define AGE_BOOST_PER_MS 0.002f

// Refresh to avoid “stuck” values (send even if below deadband)
#define REFRESH_MS_CC 60
#define REFRESH_MS_POLY 120
#define REFRESH_MS_CHPR 60
#define REFRESH_MS_PB 40

// Priority pacing: the shortest time between two messages of one kind. Besides
// the cable, the Z1's own processor is the limit: it is slow, and its panel gets
// sluggish when it is fed more than it needs. At most about 330 pitch bends, 125
// controllers, 100 channel pressures and 80 poly pressures a second.
#define THROTTLE_PB_MS 3
#define THROTTLE_CC_MS 8
#define THROTTLE_POLY_MS 12
#define THROTTLE_CHPR_MS 10

// ---------------- Link/Budget Model ----------------
// MIDI 31,250 bps ~ 3125 bytes/s ~ 3.125 bytes/ms
#define MIDI_BYTES_PER_MS 3.125f

// Limit how many items we score/sort per loop (keeps CPU tiny)
#define MAX_CANDIDATES_PER_LOOP 32

// Type weights (raise PB)
#define WEIGHT_PB 4.0f      // pitch matters most: an off-pitch note is heard at once
#define WEIGHT_CC 1.0f
#define WEIGHT_POLY 0.9f
#define WEIGHT_CHPR 0.8f

// -------- Error Budget (per-channel) --------
// At each drain frame, we compute channel error scores and apportion the total byte budget.
// Give every channel a tiny baseline so quiet channels aren’t starved.
#define CH_MIN_BYTES_BASELINE 2  // minimum bytes each active channel may spend this frame
#define CH_MIN_BYTES_IF_DIRTY 6  // if channel has ANY dirty data, ensure at least this budget


// --- Anti-loop filter for Program Change ---
static uint8_t lastDIN_ProgCh = 0xFF;
static uint8_t lastDIN_ProgChan = 0xFF;
static uint32_t lastDIN_ProgTime = 0;
#define PROG_LOOP_TIMEOUT_MS 200  // time window to ignore echoes

//constants for the number of rows and columns in the LCD
//In my example, I use a 20x4 LCD, thus it has 4 rows and 20 columns
//If you use another LCD, type in the appropriate rows and columns below
const int numRows = 2;
const int numCols = 40;
int led = 13;

int pot0, pot1, pot2;
int pot0_o, pot1_o, pot2_o;
volatile bool int0, int1, int2, int3, int4, int5;

//RS,E,D4,D5,D6,D7
//initialize the library with the numbers of the interface pins
LiquidCrystal lcd(6, 10, 36, 37, 32, 9);
LiquidCrystal lcd3(6, 11, 36, 37, 32, 9);
LiquidCrystal lcd2(6, 12, 36, 37, 32, 9);


#define USE_DEBUG 1
const size_t SYSEX_MAX = 1024;

// ---------- DIN parser state ----------
static uint8_t dinRunningStatus = 0;
static uint8_t dinData1 = 0;
static uint8_t dinNeeded = 0;

static bool dinInSysEx = false;
static bool dinSysExOverflow = false;
static uint8_t sysexBuf[SYSEX_MAX];
static size_t sysexLen = 0;
static uint32_t lastNoteEventAt = 0;

static uint8_t serinbuf[32];
static uint8_t seroutbuf[32];

bool dumpmode;


// For budget accounting
static uint32_t lastDrainAtMs = 0;
// ================== Coalesced + ACTIVE DROP STATE ==================
static uint8_t pendCC[16][128];      // latest value seen
static bool dirtyCC[16][128];        // needs policy check
static uint8_t lastSentCC[16][128];  // 0..127; 255=unset
static uint32_t lastSentTimeCC[16][128];

static uint8_t pendPoly[16][128];
static bool dirtyPoly[16][128];
static uint8_t lastSentPoly[16][128];  // 255=unset
static uint32_t lastSentTimePoly[16][128];

static uint8_t pendChPress[16];
static bool dirtyChPress[16];
static uint8_t lastSentChPress[16];  // 255=unset
static uint32_t lastSentTimeChPr[16];

static uint16_t pendPB[16];
static bool dirtyPB[16];
static uint16_t lastSentPB[16];  // 0..16383; 0xFFFF=unset
static uint32_t lastSentTimePB[16];
static uint16_t globalPB = 8192;  // center

static uint32_t nextCCSendAt = 0, nextPolySendAt = 0, nextChPressSendAt = 0, nextPBSendAt = 0;
static uint16_t ccScan = 0, polyScan = 0;
static uint8_t chPressScan = 0, pbScan = 0;




Bounce S10 = Bounce(34, 150);
Bounce S11 = Bounce(35, 150);
Bounce SI0 = Bounce(7, 150);
Bounce SI1 = Bounce(8, 150);
Bounce SI2 = Bounce(28, 150);
Bounce SI3 = Bounce(29, 150);


//S10
void p34Int() {
  Serial.println("S10");
}

//S11
void p35Int() {
  Serial.println("S11");
}


void extInt0() {
  int0 = true;
}
void extInt1() {
  int1 = true;
}
void extInt2() {
  int2 = true;
}
void extInt3() {
  int3 = true;
}
void extInt4() {
  int4 = true;
}
void extInt5() {
  int5 = true;
}


// Teensy USB MIDI <-> DIN bridge (async, no external libs)
// - Forwards channel/common/real-time both ways
// - SysEx intercepted (<=1024 bytes) and delivered as one chunk to onSysEx()
// - Oversize SysEx discarded via onSysExTooLong()
// - USB Type: "Serial + MIDI" (or "MIDI" if you don't need Serial monitor)
// - DIN: Serial1 @ 31250

// ============ Output to the Z1: one priority queue ============
// A DIN cable carries 3,125 bytes a second and everything for the Z1 has to
// share it. Nothing writes to the port directly any more; it all goes through
// txPump(), which decides what the cable carries next:
//
//   1. events    notes, program changes, bank selects, pedal: in order, never
//                dropped (txEvent)
//   2. expression  per-note bend, controllers, pressure: only the newest value
//                of each stream, pitch bend first, thinned to what fits
//                (the scheduler, drainUSBCoalescedToDIN)
//   3. sysex     editor changes, mode changes, dumps: in order, never
//                dropped, each message in one piece (txSysEx). It goes when
//                the expression leaves the cable free, and has a turn of its
//                own every TX_SYSEX_TURN_MS so that playing can not hold it
//                up for ever.
//   4. background  wonkiness drift and timbre detune: newest value only, when
//                nothing else is waiting (bgSend)
//
// The Teensy's own send buffer is kept almost empty (TX_WATERMARK bytes), so
// what waits, waits here, where a note can still go in front of it. Without
// that, a note would queue behind up to 96 bytes of older data: 30 ms.
// The one thing a note can not overtake is a sysex message that has started.
static constexpr int TX_WATERMARK = 16;          // bytes allowed in the Teensy's send buffer: 5 ms
static constexpr int TX_SYSEX_STALL_MS = 300;    // a sysex message whose end never comes is closed after this
static constexpr int TX_SYSEX_TURN_MS = 20;      // a waiting sysex message goes at least this often

// The queues grow as needed (the Teensy has half a megabyte to spare), so
// nothing is ever dropped or made to wait for want of space.
// ---- Which notes are on, as the Buddy sees it
// noteIn: what the computer has sent (note-ons without their note-off yet).
// noteOut: what has actually been written to the cable for the Z1.
// When a note hangs, these tell where the note-off went missing: if the note
// is still in noteIn, it never arrived from the computer; if it is in neither,
// the Buddy sent it and the Z1 did not take it. Buddy sysex 05 reports them.
static uint8_t noteIn[16][128];
static uint8_t noteOut[16][128];

static DebugStats dbgStats;
static int notesSounding = 0;        // notes that are on at the Z1 right now

static inline void noteCount(uint8_t table[16][128], uint8_t status, uint8_t note, uint8_t vel) {
  uint8_t c = status & 0x0F, type = status & 0xF0;
  bool in = (table == noteIn);
  note &= 0x7F;
  if (type == 0x90 && vel > 0) {
    if (table[c][note] < 255) table[c][note]++;
    if (in) dbgStats.noteOnIn++; else { dbgStats.noteOnOut++; notesSounding++; }
  } else if (type == 0x80 || type == 0x90) {
    if (!in && table[c][note] > 0 && notesSounding > 0) notesSounding--;
    if (table[c][note] > 0) table[c][note]--;
    if (in) dbgStats.noteOffIn++; else dbgStats.noteOffOut++;
  }
}

static std::deque<TxEvent> txEv;
static std::deque<uint8_t> txSx;
static bool txInSysEx = false;   // an F0 has gone out and its F7 has not yet
static uint32_t txSysExAt = 0;   // when the last byte of that message went out
static int txCapacity = 0;       // size of the Teensy's send buffer, measured in setup()
static uint32_t txBgAt = 0;      // when a background message last went out
static uint32_t txSxTurnAt = 0;  // when a sysex message was last started

static void drainUSBCoalescedToDIN();
static void bgTick(uint32_t now, bool turn);

// Bytes waiting in the Teensy's own send buffer.
static inline int txInFlight() {
  return txCapacity - Serial1.availableForWrite();
}

// Room the expression scheduler may use right now.
static inline int txRoomForExpression() {
  int room = TX_WATERMARK - txInFlight();
  return room > 0 ? room : 0;
}

// Feeds the cable. Called on every pass of loop().
static void txPump() {
  if (txCapacity == 0) txCapacity = Serial1.availableForWrite();
  uint32_t now = millis();
  for (;;) {
    if (txInFlight() >= TX_WATERMARK) return;
    if (txInSysEx) {
      if (txSx.empty()) {
        // The rest of the message has not arrived yet (it comes from USB in
        // pieces). If it never does, close it so the notes are not held up.
        if (now - txSysExAt < TX_SYSEX_STALL_MS) return;
        Serial1.write((uint8_t)0xF7);
        txInSysEx = false;
        dbgStats.sysexStalls++;
        continue;
      }
      uint8_t b = txSx.front();
      txSx.pop_front();
      Serial1.write(b);
      txSysExAt = now;
      if (b == 0xF7) { txInSysEx = false; dbgStats.sysexOut++; }
      continue;
    }
    if (!txEv.empty()) {                       // 1. events
      TxEvent e = txEv.front();
      txEv.pop_front();
      Serial1.write(e.b, e.n);
      if (e.n == 3) noteCount(noteOut, e.b[0], e.b[1], e.b[2]);
      uint32_t waited = now - e.at;
      if (waited > dbgStats.maxEventWaitMs) dbgStats.maxEventWaitMs = waited > 65535 ? 65535 : (uint16_t)waited;
      continue;
    }
    if (!txSx.empty() && now - txSxTurnAt >= TX_SYSEX_TURN_MS) {
      txInSysEx = true;                        // 3. sysex, on its guaranteed turn
      txSysExAt = txSxTurnAt = now;
      continue;
    }
    break;
  }
  if (now - txBgAt >= 100 && txSx.empty()) { // 4. background, on its guaranteed turn:
    bgTick(now, true);                         //    it joins the sysex queue
    if (!txSx.empty()) txBgAt = now;
  }
  drainUSBCoalescedToDIN();                    // 2. expression
  if (!txSx.empty() && txInFlight() <= 3) {    // 3. sysex, because the cable is free
    txInSysEx = true;
    txSysExAt = txSxTurnAt = now;
    return;
  }
  bgTick(now, false);                          // 4. background, because the cable is free
}

// A short message: note, controller, program change, system common.
static void txEvent(const uint8_t* data, size_t n) {
  if (n == 0 || n > 3) return;
  TxEvent e;
  e.n = (uint8_t)n;
  for (size_t i = 0; i < n; ++i) e.b[i] = data[i];
  e.at = millis();
  txEv.push_back(e);
  if (txEv.size() > dbgStats.maxEvents) dbgStats.maxEvents = txEv.size() > 65535 ? 65535 : (uint16_t)txEv.size();
}

static inline void txEvent3(uint8_t a, uint8_t b, uint8_t c) {
  uint8_t m[3] = { a, b, c };
  txEvent(m, 3);
}

// Sysex bytes: a whole message or a piece of one. Messages must not be mixed
// by the callers; they go out in the order they are put in.
void txSysEx(const uint8_t* data, size_t n) {
  txSx.insert(txSx.end(), data, data + n);
}

// For Z1.cpp, which builds its sysex messages a byte at a time.
void z1Write(uint8_t b) {
  txSysEx(&b, 1);
}
void z1Write(const uint8_t* data, size_t n) {
  txSysEx(data, n);
}

// A realtime byte (clock, start, stop) may go anywhere, even inside a sysex
// message, and timing is all it is for: straight to the port.
static inline void txRealtime(uint8_t b) {
  Serial1.write(b);
}

void sendcc(uint8_t channel, uint8_t cc, uint8_t value) {
  txEvent3(0xB0 | channel, cc, value);
}

// ---- Start and end values of a note
// Expression is thinned on its way to the Z1, but what a note starts with and
// what it ends with must be exact. So just before a note-on or a note-off goes
// out, every value still waiting for that channel is sent as it is, in the
// event queue, ahead of the note: the note starts at the right pitch and
// filter position, and ends on its final values.
// (Values that change after the note-off, in the release, are still thinned;
// the scheduler always delivers the last one, and a return to 0 at once.)
static void initLastSentIfNeeded();

static void txFlushExpression(uint8_t ch) {   // ch 1..16
  if (ch < 1 || ch > 16) return;
  initLastSentIfNeeded();
  uint8_t c = ch - 1;
  uint32_t now = millis();
  if (dirtyPB[c]) {
    uint16_t v = pendPB[c];
    if (v != lastSentPB[c]) txEvent3(0xE0 | c, v & 0x7F, (v >> 7) & 0x7F);
    lastSentPB[c] = v;
    lastSentTimePB[c] = now;
    dirtyPB[c] = false;
  }
  if (dirtyChPress[c]) {
    uint8_t v = pendChPress[c];
    if (v != lastSentChPress[c]) {
      uint8_t m[2] = { (uint8_t)(0xD0 | c), v };
      txEvent(m, 2);
    }
    lastSentChPress[c] = v;
    lastSentTimeChPr[c] = now;
    dirtyChPress[c] = false;
  }
  for (int i = 0; i < 128; ++i) {
    if (dirtyCC[c][i]) {
      uint8_t v = pendCC[c][i];
      if (v != lastSentCC[c][i]) txEvent3(0xB0 | c, (uint8_t)i, v);
      lastSentCC[c][i] = v;
      lastSentTimeCC[c][i] = now;
      dirtyCC[c][i] = false;
    }
    if (dirtyPoly[c][i]) {
      uint8_t v = pendPoly[c][i];
      if (v != lastSentPoly[c][i]) txEvent3(0xA0 | c, (uint8_t)i, v);
      lastSentPoly[c][i] = v;
      lastSentTimePoly[c][i] = now;
      dirtyPoly[c][i] = false;
    }
  }
}



// ---------- Helpers ----------
static inline uint8_t dataBytesNeeded(uint8_t status) {
  if (status >= 0xF0) {
    switch (status) {
      case 0xF0: return 0;  // SysEx start (handled separately)
      case 0xF1: return 1;  // MTC Quarter Frame
      case 0xF2: return 2;  // Song Position Pointer
      case 0xF3: return 1;  // Song Select
      case 0xF6: return 0;  // Tune Request
      case 0xF7: return 0;  // End of SysEx
      default: return 0;    // Real-time handled immediately
    }
  }
  switch (status & 0xF0) {
    case 0x80:
    case 0x90:
    case 0xA0:
    case 0xB0:
    case 0xE0: return 2;
    case 0xC0:
    case 0xD0: return 1;
    default: return 0;
  }
}

#if USE_DEBUG
static void dbg(const char* s) {
  Serial.println(s);
}
#else
static void dbg(const char*) {}
#endif


// Initialize lastSent as "unset"
static void initLastSentIfNeeded() {
  static bool inited = false;
  if (inited) return;
  inited = true;
  for (int ch = 0; ch < 16; ++ch) {
    for (int i = 0; i < 128; ++i) {
      lastSentCC[ch][i] = 0xFF;
      lastSentPoly[ch][i] = 0xFF;
      lastSentTimeCC[ch][i] = 0;
      lastSentTimePoly[ch][i] = 0;
      dirtyCC[ch][i] = false;
      dirtyPoly[ch][i] = false;
    }
    lastSentChPress[ch] = 0xFF;
    lastSentPB[ch] = 0xFFFF;
    lastSentTimeChPr[ch] = 0;
    lastSentTimePB[ch] = 0;
    dirtyChPress[ch] = false;
    dirtyPB[ch] = false;
  }
  lastDrainAtMs = millis();
}

// ---------------- Mark helpers ----------------
static inline void markCC(uint8_t ch, uint8_t cc, uint8_t val) {
  if (ch < 1 || ch > 16) return;
  pendCC[ch - 1][cc] = val;
  dirtyCC[ch - 1][cc] = true;
}
static inline void markPoly(uint8_t ch, uint8_t note, uint8_t val) {
  if (ch < 1 || ch > 16) return;
  pendPoly[ch - 1][note] = val;
  dirtyPoly[ch - 1][note] = true;
}
static inline void markChPress(uint8_t ch, uint8_t val) {
  if (ch < 1 || ch > 16) return;
  pendChPress[ch - 1] = val;
  dirtyChPress[ch - 1] = true;
}
// Pitch bend.
// Normal mode: everything is on channel 1 and a bend is simply the bend.
// MPE mode: a bend on a note channel is that note's own, scaled to the pitch
// bend range of the sound (mpeScaleBend). A bend on channel 1 is for all
// notes: it is added to each of them as it is, at once, so a pitch wheel
// moves every sounding note by the sound's own range.
static uint16_t rawPB[16] = { 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192,
                              8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192 };   // each note channel's own bend

// A note's own bend arrives in MPE scale: full travel means 48 semitones.
// The Z1 plays a bend in the scale of the sound: full travel is its pitch
// bend range, parameters 163 (up) and 164 (down), usually 2 semitones. So the
// bend is multiplied by 48 / range; a key bent a semitone then sounds a
// semitone higher, whatever the range of the sound, and with a small range
// the bend gets the Z1's full resolution. What does not fit the range is
// held at its end. A sound whose range is 0 or reversed is left unscaled.
// 48 is what MPE prescribes unless the controller says otherwise; it says so
// with registered parameter 0 (pitch bend sensitivity) on a note channel,
// which the Buddy follows (mpeRpn).
static int mpeBendSemitones = 48;
static int mpeBendCents = 0;          // the fraction of a semitone on top, 0-99 (RPN 0 "fine")
// How much of that true pitch a key's bend gets, in percent. 100: a key bent
// a semitone sounds a semitone higher. Lower makes the keys less sensitive to
// sideways movement. Set with Buddy sysex 07 (tools/buddy bend <percent>)
// and kept when the Buddy is switched off.
static int mpeBendPercent = 100;

static inline int32_t mpeScaleBend(int32_t offset) {   // offset from the centre, -8192..8191
  int range = 0;
  if (offset >= 0) {
    if (editables.count(163)) range = editables[163]->getIval();
  } else {
    if (editables.count(164)) range = -editables[164]->getIval();
  }
  if (range <= 0) return offset;
  // in cents, so a range such as 81.92 semitones (one cent per step) is exact
  int32_t scaled = (int32_t)((int64_t)offset * (mpeBendSemitones * 100 + mpeBendCents) / (range * 100) * mpeBendPercent / 100);
  if (scaled > 8191) scaled = 8191;
  if (scaled < -8192) scaled = -8192;
  return scaled;
}

static inline void setChannelPB(uint8_t c) {   // c = 0..15
  int32_t summed = 8192 + mpeScaleBend((int32_t)rawPB[c] - 8192) + ((int32_t)globalPB - 8192);
  if (summed < 0) summed = 0;
  if (summed > 16383) summed = 16383;
  pendPB[c] = (uint16_t)summed;
  dirtyPB[c] = true;
}

// The key bend range and depth are settings of the Buddy and are kept when it
// is switched off (four bytes of the Teensy's EEPROM, from address 0).
static constexpr uint8_t BEND_SETTINGS_MAGIC = 0xB5;

static void bendSettingsSave() {
  EEPROM.update(0, BEND_SETTINGS_MAGIC);
  EEPROM.update(1, (uint8_t)mpeBendSemitones);
  EEPROM.update(2, (uint8_t)mpeBendCents);
  EEPROM.update(3, (uint8_t)mpeBendPercent);
}

static void bendSettingsLoad() {
  if (EEPROM.read(0) != BEND_SETTINGS_MAGIC) return;
  uint8_t st = EEPROM.read(1), ct = EEPROM.read(2), pc = EEPROM.read(3);
  if (st >= 1 && st <= 96) mpeBendSemitones = st;
  if (ct <= 99) mpeBendCents = ct;
  if (pc <= 127) mpeBendPercent = pc;
}

// Registered parameter 0, "pitch bend sensitivity", on a note channel: how
// many semitones the controller means by a full bend of a key. MPE fixes it
// at 48 unless the controller sends this. The three controllers that make up
// the message (101, 100, then 6) are taken here and not passed to the Z1,
// which has no registered parameters. Returns true if cc was one of them.
static uint8_t mpeRpnMsb[16], mpeRpnLsb[16];
static bool mpeRpnInit = false;

static bool mpeRpn(uint8_t ch, uint8_t cc, uint8_t val) {   // ch 2..16
  if (!mpeRpnInit) {
    for (int i = 0; i < 16; ++i) mpeRpnMsb[i] = mpeRpnLsb[i] = 127;   // 127/127 = none selected
    mpeRpnInit = true;
  }
  uint8_t c = ch - 1;
  if (cc == 101) { mpeRpnMsb[c] = val; return true; }
  if (cc == 100) { mpeRpnLsb[c] = val; return true; }
  bool bendRange = mpeRpnMsb[c] == 0 && mpeRpnLsb[c] == 0;
  if (cc == 6 && bendRange) {                 // semitones; the cents start again at 0
    if (val >= 1 && val <= 96) {
      mpeBendSemitones = val;
      mpeBendCents = 0;
      bendSettingsSave();
      Serial.printf("MPE: a full key bend is %d semitones\r\n", val);
    }
    return true;
  }
  if (cc == 38 && bendRange) {                // cents on top
    if (val <= 99) {
      mpeBendCents = val;
      bendSettingsSave();
      Serial.printf("MPE: a full key bend is %d.%02d semitones\r\n", mpeBendSemitones, val);
    }
    return true;
  }
  return false;
}

static inline void markPB(uint8_t ch, uint16_t v14) {
  if (ch < 1 || ch > 16) return;
  if (Z1Edit::getInstance()->getMainState() == mainState::NORMAL) {
    pendPB[ch - 1] = v14;
    dirtyPB[ch - 1] = true;
    return;
  }
  if (ch == 1) {
    globalPB = v14;
    uint8_t last = (MPE_MAX_CHANNEL < 2) ? 1 : (MPE_MAX_CHANNEL > 16 ? 16 : MPE_MAX_CHANNEL);
    for (uint8_t c = 1; c < last; ++c) setChannelPB(c);
  } else {
    rawPB[ch - 1] = v14;
    setChannelPB(ch - 1);
  }
}

// ---------------- Active-drop tests ----------------
static inline bool shouldSend7(uint8_t newv, uint8_t lastv, uint8_t base_q, float slack_factor, uint32_t now, uint32_t last_ts) {
  if (newv == 0) return (lastv != 0);
  if (lastv == 0xFF) return true;
  float age_ms = last_ts ? (float)(now - last_ts) : 1.0f;
  float qeff = (float)base_q / (1.0f + slack_factor * 0.7f) / (1.0f + AGE_BOOST_PER_MS * age_ms);
  if (qeff < 1.0f) qeff = 1.0f;
  uint8_t diff = (newv > lastv) ? (newv - lastv) : (lastv - newv);
  return diff >= (uint8_t)qeff;
}
static inline bool shouldSend14(uint16_t newv, uint16_t lastv, uint16_t base_q, float slack_factor, uint32_t now, uint32_t last_ts) {
  if (newv == 0) return (lastv != 0);
  if (lastv == 0xFFFF) return true;
  float age_ms = last_ts ? (float)(now - last_ts) : 1.0f;
  float qeff = (float)base_q / (1.0f + slack_factor * 0.7f) / (1.0f + AGE_BOOST_PER_MS * age_ms);
  if (qeff < 1.0f) qeff = 1.0f;
  uint16_t diff = (newv > lastv) ? (newv - lastv) : (lastv - newv);
  return diff >= (uint16_t)qeff;
}

// ---------------- Candidate and PQ ----------------


static void pq_insert(Candidate* arr, int& n, const Candidate& c) {
  if (n >= MAX_CANDIDATES_PER_LOOP && c.score <= arr[n - 1].score) return;
  if (n < MAX_CANDIDATES_PER_LOOP) n++;
  int i = n - 2;
  while (i >= 0 && arr[i].score < c.score) {
    arr[i + 1] = arr[i];
    i--;
  }
  arr[i + 1] = c;
}

// ---------------- Budget helpers ----------------
static inline int compute_bytes_budget(uint32_t now) {
  if (lastDrainAtMs == 0) lastDrainAtMs = now;
  uint32_t dt = now - lastDrainAtMs;
  float gen = dt * MIDI_BYTES_PER_MS;      // bytes “available” since last drain
  int room = txRoomForExpression();        // keep the Teensy's send buffer nearly empty
  int budget = (int)gen;
  if (budget > room) budget = room;
  if (budget < 0) budget = 0;
  return budget;
}
static inline float slack_factor_from_budget(int bytes_budget) {
  if (bytes_budget <= 0) return 0.0f;
  if (bytes_budget >= 24) return 2.0f;
  return bytes_budget / 12.0f;  // up to ~2.0
}

// ---------------- Per-channel ERROR BUDGET allocation ----------------
static void compute_channel_error_scores(uint32_t now, float chErr[16], bool chHasDirty[16]) {
  for (int ch = 0; ch < 16; ++ch) {
    chErr[ch] = 0.0f;
    chHasDirty[ch] = false;
  }

  // PB
  for (int ch = 0; ch < 16; ++ch)
    if (dirtyPB[ch]) {
      chHasDirty[ch] = true;
      uint16_t nv = pendPB[ch], lv = lastSentPB[ch];
      float err = (lv == 0xFFFF) ? (float)BASE_Q_PB : (float)abs((int)nv - (int)lv);
      chErr[ch] += WEIGHT_PB * (err / (float)BASE_Q_PB);
    }
  // CC (scan all)
  for (int idx = 0; idx < 2048; ++idx) {
    int ch = idx / 128;
    int cc = idx % 128;
    if (!dirtyCC[ch][cc]) continue;
    chHasDirty[ch] = true;
    uint8_t nv = pendCC[ch][cc], lv = lastSentCC[ch][cc];
    float err = (lv == 0xFF) ? (float)BASE_Q_CC : (float)abs((int)nv - (int)lv);
    chErr[ch] += WEIGHT_CC * (err / (float)BASE_Q_CC);
  }
  // Poly
  for (int idx = 0; idx < 2048; ++idx) {
    int ch = idx / 128;
    int note = idx % 128;
    if (!dirtyPoly[ch][note]) continue;
    chHasDirty[ch] = true;
    uint8_t nv = pendPoly[ch][note], lv = lastSentPoly[ch][note];
    float err = (lv == 0xFF) ? (float)BASE_Q_POLY : (float)abs((int)nv - (int)lv);
    chErr[ch] += WEIGHT_POLY * (err / (float)BASE_Q_POLY);
  }
  // Channel Pressure
  for (int ch = 0; ch < 16; ++ch)
    if (dirtyChPress[ch]) {
      chHasDirty[ch] = true;
      uint8_t nv = pendChPress[ch], lv = lastSentChPress[ch];
      float err = (lv == 0xFF) ? (float)BASE_Q_CHPR : (float)abs((int)nv - (int)lv);
      chErr[ch] += WEIGHT_CHPR * (err / (float)BASE_Q_CHPR);
    }
}

static void apportion_channel_budgets(int totalBytes, const float chErr[16], const bool chHasDirty[16], int chBytes[16]) {
  // Sum errors
  float sumErr = 0.0f;
  for (int ch = 0; ch < 16; ++ch) sumErr += chErr[ch];

  // Baseline guarantees
  int activeChannels = 0;
  for (int ch = 0; ch < 16; ++ch)
    if (chHasDirty[ch]) activeChannels++;

  int baseline = 0;
  for (int ch = 0; ch < 16; ++ch) {
    if (chHasDirty[ch]) {
      chBytes[ch] = CH_MIN_BYTES_IF_DIRTY;
      baseline += CH_MIN_BYTES_IF_DIRTY;
    } else {
      chBytes[ch] = CH_MIN_BYTES_BASELINE;
      baseline += CH_MIN_BYTES_BASELINE;
    }
  }
  if (baseline > totalBytes) {
    // Not enough room even for baselines; scale them down
    float scale = (float)totalBytes / (float)baseline;
    int used = 0;
    for (int ch = 0; ch < 16; ++ch) {
      chBytes[ch] = (int)((float)chBytes[ch] * scale);
      if (chBytes[ch] < 0) chBytes[ch] = 0;
      used += chBytes[ch];
    }
    // Any leftover due to rounding
    while (used < totalBytes) {
      int best = -1;
      float bestErr = -1.0f;
      for (int ch = 0; ch < 16; ++ch)
        if (chHasDirty[ch]) {
          if (chErr[ch] > bestErr) {
            bestErr = chErr[ch];
            best = ch;
          }
        }
      if (best < 0) break;
      chBytes[best]++;
      used++;
    }
    return;
  }

  int remaining = totalBytes - baseline;
  if (remaining <= 0 || sumErr <= 0.0f) return;

  // Distribute remaining proportionally by error scores
  int used = baseline;
  for (int ch = 0; ch < 16; ++ch) {
    int add = (int)((chErr[ch] / sumErr) * remaining);
    chBytes[ch] += add;
    used += add;
  }
  // Fix rounding leftovers
  while (used < totalBytes) {
    int best = -1;
    float bestErr = -1.0f;
    for (int ch = 0; ch < 16; ++ch) {
      if (chErr[ch] > bestErr) {
        bestErr = chErr[ch];
        best = ch;
      }
    }
    if (best < 0) break;
    chBytes[best]++;
    used++;
  }
}

// ---------------- Priority drain with error budget ----------------
static void drainUSBCoalescedToDIN() {
  initLastSentIfNeeded();

  // NOTE-FIRST RULE: pause MPE drains briefly after a note event
  const uint32_t now = millis();
  if ((now - lastNoteEventAt) < NOTE_PRECEDENCE_PAUSE_MS) return;

  // Compute total bytes budget for this frame
  int totalBudget = compute_bytes_budget(now);
  if (totalBudget <= 0) return;

  float slack = slack_factor_from_budget(totalBudget);

  // Build a small PQ of best candidates overall
  Candidate pq[MAX_CANDIDATES_PER_LOOP];
  int pn = 0;

  // Gather candidates (PB first scan so they tend to rank higher)
  if (now >= nextPBSendAt) {
    for (uint8_t k = 0; k < 16; ++k) {
      uint8_t ch = (pbScan + k) & 0x0F;
      if (!dirtyPB[ch]) continue;
      uint16_t nv = pendPB[ch], lv = lastSentPB[ch];
      bool sendZero = (nv == 0 && lv != 0);
      bool bigEnough = shouldSend14(nv, lv, BASE_Q_PB, slack, now, lastSentTimePB[ch]);
      if (sendZero || bigEnough) {
        Candidate c;
        c.t = C_PB;
        c.ch = ch;
        c.a = 0;
        c.v = nv;
        c.size = 3;
        float err = (lv == 0xFFFF) ? (float)BASE_Q_PB : (float)abs((int)nv - (int)lv);
        float age_ms = lastSentTimePB[ch] ? (float)(now - lastSentTimePB[ch]) : 1.0f;
        c.score = WEIGHT_PB * (err / (float)BASE_Q_PB) * (1.0f + AGE_BOOST_PER_MS * age_ms);
        pq_insert(pq, pn, c);
      } else if (lastSentTimePB[ch] && (now - lastSentTimePB[ch] >= REFRESH_MS_PB) && nv != lv) {
        Candidate c;
        c.t = C_PB;
        c.ch = ch;
        c.a = 0;
        c.v = nv;
        c.size = 3;
        c.score = WEIGHT_PB * 0.7f;
        pq_insert(pq, pn, c);
      }
      if (pn >= MAX_CANDIDATES_PER_LOOP) break;
    }
  }

  if (now >= nextCCSendAt && pn < MAX_CANDIDATES_PER_LOOP) {
    for (uint16_t i = 0; i < 2048; ++i) {
      uint16_t idx = (ccScan + i) & 2047;
      uint8_t ch = (idx / 128), cc = (idx % 128);
      if (!dirtyCC[ch][cc]) continue;
      uint8_t nv = pendCC[ch][cc], lv = lastSentCC[ch][cc];
      bool sendZero = (nv == 0 && lv != 0);
      bool bigEnough = shouldSend7(nv, lv, BASE_Q_CC, slack, now, lastSentTimeCC[ch][cc]);
      if (sendZero || bigEnough) {
        Candidate c;
        c.t = C_CC;
        c.ch = ch;
        c.a = cc;
        c.v = nv;
        c.size = 3;
        float err = (lv == 0xFF) ? (float)BASE_Q_CC : (float)abs((int)nv - (int)lv);
        float age_ms = lastSentTimeCC[ch][cc] ? (float)(now - lastSentTimeCC[ch][cc]) : 1.0f;
        c.score = WEIGHT_CC * (err / (float)BASE_Q_CC) * (1.0f + AGE_BOOST_PER_MS * age_ms);
        pq_insert(pq, pn, c);
      } else if (lastSentTimeCC[ch][cc] && (now - lastSentTimeCC[ch][cc] >= REFRESH_MS_CC) && nv != lv) {
        Candidate c;
        c.t = C_CC;
        c.ch = ch;
        c.a = cc;
        c.v = nv;
        c.size = 3;
        c.score = WEIGHT_CC * 0.75f;
        pq_insert(pq, pn, c);
      }
      if (pn >= MAX_CANDIDATES_PER_LOOP) break;
    }
  }

  if (now >= nextPolySendAt && pn < MAX_CANDIDATES_PER_LOOP) {
    for (uint16_t i = 0; i < 2048; ++i) {
      uint16_t idx = (polyScan + i) & 2047;
      uint8_t ch = (idx / 128), note = (idx % 128);
      if (!dirtyPoly[ch][note]) continue;
      uint8_t nv = pendPoly[ch][note], lv = lastSentPoly[ch][note];
      bool sendZero = (nv == 0 && lv != 0);
      bool bigEnough = shouldSend7(nv, lv, BASE_Q_POLY, slack, now, lastSentTimePoly[ch][note]);
      if (sendZero || bigEnough) {
        Candidate c;
        c.t = C_POLY;
        c.ch = ch;
        c.a = note;
        c.v = nv;
        c.size = 3;
        float err = (lv == 0xFF) ? (float)BASE_Q_POLY : (float)abs((int)nv - (int)lv);
        float age_ms = lastSentTimePoly[ch][note] ? (float)(now - lastSentTimePoly[ch][note]) : 1.0f;
        c.score = WEIGHT_POLY * (err / (float)BASE_Q_POLY) * (1.0f + AGE_BOOST_PER_MS * age_ms);
        pq_insert(pq, pn, c);
      } else if (lastSentTimePoly[ch][note] && (now - lastSentTimePoly[ch][note] >= REFRESH_MS_POLY) && nv != lv) {
        Candidate c;
        c.t = C_POLY;
        c.ch = ch;
        c.a = note;
        c.v = nv;
        c.size = 3;
        c.score = WEIGHT_POLY * 0.70f;
        pq_insert(pq, pn, c);
      }
      if (pn >= MAX_CANDIDATES_PER_LOOP) break;
    }
  }

  if (now >= nextChPressSendAt && pn < MAX_CANDIDATES_PER_LOOP) {
    for (uint8_t k = 0; k < 16; ++k) {
      uint8_t ch = (chPressScan + k) & 0x0F;
      if (!dirtyChPress[ch]) continue;
      uint8_t nv = pendChPress[ch], lv = lastSentChPress[ch];
      bool sendZero = (nv == 0 && lv != 0);
      bool bigEnough = shouldSend7(nv, lv, BASE_Q_CHPR, slack, now, lastSentTimeChPr[ch]);
      if (sendZero || bigEnough) {
        Candidate c;
        c.t = C_CHPR;
        c.ch = ch;
        c.a = 0;
        c.v = nv;
        c.size = 2;
        float err = (lv == 0xFF) ? (float)BASE_Q_CHPR : (float)abs((int)nv - (int)lv);
        float age_ms = lastSentTimeChPr[ch] ? (float)(now - lastSentTimeChPr[ch]) : 1.0f;
        c.score = WEIGHT_CHPR * (err / (float)BASE_Q_CHPR) * (1.0f + AGE_BOOST_PER_MS * age_ms);
        pq_insert(pq, pn, c);
      } else if (lastSentTimeChPr[ch] && (now - lastSentTimeChPr[ch] >= REFRESH_MS_CHPR) && nv != lv) {
        Candidate c;
        c.t = C_CHPR;
        c.ch = ch;
        c.a = 0;
        c.v = nv;
        c.size = 2;
        c.score = WEIGHT_CHPR * 0.60f;
        pq_insert(pq, pn, c);
      }
      if (pn >= MAX_CANDIDATES_PER_LOOP) break;
    }
  }

  if (pn == 0) {
    lastDrainAtMs = now;
    return;
  }

  // ---- Per-channel ERROR BUDGET allocation ----
  float chErr[16];
  bool chHasDirty[16];
  compute_channel_error_scores(now, chErr, chHasDirty);

  int chBytesBudget[16];
  for (int ch = 0; ch < 16; ++ch) chBytesBudget[ch] = 0;
  apportion_channel_budgets(totalBudget, chErr, chHasDirty, chBytesBudget);

  // ---- Emit by descending score, respecting both total & per-channel budgets ----
  int budgetLeft = totalBudget;
  for (int i = 0; i < pn && budgetLeft > 0; ++i) {
    Candidate& c = pq[i];
    int need = c.size;
    if (need > budgetLeft) continue;
    if (txRoomForExpression() < need) continue;
    if (chBytesBudget[c.ch] < need) continue;  // not enough per-channel budget

    // Send
    switch (c.t) {
      case C_PB:
        sendPB_toDIN(c.ch + 1, (uint16_t)c.v);
        lastSentPB[c.ch] = (uint16_t)c.v;
        lastSentTimePB[c.ch] = now;
        dirtyPB[c.ch] = false;
        pbScan = (c.ch + 1) & 0x0F;
        nextPBSendAt = now + THROTTLE_PB_MS;
        break;

      case C_CC:
        sendCC_toDIN(c.ch + 1, c.a, (uint8_t)c.v);
        lastSentCC[c.ch][c.a] = (uint8_t)c.v;
        lastSentTimeCC[c.ch][c.a] = now;
        dirtyCC[c.ch][c.a] = false;
        ccScan = ((c.ch * 128 + c.a) + 1) & 2047;
        nextCCSendAt = now + THROTTLE_CC_MS;
        break;

      case C_POLY:
        sendPoly_toDIN(c.ch + 1, c.a, (uint8_t)c.v);
        lastSentPoly[c.ch][c.a] = (uint8_t)c.v;
        lastSentTimePoly[c.ch][c.a] = now;
        dirtyPoly[c.ch][c.a] = false;
        polyScan = ((c.ch * 128 + c.a) + 1) & 2047;
        nextPolySendAt = now + THROTTLE_POLY_MS;
        break;

      case C_CHPR:
        sendChPress_toDIN(c.ch + 1, (uint8_t)c.v);
        lastSentChPress[c.ch] = (uint8_t)c.v;
        lastSentTimeChPr[c.ch] = now;
        dirtyChPress[c.ch] = false;
        chPressScan = (c.ch + 1) & 0x0F;
        nextChPressSendAt = now + THROTTLE_CHPR_MS;
        break;
    }

    chBytesBudget[c.ch] -= need;
    budgetLeft -= need;
  }

  // Update global drain timestamp
  lastDrainAtMs = now;
}



// ============ App hooks ============
static void wonkProgramLoaded();   // further down, with the rest of the wonkiness
void mpeDetuneForget();            // further down, with the MPE unison

void onSysEx(const uint8_t* data, size_t length, bool fromUSB) {
#if USE_DEBUG
  Serial.print(fromUSB ? "[USB] " : "[DIN] ");
  Serial.print("SysEx chunk: ");
  Serial.print(length);
  Serial.println(" bytes");
  //for(int i=0;i<(int)length;i++) Serial.printf("%02x ",data[i]);
  //Serial.println("");

#endif
  auto z = Z1Edit::getInstance();
  // Everything the Z1 sends is also passed to the computer. The DIN buffer
  // holds the message without F0/F7; sendSysEx() adds them.
  if (!fromUSB) usbMIDI.sendSysEx(length, data);

  if (!dumpmode) {
    if (length == 664) {
      z->parseZ1SysExPreset(data, length);
      wonkProgramLoaded();
    }
    else if (length == 298) {
      // The arpeggio pattern: the Z1 sends it whenever its mode has changed.
      Serial.println(" received arp msg");
      if (z->getMainState() == mainState::MPEPRE2) {
        // The Z1 is in Multi mode now. Give it the setup that plays A127 on
        // six channels; it answers "load completed" or an error.
        Serial.println(" Z1 in Multi mode, sending the MPE multi setup");
        txSysEx(mpe_multi_syx, mpe_multi_syx_len);
        mpeDetuneForget();
        z->setMainState(mainState::MPEPRE3);
      } else if (z->getMainState() == mainState::NORMAL) {
        // Back in Program mode (or the mode was changed on the Z1): read the
        // sound again so the displays are right.
        wonkPause(false, false);
        z->getCurrPatch();
      }
    } else if (length == 9) z->parseZ1SysExParam(data, length);
    else if (length == 4) {
      Serial.println(" received dump msg");
      if (z->getMainState() == mainState::MPEPRE) {
        if (data[3] == 0x21) {
          // The sound is in A127. Put the Z1 in Multi mode; it answers with
          // its arpeggio pattern (the 298-byte message below).
          Serial.println(" write completed, switching the Z1 to Multi mode");
          z->setZ1Mode(true);
          z->setMainState(mainState::MPEPRE2);
        }
      } else if (z->getMainState() == mainState::MPEPRE3 && (data[3] == 0x23 || data[3] == 0x24)) {
        // 23h = data load completed, 24h = data load error. MPE mode starts
        // either way, so a Z1 that refuses the setup can still be played with
        // one prepared by hand.
        Serial.println(data[3] == 0x23 ? " multi setup loaded" : " multi setup REFUSED by the Z1");
        z->sendZ1ProgParm(0, 1, 21, 0);   // voice assign mode: mono
        z->setMainState(mainState::MPE);
      }
    } else if (length == 6) {
      for (int i = 0; i < (int)length; i++) Serial.printf("%02x ", data[i]);
    }
  } else {
    //dump mode: the sysex has already been sent to USB above
    if (length == 664) {
      z->getCurrArp();
    }
    if (length == 298) {
      dumpmode = false;
    }
    
  }
}

void onSysExTooLong(bool fromUSB) {
#if USE_DEBUG
  Serial.println(fromUSB ? "[USB] SysEx too long (discarded)"
                         : "[DIN] SysEx too long (discarded)");
#endif
}

// ============ MPE unison ============
// With unison on in MPE mode, each note is played by 2, 3 or 6 of the six
// timbres, spread apart with the Detune of each timbre in the Z1's Multi
// setup. The six output channels are cut into groups of that size: 3, 2 or 1
// groups, which is the polyphony left. An input channel (one MPE note) is
// given a group when its first message arrives and keeps it until another
// note needs it.
static constexpr int MPE_FIRST_CH = 2;
static constexpr int MPE_TIMBRES = 6;
static constexpr int MPE_GROUP_NOTES = 4;

// ============ Background parameter changes ============
// Parameter changes that are not played: the wonkiness drift, the detune of
// the timbres. They must never be in the way of a note, so they wait here and
// go out one at a time, only when nothing else is waiting for the cable and no
// note has just passed (txPump() calls bgTick()). A newer value for the same parameter replaces the waiting
// one, so turning a pot does not pile up messages.
static constexpr int BG_AFTER_NOTE_MS = 3;
static std::vector<BgParam> bgQueue;

static void bgSendNow(const BgParam& p) {
  Z1Edit::getInstance()->sendZ1ProgParm(0, p.group, p.id, p.val);
}

static void bgSend(uint8_t group, int id, int val) {
  for (auto& p : bgQueue) {
    if (p.group == group && p.id == id) {
      p.val = (int16_t)val;
      return;
    }
  }
  BgParam p;
  p.group = group;
  p.id = (int16_t)id;
  p.val = (int16_t)val;
  bgQueue.push_back(p);
}

// Forget what is waiting for one group (the values are about to be replaced).
static void bgClear(uint8_t group) {
  std::vector<BgParam> keep;
  for (auto& p : bgQueue) if (p.group != group) keep.push_back(p);
  bgQueue.swap(keep);
}

// Put everything that is waiting into the sysex queue now, in order. For the
// few moments where the order matters, such as before the Z1 is told to
// change mode.
static void bgFlush() {
  std::vector<BgParam> all;
  all.swap(bgQueue);
  for (auto& p : all) bgSendNow(p);
}

// Called from txPump(). Moves one waiting change into the sysex queue. "turn"
// is the background's guaranteed turn (ten times a second); otherwise it only
// goes when the cable is idle.
static void bgTick(uint32_t now, bool turn) {
  if (bgQueue.empty()) return;
  if ((now - lastNoteEventAt) < BG_AFTER_NOTE_MS) return;
  if (!txEv.empty() || !txSx.empty() || txInSysEx) return;   // something more important waits
  if (!turn && txInFlight() > 3) return;
  BgParam p = bgQueue.front();
  bgQueue.erase(bgQueue.begin());
  bgSendNow(p);
}

// ---- Wonkiness (pot A6): see the block further down ----
static constexpr int WONK_PITCH_MAX = 50;    // Random Pitch Intensity at full pot (Z1 range 0-99)
static constexpr int WONK_DETUNE_MAX = 12;   // offset of each timbre at full pot, cents (MPE mode)
static constexpr int WONK_IDLE_MS = 4000;    // after the last note, the drift goes on this long
static constexpr int WONK_STEP_MS = 300;     // how often the drift moves one parameter
static int wonkAmount = 0;                 // 0..99 from the pot
static float wonkVoice[MPE_TIMBRES];       // fixed offset of each timbre, -1..1

static uint8_t mpeVoices = 1;                   // timbres per note: 1 (off), 2, 3 or 6
static int8_t mpeGroupOf[17];                   // input channel 1..16 -> group, -1 = none
static uint8_t mpeOwner[MPE_TIMBRES];           // group -> input channel, 0 = none
static uint8_t mpeHeld[MPE_TIMBRES][MPE_GROUP_NOTES];
static uint8_t mpeHeldCount[MPE_TIMBRES];
static uint32_t mpeStamp[MPE_TIMBRES];          // order of use, for stealing
static uint32_t mpeClock = 0;
static int mpeDetuneCents = 0;                  // spread: outer timbres at -/+ this
static bool mpeTablesReady = false;

static void mpeResetTables() {
  for (int i = 0; i < 17; ++i) mpeGroupOf[i] = -1;
  for (int g = 0; g < MPE_TIMBRES; ++g) {
    mpeOwner[g] = 0;
    mpeHeldCount[g] = 0;
    mpeStamp[g] = 0;
  }
  mpeTablesReady = true;
}

static inline uint8_t mpeOutCh(int group, int k) {
  return (uint8_t)(MPE_FIRST_CH + group * mpeVoices + k);
}

// Notes must never be lost, so they wait for room instead of being dropped.
static void mpeNoteToGroup(int group, uint8_t status, uint8_t note, uint8_t vel) {
  for (int k = 0; k < mpeVoices; ++k) txFlushExpression(mpeOutCh(group, k));   // start and end values first
  for (int k = 0; k < mpeVoices; ++k) {
    txEvent3((uint8_t)(status | (mpeOutCh(group, k) - 1)), note, vel);
  }
}

static void mpeReleaseGroup(int group) {
  for (int i = 0; i < mpeHeldCount[group]; ++i) mpeNoteToGroup(group, 0x80, mpeHeld[group][i], 0);
  mpeHeldCount[group] = 0;
}

// The group for an input channel; gives it one if it has none.
static int mpeGroupFor(uint8_t inCh) {
  if (!mpeTablesReady) mpeResetTables();
  if (inCh < 1 || inCh > 16) return -1;
  if (mpeGroupOf[inCh] >= 0) return mpeGroupOf[inCh];
  int groups = MPE_TIMBRES / mpeVoices;
  int best = -1;
  // First choice: a group with no notes, the one unused longest. Otherwise
  // the group whose note is oldest, which is then cut off.
  for (int pass = 0; pass < 2 && best < 0; ++pass) {
    for (int g = 0; g < groups; ++g) {
      if (pass == 0 && mpeHeldCount[g] > 0) continue;
      if (best < 0 || mpeStamp[g] < mpeStamp[best]) best = g;
    }
  }
  if (best < 0) return -1;
  mpeReleaseGroup(best);
  if (mpeOwner[best]) mpeGroupOf[mpeOwner[best]] = -1;
  mpeOwner[best] = inCh;
  mpeGroupOf[inCh] = (int8_t)best;
  mpeStamp[best] = ++mpeClock;
  return best;
}

static void mpeUnisonNoteOn(uint8_t inCh, uint8_t note, uint8_t vel) {
  int g = mpeGroupFor(inCh);
  if (g < 0) return;
  if (mpeHeldCount[g] < MPE_GROUP_NOTES) mpeHeld[g][mpeHeldCount[g]++] = note;
  mpeStamp[g] = ++mpeClock;
  mpeNoteToGroup(g, 0x90, note, vel);
}

static void mpeUnisonNoteOff(uint8_t inCh, uint8_t note, uint8_t vel) {
  if (!mpeTablesReady || inCh < 1 || inCh > 16) return;
  int g = mpeGroupOf[inCh];
  if (g < 0) return;                             // its group was taken by a newer note
  for (int i = 0; i < mpeHeldCount[g]; ++i) {
    if (mpeHeld[g][i] != note) continue;
    mpeHeld[g][i] = mpeHeld[g][--mpeHeldCount[g]];
    mpeNoteToGroup(g, 0x80, note, vel);
    break;
  }
}

// Detune of each timbre: Multi parameter 21, 27 apart per timbre, -50..+50
// cents. Within a group the timbres are spread evenly from -cents to +cents.
// What each timbre's detune was last set to, so that only the ones that
// change are sent. "Not known" after the Multi setup has been loaded afresh.
static constexpr int MPE_DETUNE_UNKNOWN = -1000;
static int mpeDetuneSent[MPE_TIMBRES] = { MPE_DETUNE_UNKNOWN, MPE_DETUNE_UNKNOWN, MPE_DETUNE_UNKNOWN,
                                          MPE_DETUNE_UNKNOWN, MPE_DETUNE_UNKNOWN, MPE_DETUNE_UNKNOWN };

// The Multi setup was just loaded into the Z1: its timbres have the detune
// the setup gives them, which is 0.
void mpeDetuneForget() {
  for (int t = 0; t < MPE_TIMBRES; ++t) mpeDetuneSent[t] = 0;
}

static void mpeSendDetune() {
  for (int t = 0; t < MPE_TIMBRES; ++t) {
    int cents = 0;
    if (mpeVoices > 1) {
      int k = t % mpeVoices;
      cents = (int)lroundf(mpeDetuneCents * (2.0f * k / (mpeVoices - 1) - 1.0f));
    }
    // Each timbre is also a little off by its own amount, like aged voice cards.
    cents += (int)lroundf(wonkVoice[t] * WONK_DETUNE_MAX * wonkAmount / 99.0f);
    if (cents > 50) cents = 50;
    if (cents < -50) cents = -50;
    if (cents == mpeDetuneSent[t]) continue;       // nothing to tell the Z1
    mpeDetuneSent[t] = cents;
    bgSend(3, 21 + 27 * t, cents);                 // group 3 = Multi
  }
}

// The unison button in MPE mode (called from Z1Edit), and on leaving MPE.
void mpeUnisonChanged(int voices) {
  if (!mpeTablesReady) mpeResetTables();
  // Nothing may keep sounding on channels that are about to change hands.
  for (int g = 0; g < MPE_TIMBRES; ++g) mpeReleaseGroup(g);
  for (uint8_t ch = MPE_FIRST_CH; ch < MPE_FIRST_CH + MPE_TIMBRES; ++ch) {
    txEvent3((uint8_t)(0xB0 | (ch - 1)), 123, 0);   // All Notes Off
  }
  mpeVoices = (voices == 2 || voices == 3 || voices == 6) ? voices : 1;
  mpeResetTables();
  mpeSendDetune();
  bgFlush();                                   // nothing is sounding; and a mode change may follow
}

// The detune pot in MPE mode: 0..99 becomes a spread of 0..50 cents.
static void mpeDetunePot(int pot) {
  int cents = (pot * 50 + 49) / 99;
  if (cents == mpeDetuneCents) return;
  mpeDetuneCents = cents;
  if (mpeVoices > 1) mpeSendDetune();
}

// ============ Wonkiness ============
// Pot A6 makes the current sound behave like an old analog synth: nothing is
// quite in tune or quite where it was set, and it keeps moving a little.
// Everything is done with Korg parameter changes (sysex function 41h), the
// same numbered parameters the editor uses; the Z1's NRPNs only reach the
// arpeggiator.
//   Per note:   program parameter 30, Random Pitch Intensity - the Z1 then
//               scatters the pitch of every note by itself.
//   Per voice:  in MPE mode each timbre has a fixed offset of its own (Multi
//               Detune, added to the unison spread in mpeSendDetune()).
//   Drift:      a list of program parameters wanders slowly around the values
//               of the patch. Always the fine tune of the oscillators and the
//               two filter cutoffs; the rest depends on the oscillator type
//               (WONK_BY_TYPE below), so a brass model drifts in its lip and
//               breath noise, a plucked string in where it is plucked, etc.
// The patch's own values are taken from the editor (kept up to date by the
// dumps and by edits), so the drift follows when a parameter is edited.


// For every patch.
static constexpr WonkSpec WONK_COMMON[] = {
  { 177, -50, 50, 6 },   // OSC1 fine tune, cents
  { 191, -50, 50, 6 },   // OSC2 fine tune
  { 204, -50, 50, 4 },   // sub oscillator fine tune
  { 263, 0, 99, 8 },     // filter 1 cutoff
  { 290, 0, 99, 8 },     // filter 2 cutoff
};

// Per oscillator type (parameter 174 for OSC1, 188 for OSC2), in the order of
// the Z1's type list. Up to four parameters each; id 0 = unused.
static constexpr int WONK_PER_TYPE = 4;
static constexpr WonkSpec WONK_BY_TYPE[13][WONK_PER_TYPE] = {
  /* Standard */ { { 465, 0, 99, 5 }, { 470, -99, 99, 4 } },                    // wave edge, wave form
  /* Comb     */ { { 492, 0, 99, 3 }, { 497, 0, 99, 5 } },                      // feedback, high damp
  /* VPM      */ { { 520, 0, 99, 4 }, { 514, -50, 50, 3 } },                    // modulator level, modulator fine
  /* Reso     */ { { 535, -99, 99, 4 }, { 541, -99, 99, 4 }, { 547, -99, 99, 4 }, { 553, -99, 99, 4 } },  // the four band frequencies
  /* Ring     */ { { 559, 0, 99, 4 }, { 565, 0, 99, 4 } },                      // modulation depth, wave edge
  /* Cross    */ { { 568, 0, 99, 3 }, { 573, 0, 99, 4 } },                      // modulation depth, wave edge
  /* Sync     */ { { 576, 0, 99, 5 } },                                         // wave edge
  /* Organ    */ { { 579, -99, 99, 3 }, { 586, -99, 99, 3 }, { 593, -99, 99, 3 } },   // fine tune of the three drawbars
  /* E.Piano  */ { { 602, 0, 99, 5 }, { 608, 0, 99, 4 }, { 611, 0, 99, 3 } },   // hammer force, overtone level, pickup position
  /* Brass    */ { { 626, 0, 99, 5 }, { 634, 0, 99, 4 }, { 648, 0, 99, 4 } },   // lip character, noise level, strength
  /* Reed     */ { { 668, 0, 99, 5 }, { 691, 0, 99, 4 } },                      // noise level, wave shape
  /* Plucked  */ { { 702, 0, 99, 5 }, { 708, 0, 99, 4 }, { 700, 0, 99, 5 }, { 719, 0, 99, 4 } },  // string position, damping, attack noise, pickup position
  /* Bowed    */ { { 736, 0, 99, 5 }, { 737, 0, 99, 4 }, { 740, 0, 99, 3 } },   // rosin, string position, damping
};

static constexpr int WONK_MAX_TARGETS = (5 + 2 * WONK_PER_TYPE);
static constexpr int WONK_PER_STEP = 1;      // parameters moved per step: the Z1's processor is slow, every message costs it time

static constexpr int WONK_UNSENT = -10000;

static WonkTarget wonkTargets[WONK_MAX_TARGETS];
static int wonkCount = 0;
static int wonkNext = 0;                       // round robin
static int wonkBuiltFor[2] = { -1, -1 };       // oscillator types the list was built for
static int wonkSentPitch = -1;
static uint32_t wonkNextAt = 0;
static bool wonkSeeded = false;
// While this is set nothing is sent. The drift needs the patch's own values,
// and after a program change the editor does not have them until a dump has
// arrived; and a dump must not contain drifted values, or they would be
// taken for the patch's own.
static bool wonkHold = false;
static uint32_t wonkAskAt = 0;                 // when to ask the Z1 for the dump ourselves

static void wonkSeed() {
  if (wonkSeeded) return;
  wonkSeeded = true;
  randomSeed(micros());
  for (int t = 0; t < MPE_TIMBRES; ++t) wonkVoice[t] = (float)random(-1000, 1001) / 1000.0f;
}

static int wonkBase(int key) {
  return editables.count(key) ? editables[key]->getIval() : 0;
}

static int wonkOscType(int slot) {
  int t = wonkBase(slot == 0 ? 174 : 188);
  return (t >= 0 && t < 13) ? t : -1;
}

static void wonkAdd(const WonkSpec& spec, int exid) {
  int key = spec.id | exid;
  if (spec.id == 0 || wonkCount >= WONK_MAX_TARGETS || !editables.count(key)) return;
  WonkTarget& t = wonkTargets[wonkCount++];
  t.key = key;
  t.lo = spec.lo;
  t.hi = spec.hi;
  t.depth = spec.depth;
  t.walk = 0.0f;
  t.sent = WONK_UNSENT;
}

// The list of drifting parameters for the patch as the editor knows it.
static void wonkBuild() {
  wonkCount = 0;
  wonkNext = 0;
  for (unsigned i = 0; i < sizeof(WONK_COMMON) / sizeof(WONK_COMMON[0]); ++i) wonkAdd(WONK_COMMON[i], 0);
  for (int slot = 0; slot < 2; ++slot) {
    int type = wonkOscType(slot);
    wonkBuiltFor[slot] = type;
    if (type < 0) continue;
    for (int i = 0; i < WONK_PER_TYPE; ++i) wonkAdd(WONK_BY_TYPE[type][i], slot == 0 ? 0x800 : 0x1000);
  }
}

static void wonkSendPitch() {
  int v = wonkBase(30) + wonkAmount * WONK_PITCH_MAX / 99;
  if (v > 99) v = 99;
  if (v == wonkSentPitch) return;
  wonkSentPitch = v;
  bgSend(1, 30, v);
}

// Moves one parameter; with the pot at 0 it goes back to the patch value.
static void wonkMove(WonkTarget& t) {
  int base = wonkBase(t.key);
  int target = base;
  if (wonkAmount > 0) {
    t.walk = t.walk * 0.9f + (float)random(-300, 301) / 1000.0f;
    if (t.walk > 1.0f) t.walk = 1.0f;
    if (t.walk < -1.0f) t.walk = -1.0f;
    target = base + (int)lroundf(t.walk * t.depth * wonkAmount / 99.0f);
    if (target > t.hi) target = t.hi;
    if (target < t.lo) target = t.lo;
  } else {
    t.walk = 0.0f;
    if (t.sent == WONK_UNSENT) return;           // nothing was changed
  }
  if (target == t.sent || (t.sent == WONK_UNSENT && target == base)) return;
  bgSend(1, t.key, target);
  t.sent = (target == base) ? WONK_UNSENT : target;
}

// Puts every drifted parameter back to the patch value.
static void wonkRestore() {
  bgClear(1);                                    // drift values still waiting are obsolete
  wonkSentPitch = -1;
  for (int i = 0; i < wonkCount; ++i) {
    WonkTarget& t = wonkTargets[i];
    if (t.sent == WONK_UNSENT) continue;
    Z1Edit::getInstance()->sendZ1ProgParm(0, 1, t.key, wonkBase(t.key));
    t.sent = WONK_UNSENT;
    t.walk = 0.0f;
  }
}

// The pot moved.
static void wonkPot(int pot) {
  wonkSeed();
  if (pot == wonkAmount) return;
  wonkAmount = pot;
  if (Z1Edit::getInstance()->inMPE()) mpeSendDetune();
  if (wonkHold) return;                          // the rest follows when the dump is in
  if (wonkAmount == 0) wonkRestore();            // first: it also drops what is waiting
  wonkSendPitch();
}

// Stop sending until the next program dump has arrived.
//   restore:  put the patch's values back first (a dump was asked for, and
//             it must show the patch as it is)
//   askLater: the program changed; ask for the dump ourselves if nobody else
//             does within 400 ms
static void wonkPause(bool restore, bool askLater) {
  if (restore && !wonkHold) wonkRestore();
  if (wonkAmount == 0 && !wonkHold) return;      // nothing to hold back
  wonkHold = true;
  wonkAskAt = askLater ? millis() + 400 : 0;
}

// Called from loop().
static void wonkTick(uint32_t now) {
  if (wonkHold) {
    if (wonkAskAt && now >= wonkAskAt) {
      wonkAskAt = 0;
      Z1Edit::getInstance()->getCurrPatch();
    }
    return;
  }
  if (wonkAmount == 0 || now < wonkNextAt) return;
  wonkNextAt = now + WONK_STEP_MS;
  // Nothing drifts while nothing sounds: with no note on and none for a few
  // seconds (the releases have died away) the Z1 is left in peace.
  if (notesSounding == 0 && now - lastNoteEventAt > WONK_IDLE_MS) return;
  // An oscillator type was changed in the editor: other parameters matter now.
  if (wonkOscType(0) != wonkBuiltFor[0] || wonkOscType(1) != wonkBuiltFor[1]) wonkBuild();
  for (int n = 0; n < WONK_PER_STEP && wonkCount > 0; ++n) {
    wonkMove(wonkTargets[wonkNext]);
    wonkNext = (wonkNext + 1) % wonkCount;
  }
}

// A program dump arrived: the editor has the patch's own values again, so
// the wonkiness is applied afresh.
static void wonkProgramLoaded() {
  wonkHold = false;
  wonkAskAt = 0;
  wonkSentPitch = -1;
  wonkBuild();
  if (wonkAmount > 0) wonkSendPitch();
}

// ============ Pots ============
// Pot 0 (A6) wonkiness, pot 1 (A7) portamento time, pot 2 (A8) detune. What
// counts is potValue: it follows the pot, but can also be set from the
// computer, so that a Live set can bring back how the pots stood.
static constexpr int POT_TAKEOVER = 3;                       // steps a pot must move to take over again
static int potValue[3] = { 0, 0, 0 };        // value in effect, 0-99 (a pot at 0 at power-up sends nothing)
static bool potRemote[3] = { false, false, false };
static int potAnchor[3] = { 0, 0, 0 };       // where the pot stood when the value was set

// A pot that moves is shown for a moment in the last slot of the last display,
// which the pages rarely use. Only that slot is written, so nothing else is
// redrawn; afterwards the slot gets back what the page had there.
static constexpr int POT_SLOT = 17;
static constexpr int POT_SHOW_MS = 1500;
static uint32_t potShownUntil = 0;

static void potShow(int i, int v) {
  static const char* const names[3] = { "Wonk", "Porta", "Detune" };
  auto s = SynthBuddy::getInstance();
  s->label(POT_SLOT, names[i]);
  s->value(POT_SLOT, v);
  potShownUntil = millis() + POT_SHOW_MS;
  if (potShownUntil == 0) potShownUntil = 1;
}

// Called from loop().
static void potShowTick(uint32_t now) {
  if (potShownUntil == 0 || (int32_t)(now - potShownUntil) < 0) return;
  potShownUntil = 0;
  auto s = SynthBuddy::getInstance();
  if (s->currentEditable[POT_SLOT]) {
    s->currentEditable[POT_SLOT]->show(POT_SLOT);
  } else {
    s->label(POT_SLOT, "");
    s->value(POT_SLOT, "");
  }
}

static void potApply(int i, int v) {
  if (v < 0) v = 0;
  if (v > 99) v = 99;
  potValue[i] = v;
  potShow(i, v);
  auto z = Z1Edit::getInstance();
  switch (i) {
    case 0: wonkPot(v); break;                         // wonkiness
    case 1: sendcc(0, 5, v); break;                    // portamento time
    case 2:
      if (z->inMPE()) mpeDetunePot(v);                 // spread of the stacked timbres
      else z->sendZ1ProgParm(0, 1, 27, v);             // unison detune of the program, 0-99 cents
      break;
  }
}

// A value from the computer. The pot itself is left where it is.
static void potSet(int i, int v) {
  if (i < 0 || i > 2 || v > 99) return;
  potRemote[i] = true;
  potAnchor[i] = (i == 0) ? pot0 : (i == 1) ? pot1 : pot2;
  potApply(i, v);
}

// ============ Buddy sysex ============
// Short messages between the computer and the Buddy itself. They are never
// passed on to the Z1. Manufacturer ID 7Dh is the one MIDI sets aside for
// private use; "Z1" (5Ah 31h) follows so nothing else answers to them.
//
//   F0 7D 5A 31 <command> <data...> F7
//
//   to the Buddy:
//     01                  send the status
//     02 mm               go to main mode mm: 0 Normal, 1 MPE
//     03 uu               set unison uu: 0 off, 1 = 2 voices, 2 = 3, 3 = 6
//     04 ww pt dt         set the three pots, 0-99 each; 7F = leave as it is.
//                         The value holds until the pot itself is moved.
//     05                  send the debug status (answer: 12 ..., see buddySendDebug)
//     06                  panic: note-off for every note the Buddy has on for
//                         the Z1, then All Notes Off on every channel
//     07 pp               depth of a key's own bend in MPE mode, percent of true
//                         pitch, 0-127 (100 = a semitone of bend is a semitone)
//     08 ss cc            what the controller means by a full bend of a key:
//                         ss semitones (1-96) and cc cents (0-99). The same as
//                         RPN 0 on a note channel, for a controller that does
//                         not set it itself. Kept when switched off; 48 00
//                         on a new Buddy.
//   from the Buddy:
//     11 mm uu ww bb pp pt dt   status, sent when asked and whenever it changes
//        mm  0 Normal, 1 entering MPE (waiting for the Z1), 2 MPE
//        uu  unison shown on the LEDs (the program's in Normal mode, the
//            number of stacked timbres in MPE mode)
//        ww  wonkiness, 0-99 (the value in effect: the pot, or what was set)
//        bb pp  bank (0 = A, 1 = B, 7F = not known) and program (0-127) last
//            selected in Normal mode: in MPE mode, the sound that was copied
//            to the working slot A127
//        pt dt  portamento time and detune, 0-99
//     12 ...             debug status: queues, longest waits, counters and the
//                         notes that are on; laid out at buddySendDebug()
static constexpr int BUDDY_ID_1 = 0x7D;
static constexpr int BUDDY_ID_2 = 0x5A;
static constexpr int BUDDY_ID_3 = 0x31;

static uint8_t buddyBank = 0x7F;     // not known until a bank select has passed
static uint8_t buddyProg = 0;
static uint8_t buddySent[7] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
static uint32_t buddyPotAt = 0;

static uint8_t buddyPot(int i) {
  return (uint8_t)(potValue[i] < 0 ? 0 : potValue[i]);
}

// Bank select and program change seen in Normal mode, from either side.
static void buddyNoteBank(uint8_t lsb) {
  if (Z1Edit::getInstance()->getMainState() == mainState::NORMAL) buddyBank = lsb & 0x7F;
}
static void buddyNoteProgram(uint8_t prog) {
  if (Z1Edit::getInstance()->getMainState() == mainState::NORMAL) buddyProg = prog & 0x7F;
}

static uint8_t buddyModeCode() {
  switch (Z1Edit::getInstance()->getMainState()) {
    case mainState::NORMAL: return 0;
    case mainState::MPE:
    case mainState::MPEPLUS: return 2;
    default: return 1;
  }
}

static void buddySendStatus() {
  uint8_t m[11] = { BUDDY_ID_1, BUDDY_ID_2, BUDDY_ID_3, 0x11, buddyModeCode(),
                    (uint8_t)Z1Edit::getInstance()->unisonShown(), buddyPot(0), buddyBank, buddyProg,
                    buddyPot(1), buddyPot(2) };
  for (int i = 0; i < 7; ++i) buddySent[i] = m[4 + i];
  usbMIDI.sendSysEx(sizeof m, m);    // adds F0 and F7
}

// Called from loop(): tells the computer when something changed. The pots are
// held back a little so that turning one does not send a stream of messages.
static void buddyTick(uint32_t now) {
  uint8_t mode = buddyModeCode();
  uint8_t unison = (uint8_t)Z1Edit::getInstance()->unisonShown();
  bool changed = mode != buddySent[0] || unison != buddySent[1] || buddyBank != buddySent[3] || buddyProg != buddySent[4];
  if (!changed && (buddyPot(0) != buddySent[2] || buddyPot(1) != buddySent[5] || buddyPot(2) != buddySent[6])) {
    if (buddyPotAt == 0) buddyPotAt = now + 300;
    else if (now >= buddyPotAt) changed = true;
  }
  if (!changed) return;
  buddyPotAt = 0;
  buddySendStatus();
}

// The debug status: everything needed to see where a note or a message went.
// Layout after F0 7D 5A 31 12 (all bytes 0-127; a number of several bytes has
// its low 7 bits first):
//   1  format version (1)
//   7  mode, unison, wonkiness, portamento, detune, bank, program
//   3  seconds since power-up
//   2  events waiting   2  sysex bytes waiting   1  background changes waiting
//   1  1 if a sysex message is on its way out
//   2  most events waiting at once   2  longest wait of an event, ms
//   2  longest pass of the main loop, ms        (these three since the last report)
//   3  note-ons from the computer    3  note-offs from the computer
//   3  note-ons sent to the Z1       3  note-offs sent to the Z1
//   3  expression messages sent      3  sysex messages sent
//   2  sysex messages closed because their end never came
//   1  ni   1  no   then ni and no pairs <channel 0-15, note>: the notes that
//      are on as received from the computer, and as sent to the Z1 (24 at most
//      of each)
static uint8_t* put7(uint8_t* p, uint32_t v, int bytes) {
  for (int i = 0; i < bytes; ++i) { *p++ = v & 0x7F; v >>= 7; }
  return p;
}

static void buddySendDebug() {
  uint8_t m[64 + 4 * 24];
  uint8_t* p = m;
  *p++ = BUDDY_ID_1; *p++ = BUDDY_ID_2; *p++ = BUDDY_ID_3; *p++ = 0x12;
  *p++ = 1;
  *p++ = buddyModeCode();
  *p++ = (uint8_t)Z1Edit::getInstance()->unisonShown();
  *p++ = buddyPot(0); *p++ = buddyPot(1); *p++ = buddyPot(2);
  *p++ = buddyBank; *p++ = buddyProg;
  p = put7(p, millis() / 1000, 3);
  p = put7(p, txEv.size(), 2);
  p = put7(p, txSx.size(), 2);
  *p++ = (uint8_t)(bgQueue.size() > 127 ? 127 : bgQueue.size());
  *p++ = txInSysEx ? 1 : 0;
  p = put7(p, dbgStats.maxEvents, 2);
  p = put7(p, dbgStats.maxEventWaitMs, 2);
  p = put7(p, dbgStats.maxLoopMs, 2);
  p = put7(p, dbgStats.noteOnIn, 3);
  p = put7(p, dbgStats.noteOffIn, 3);
  p = put7(p, dbgStats.noteOnOut, 3);
  p = put7(p, dbgStats.noteOffOut, 3);
  p = put7(p, dbgStats.expressionOut, 3);
  p = put7(p, dbgStats.sysexOut, 3);
  p = put7(p, dbgStats.sysexStalls, 2);
  uint8_t* counts = p;
  p += 2;
  Serial.print("notes on, from the computer:");
  for (int t = 0; t < 2; ++t) {
    uint8_t (*table)[128] = t == 0 ? noteIn : noteOut;
    uint8_t count = 0;
    for (int c = 0; c < 16; ++c) {
      for (int k = 0; k < 128; ++k) {
        if (!table[c][k]) continue;
        Serial.printf(" ch%d:%d", c + 1, k);
        if (count < 24) { *p++ = (uint8_t)c; *p++ = (uint8_t)k; count++; }
      }
    }
    counts[t] = count;
    if (t == 0) Serial.print(" | sent to the Z1:");
  }
  Serial.println("");
  usbMIDI.sendSysEx(p - m, m);
  dbgStats.maxEvents = 0;
  dbgStats.maxEventWaitMs = 0;
  dbgStats.maxLoopMs = 0;
}

// Silence: a note-off for everything we have on, then All Notes Off.
static void buddyPanic() {
  for (int c = 0; c < 16; ++c) {
    for (int k = 0; k < 128; ++k) {
      while (noteOut[c][k] > 0) { noteOut[c][k]--; txEvent3((uint8_t)(0x80 | c), (uint8_t)k, 0); }
      noteIn[c][k] = 0;
    }
  }
  // (the note-offs above are counted again when they are written; that is
  // harmless, the count does not go below zero)
  for (int c = 0; c < 16; ++c) txEvent3((uint8_t)(0xB0 | c), 123, 0);
  notesSounding = 0;
  Serial.println("panic: all notes off");
}

// A complete sysex message from the computer; true if it was for the Buddy.
static bool buddyHandle(const uint8_t* data, uint16_t length) {
  if (length < 6 || data[0] != 0xF0 || data[1] != BUDDY_ID_1 || data[2] != BUDDY_ID_2 || data[3] != BUDDY_ID_3) return false;
  auto z = Z1Edit::getInstance();
  switch (data[4]) {
    case 0x01: buddySendStatus(); break;
    case 0x02: if (length >= 7) z->gotoMainMode(data[5]); break;
    case 0x03: if (length >= 7) z->setUnisonLevel(data[5]); break;
    case 0x04:
      for (int i = 0; i < 3 && 6 + i < length; ++i) potSet(i, data[5 + i]);
      break;
    case 0x05: buddySendDebug(); break;
    case 0x06: buddyPanic(); break;
    case 0x08:
      if (length >= 7 && data[5] >= 1 && data[5] <= 96) {
        mpeBendSemitones = data[5];
        mpeBendCents = (length >= 8 && data[6] <= 99) ? data[6] : 0;
        bendSettingsSave();
        Serial.printf("MPE: a full key bend is %d.%02d semitones (set by Buddy message)\r\n", mpeBendSemitones, mpeBendCents);
      }
      break;
    case 0x07:
      if (length >= 7) {
        mpeBendPercent = data[5];
        bendSettingsSave();
        Serial.printf("MPE bend depth %d %%\r\n", mpeBendPercent);
      }
      break;
  }
  return true;
}

// --- MPE hooks (USB side) ---
void onMPE_NoteOn(uint8_t ch, uint8_t note, uint8_t vel) {
  Serial.printf("noteon %d %d %d\r\n", ch, note, vel);
}
void onMPE_NoteOff(uint8_t ch, uint8_t note, uint8_t vel) {
  Serial.printf("noteoff %d %d %d\r\n", ch, note, vel);
}


void onMPE_PolyPressure(uint8_t ch, uint8_t note, uint8_t press) { /* stub */
}
void onMPE_ChannelPressure(uint8_t ch, uint8_t press) { /* stub */
}
void onMPE_PitchBend(uint8_t ch, int value14) {
  //Serial.printf("pb %d %d\r\n", ch, value14);
}
void onMPE_CC(uint8_t ch, uint8_t cc, uint8_t val) { /* stub */
}

// ---------- Forwarding: DIN -> USB ----------
static void forwardDIN_ChannelMsg_ToUSB(uint8_t status, uint8_t d1, uint8_t d2) {
  uint8_t type = status & 0xF0;
  uint8_t ch = (status & 0x0F) + 1;  // usbMIDI uses 1..16
  switch (type) {
    case 0x80: usbMIDI.sendNoteOff(d1, d2, ch); break;
    case 0x90: usbMIDI.sendNoteOn(d1, d2, ch); break;
    case 0xA0: usbMIDI.sendPolyPressure(d1, d2, ch); break;
    case 0xB0:
      usbMIDI.sendControlChange(d1, d2, ch);
      if (d1 == 32) buddyNoteBank(d2);
      break;
    case 0xC0:
      usbMIDI.sendProgramChange(d1, ch);
      // remember for anti-loop
      lastDIN_ProgCh = d1;
      lastDIN_ProgChan = ch;
      lastDIN_ProgTime = millis();
      buddyNoteProgram(d1);
      wonkPause(false, true);
      break;
    case 0xD0: usbMIDI.sendAfterTouch(d1, ch); break;
    case 0xE0:
      {
        int value14 = ((int)d2 << 7) | d1;          // 0..16383
        usbMIDI.sendPitchBend(value14 - 8192, ch);  // Teensy expects -8192..+8191
      }
      break;
  }
}

static void forwardDIN_SystemCommon_ToUSB(uint8_t status, uint8_t d1, uint8_t d2) {
  switch (status) {
    case 0xF1:
      {  // MTC Quarter Frame: split nibbles
        uint8_t typeNib = (d1 >> 4) & 0x07;
        uint8_t valNib = d1 & 0x0F;
        usbMIDI.sendTimeCodeQuarterFrame(typeNib, valNib);
      }
      break;
    case 0xF2: usbMIDI.sendSongPosition(((int)d2 << 7) | d1); break;
    case 0xF3: usbMIDI.sendSongSelect(d1); break;
    case 0xF6: usbMIDI.sendTuneRequest(); break;
    default: break;
  }
}

static void forwardDIN_RealTime_ToUSB(uint8_t rt) {
  switch (rt) {
    //case 0xF8: usbMIDI.sendRealTime(usbMIDI.Clock); break;
    case 0xFA: usbMIDI.sendRealTime(usbMIDI.Start); break;
    case 0xFB: usbMIDI.sendRealTime(usbMIDI.Continue); break;
    case 0xFC: usbMIDI.sendRealTime(usbMIDI.Stop); break;
    case 0xFE: usbMIDI.sendRealTime(usbMIDI.ActiveSensing); break;
    case 0xFF: usbMIDI.sendRealTime(usbMIDI.SystemReset); break;
    default: break;
  }
}

// Short messages for the Z1 go through the output queue (txEvent), where
// they are never dropped and notes keep their order. The expression scheduler
// writes directly (txDirect): it has checked that there is room, and what it
// sends is the newest value at this very moment.
static inline void uartTxWrite(const uint8_t* data, size_t n) {
  if (n == 1 && data[0] >= 0xF8) txRealtime(data[0]);
  else txEvent(data, n);
}
static inline void txDirect(const uint8_t* data, size_t n) {
  Serial1.write(data, n);
  dbgStats.expressionOut++;
}

// Tiny helpers to send single MIDI messages (raw bytes) via uartTxWrite()
static inline void sendCC_toDIN(uint8_t ch, uint8_t cc, uint8_t val) {
  uint8_t m[3] = { (uint8_t)(0xB0 | ((ch - 1) & 0x0F)), cc, val };
  txDirect(m, 3);
}
static inline void sendPoly_toDIN(uint8_t ch, uint8_t note, uint8_t val) {
  uint8_t m[3] = { (uint8_t)(0xA0 | ((ch - 1) & 0x0F)), note, val };
  txDirect(m, 3);
}
static inline void sendChPress_toDIN(uint8_t ch, uint8_t val) {
  uint8_t m[2] = { (uint8_t)(0xD0 | ((ch - 1) & 0x0F)), val };
  txDirect(m, 2);
}
static inline void sendPB_toDIN(uint8_t ch, uint16_t v14) {
  uint8_t lsb = v14 & 0x7F, msb = (v14 >> 7) & 0x7F;
  uint8_t m[3] = { (uint8_t)(0xE0 | ((ch - 1) & 0x0F)), lsb, msb };
  txDirect(m, 3);
}
static inline void forwardUSB_ChannelMsg_ToDIN(uint8_t status, uint8_t d1, uint8_t d2, uint8_t needed) {
  if (needed == 1) {
    uint8_t m[2] = { status, d1 };
    uartTxWrite(m, 2);
  } else {
    uint8_t m[3] = { status, d1, d2 };
    uartTxWrite(m, 3);
  }
}
static inline void forwardUSB_SystemCommon_ToDIN(uint8_t status, uint8_t d1, uint8_t d2) {
  if (status == 0xF2) {
    uint8_t m[3] = { status, d1, d2 };
    uartTxWrite(m, 3);
  } else if (status == 0xF1 || status == 0xF3) {
    uint8_t m[2] = { status, d1 };
    uartTxWrite(m, 2);
  } else {
    uint8_t m[1] = { status };
    uartTxWrite(m, 1);
  }
}
static inline void forwardUSB_RealTime_ToDIN(uint8_t rt) {
  uint8_t b = rt;
  uartTxWrite(&b, 1);
}





// ---------- DIN parser (non-blocking; SysEx single-chunk) ----------
void processDIN() {
  while (Serial1.available()) {
    uint8_t b = Serial1.read();

    // Real-time can appear anywhere; forward immediately
    if (b >= 0xF8) {
      forwardDIN_RealTime_ToUSB(b);
      continue;
    }

    if (dinInSysEx) {
      if (b == 0xF7) {
        if (dinSysExOverflow) onSysExTooLong(false);
        else onSysEx(sysexBuf, sysexLen, /*fromUSB=*/false);
        dinInSysEx = false;
        dinSysExOverflow = false;
        sysexLen = 0;
      } else if (b == 0xF0) {
        dinSysExOverflow = false;
        sysexLen = 0;  // restart
      } else {
        if (!dinSysExOverflow) {
          if (sysexLen < SYSEX_MAX) sysexBuf[sysexLen++] = b;
          else dinSysExOverflow = true;  // drain until F7
        }
      }
      continue;
    }

    if (b & 0x80) {  // status
      if (b == 0xF0) {
        dinInSysEx = true;
        dinSysExOverflow = false;
        sysexLen = 0;
        continue;
      }
      dinRunningStatus = b;
      dinNeeded = dataBytesNeeded(b);
      dinData1 = 0;
      if (dinNeeded == 0 && b >= 0xF0) forwardDIN_SystemCommon_ToUSB(b, 0, 0);
      continue;
    }

    // data
    if (dinRunningStatus == 0) continue;
    if (dinNeeded == 2) {
      dinData1 = b;
      dinNeeded = 1;
    } else if (dinNeeded == 1) {
      uint8_t s = dinRunningStatus;
      uint8_t needed = dataBytesNeeded(s);  // how many data bytes this status really uses

      if (s >= 0xF0) {
        // System Common / Sys Real-Time that use data bytes
        if (needed == 2) {
          forwardDIN_SystemCommon_ToUSB(s, dinData1, b);
        } else if (needed == 1) {
          forwardDIN_SystemCommon_ToUSB(s, b, 0);
        } else {
          forwardDIN_SystemCommon_ToUSB(s, 0, 0);
        }
      } else {
        // Channel Voice
        if (needed == 2) {
          // e.g., NoteOn/Off, CC, PolyPressure, PitchBend
          forwardDIN_ChannelMsg_ToUSB(s, dinData1, b);
        } else {
          // e.g., Program Change (0xC0), Channel Pressure (0xD0) -> 1 data byte
          forwardDIN_ChannelMsg_ToUSB(s, b, 0);
        }
      }

      dinNeeded = dataBytesNeeded(s);  // reset for running-status continuation
    }
  }
}

// ---------- USB readers ----------
// Korg Z1 sysex from the computer (F0 42 3g 46 ...) is passed on to the Z1, so
// a host can request and send program dumps. usbMIDI delivers long messages in
// several chunks; the first one starts with F0 and decides for the whole message.
static bool usbSysExToDIN = false;
// The same message is also collected here, so that the Buddy can read what the
// computer sends to the Z1 (a parameter change, a program dump) and keep its
// own displays and values in step.
static uint8_t usbSxBuf[SYSEX_MAX + 2];
static size_t usbSxLen = 0;
static bool usbSxTooLong = false;

void handleUSB_SysEx(const uint8_t* data, uint16_t length, bool complete) {
  if (length > 0 && data[0] == 0xF0) {
    usbSysExToDIN = (length >= 4 && data[1] == 0x42 && data[3] == 0x46);
    // A request for the current program: it must get the patch without drift.
    if (usbSysExToDIN && length >= 5 && data[4] == 0x10) wonkPause(true, false);
  }
  if (usbSysExToDIN) {
    txSysEx(data, length);        // into the output queue; a 666-byte dump takes ~0.2 s on the cable
    if (complete) usbSysExToDIN = false;
  }
  // Collect the chunks into one message.
  if (length > 0 && data[0] == 0xF0) {
    usbSxLen = 0;
    usbSxTooLong = false;
  }
  if (usbSxLen + length <= sizeof usbSxBuf) {
    memcpy(usbSxBuf + usbSxLen, data, length);
    usbSxLen += length;
  } else {
    usbSxTooLong = true;
  }
  if (!complete) return;
  if (usbSxTooLong || usbSxLen < 2) {
    onSysExTooLong(true);
    return;
  }
  if (buddyHandle(usbSxBuf, (uint16_t)usbSxLen)) return;   // a message for the Buddy itself
  // Without F0 and F7, the way the Z1's own messages reach onSysEx().
  onSysEx(usbSxBuf + 1, usbSxLen - 2, /*fromUSB=*/true);
}

void processUSB() {
  auto z = Z1Edit::getInstance();
  while (usbMIDI.read()) {
    uint8_t type = usbMIDI.getType();

    if (type == usbMIDI.SystemExclusive) continue;  // callback handles it
    if (type >= 0xF8) {
      forwardUSB_RealTime_ToDIN(type);
      continue;
    }

    uint8_t ch = usbMIDI.getChannel();  // 1..16
    uint8_t d1 = usbMIDI.getData1();
    uint8_t d2 = usbMIDI.getData2();
    if (type == usbMIDI.NoteOn) noteCount(noteIn, (uint8_t)(0x90 | ((ch - 1) & 0x0F)), d1, d2);
    else if (type == usbMIDI.NoteOff) noteCount(noteIn, (uint8_t)(0x80 | ((ch - 1) & 0x0F)), d1, 0);
    if (z->getMainState() == mainState::NORMAL) {
      ch = 1;
      switch (type) {
        case usbMIDI.NoteOff:
          { txFlushExpression(ch); forwardUSB_ChannelMsg_ToDIN((uint8_t)(0x80 | ((ch - 1) & 0x0F)), d1, d2, 2); }
          lastNoteEventAt = millis();
          onMPE_NoteOff(ch, d1, d2);
          break;

        case usbMIDI.NoteOn:
          { txFlushExpression(ch); forwardUSB_ChannelMsg_ToDIN((uint8_t)(0x90 | ((ch - 1) & 0x0F)), d1, d2, 2); }
          lastNoteEventAt = millis();
          onMPE_NoteOn(ch, d1, d2);
          break;

        // ---- ACTIVE-DROPPED STREAMS (fan-out on ch1) ----
        case usbMIDI.ControlChange:
          {
            onMPE_CC(ch, d1, d2);

            if (d1 == 0 || d1 == 32) {
              // Bank select must reach the Z1 before the program change that
              // follows it, so it does not wait in the coalescing queue.
              forwardUSB_ChannelMsg_ToDIN((uint8_t)(0xB0 | ((ch - 1) & 0x0F)), d1, d2, 2);
              if (d1 == 32) buddyNoteBank(d2);
            } else {
              markCC(ch, d1, d2);
            }
          }
          break;

        case usbMIDI.AfterTouchPoly:
          markPoly(ch, d1, d2);
          onMPE_PolyPressure(ch, d1, d2);
          break;

        case usbMIDI.AfterTouchChannel:
          markChPress(ch, d1);
          onMPE_ChannelPressure(ch, d1);
          break;

        case usbMIDI.PitchBend:
          {
            uint16_t v14 = ((uint16_t)d2 << 7) | (d1 & 0x7F);
            onMPE_PitchBend(ch, v14);

            markPB(ch, v14);
          }
          break;

        // Immediate system/common
        case usbMIDI.ProgramChange:
          Serial.printf("usbMIDI.ProgramChange %d\r\n", d1);
          forwardUSB_ChannelMsg_ToDIN((uint8_t)(0xC0 | ((ch - 1) & 0x0F)), d1, 0, 1);
          buddyNoteProgram(d1);
          wonkPause(false, true);
          break;
        case usbMIDI.TimeCodeQuarterFrame:
          {
            uint8_t dataByte = ((d1 & 0x07) << 4) | (d2 & 0x0F);
            forwardUSB_SystemCommon_ToDIN(0xF1, dataByte, 0);
          }
          break;
        case usbMIDI.SongPosition:
          forwardUSB_SystemCommon_ToDIN(0xF2, d1, d2);
          break;
        case usbMIDI.SongSelect:
          forwardUSB_SystemCommon_ToDIN(0xF3, d1, 0);
          break;
        case usbMIDI.TuneRequest:
          forwardUSB_SystemCommon_ToDIN(0xF6, 0, 0);
          break;
        default: break;
      }

    } else {  //if we are in MPE mode forward global changes to voice channels
      switch (type) {
        // With unison on, a note channel is played by a group of timbres:
        // "unison" is true for those messages and g is the group.
        case usbMIDI.NoteOff:
          if (mpeVoices > 1 && ch >= 2) mpeUnisonNoteOff(ch, d1, d2);
          else { txFlushExpression(ch); forwardUSB_ChannelMsg_ToDIN((uint8_t)(0x80 | ((ch - 1) & 0x0F)), d1, d2, 2); }
          lastNoteEventAt = millis();
          onMPE_NoteOff(ch, d1, d2);
          break;

        case usbMIDI.NoteOn:
          if (mpeVoices > 1 && ch >= 2) {
            if (d2 == 0) mpeUnisonNoteOff(ch, d1, 0);
            else mpeUnisonNoteOn(ch, d1, d2);
          } else { txFlushExpression(ch); forwardUSB_ChannelMsg_ToDIN((uint8_t)(0x90 | ((ch - 1) & 0x0F)), d1, d2, 2); }
          lastNoteEventAt = millis();
          onMPE_NoteOn(ch, d1, d2);
          break;

        // ---- ACTIVE-DROPPED STREAMS (fan-out on ch1) ----
        case usbMIDI.ControlChange:
          {
            onMPE_CC(ch, d1, d2);
            Serial.printf("CC %d %d %d\r\n", ch, d1, d2);
            if (ch >= 2 && mpeRpn(ch, d1, d2)) break;   // the controller stating its bend range
            if (d1 == 74) d1 = editables[10001]->getIval();
            if (ch == 1) {
              uint8_t last = (MPE_MAX_CHANNEL < 2) ? 1 : (MPE_MAX_CHANNEL > 16 ? 16 : MPE_MAX_CHANNEL);
              for (uint8_t v = 2; v <= last; ++v) markCC(v, d1, d2);
            } else if (mpeVoices > 1) {
              int g = mpeGroupFor(ch);
              for (int k = 0; g >= 0 && k < mpeVoices; ++k) markCC(mpeOutCh(g, k), d1, d2);
            } else {
              markCC(ch, d1, d2);
            }
          }
          break;

        case usbMIDI.AfterTouchPoly:
          if (mpeVoices > 1 && ch >= 2) {
            int g = mpeGroupFor(ch);
            for (int k = 0; g >= 0 && k < mpeVoices; ++k) markPoly(mpeOutCh(g, k), d1, d2);
          } else markPoly(ch, d1, d2);
          onMPE_PolyPressure(ch, d1, d2);
          break;

        case usbMIDI.AfterTouchChannel:
          if (mpeVoices > 1 && ch >= 2) {
            int g = mpeGroupFor(ch);
            for (int k = 0; g >= 0 && k < mpeVoices; ++k) markChPress(mpeOutCh(g, k), d1);
          } else markChPress(ch, d1);
          onMPE_ChannelPressure(ch, d1);
          break;

        case usbMIDI.PitchBend:
          {
            uint16_t v14 = ((uint16_t)d2 << 7) | (d1 & 0x7F);

            if (mpeVoices > 1 && ch >= 2) {
              int g = mpeGroupFor(ch);
              for (int k = 0; g >= 0 && k < mpeVoices; ++k) markPB(mpeOutCh(g, k), v14);
            } else markPB(ch, v14);
          }
          break;

        // Immediate system/common
        case usbMIDI.ProgramChange:
          {
            uint32_t now = millis();
            // check for loop
            bool isEcho = (ch == lastDIN_ProgChan) && (d1 == lastDIN_ProgCh) && ((now - lastDIN_ProgTime) < PROG_LOOP_TIMEOUT_MS);
            if (!isEcho) {
              forwardUSB_ChannelMsg_ToDIN((uint8_t)(0xC0 | ((ch - 1) & 0x0F)), d1, 0, 1);
              wonkPause(false, true);
            }
          }
          break;
        case usbMIDI.TimeCodeQuarterFrame:
          {
            uint8_t dataByte = ((d1 & 0x07) << 4) | (d2 & 0x0F);
            forwardUSB_SystemCommon_ToDIN(0xF1, dataByte, 0);
          }
          break;
        case usbMIDI.SongPosition:
          forwardUSB_SystemCommon_ToDIN(0xF2, d1, d2);
          break;
        case usbMIDI.SongSelect:
          forwardUSB_SystemCommon_ToDIN(0xF3, d1, 0);
          break;
        case usbMIDI.TuneRequest:
          forwardUSB_SystemCommon_ToDIN(0xF6, 0, 0);
          break;
        default: break;
      }
    }
  }

  txPump();
  usbMIDI.send_now();  // flush
}






void setup() {

  pinMode(34, INPUT);
  pinMode(35, INPUT);
  pinMode(7, INPUT);
  pinMode(8, INPUT);
  pinMode(28, INPUT);
  pinMode(29, INPUT);

  // ---------- Setup / Loop ----------
#if USE_DEBUG
  Serial.begin(115200);
  while (!Serial && millis() < 2000) {}
  dbg("MIDI bridge starting...");
#endif

  Serial1.begin(31250);
  Serial1.addMemoryForRead(serinbuf, 32);
  Serial1.addMemoryForWrite(seroutbuf, 32);
  txCapacity = Serial1.availableForWrite();   // the send buffer is empty now: this is its size
  usbMIDI.setHandleSysEx(handleUSB_SysEx);

  dumpmode = false;
  bendSettingsLoad();



  //MIDI.begin(MIDI_CHANNEL_OMNI);

  Serial.println("i2c setup");
  Wire.setClock(1000000);
  //Wire.setClock(400000);
  //Wire.begin(I2C_MASTER, 0x00, I2C_PINS_18_19, I2C_PULLUP_EXT, I2C_RATE_1800, I2C_OP_MODE_DMA);
  Wire.begin();
  //Wire.resetBus();
  Wire.beginTransmission(MCP_LB1);
  Wire.write(MCP23017_IODIRA);
  Wire.write(0x00);  // set all of port A to outputs
  Wire.endTransmission();
  Wire.beginTransmission(MCP_LB1);
  Wire.write(MCP23017_IODIRB);  // IODIRB register
  Wire.write(0x00);             // set all of port B to outputs
  Wire.endTransmission();
  Serial.println("i2c chip0 done");






  Serial.println("i2c done");

  pinMode(39, INPUT);
  attachInterrupt(digitalPinToInterrupt(39), extInt5, FALLING);
  pinMode(38, INPUT);
  attachInterrupt(digitalPinToInterrupt(38), extInt4, FALLING);
  pinMode(27, INPUT);
  attachInterrupt(digitalPinToInterrupt(27), extInt3, FALLING);
  pinMode(26, INPUT);
  attachInterrupt(digitalPinToInterrupt(26), extInt2, FALLING);
  pinMode(25, INPUT);
  attachInterrupt(digitalPinToInterrupt(25), extInt1, FALLING);
  pinMode(24, INPUT);
  attachInterrupt(digitalPinToInterrupt(24), extInt0, FALLING);

  pinMode(led, OUTPUT);
  lcd.begin(numCols, numRows);
  lcd.print("Hello there");

  lcd2.begin(numCols, numRows);
  lcd2.print("Hello there lcd2");

  lcd3.begin(numCols, numRows);
  lcd3.print("Hello there lcd3");

  auto s = SynthBuddy::getInstance();
  auto z = Z1Edit::getInstance();
  s->addDisplay(&lcd);
  s->addDisplay(&lcd2);
  s->addDisplay(&lcd3);
  s->configEncBoard(MCP_LB2);
  s->configEncBoard(MCP_LB3);
  s->configEncBoard(MCP_LB4);
  z->init();
  Serial.println("init done");

  z->showOSC1();
  Serial.println("showosc1 done");

  z->getCurrPatch();
  Serial.println("startup done");
}




int prevtime;
int foo;

void loop() {
  static uint32_t loopStartedAt = 0;
  uint32_t loopNow = millis();
  if (loopStartedAt && loopNow - loopStartedAt > dbgStats.maxLoopMs) dbgStats.maxLoopMs = (loopNow - loopStartedAt) > 65535 ? 65535 : (uint16_t)(loopNow - loopStartedAt);
  loopStartedAt = loopNow;

  auto s = SynthBuddy::getInstance();
  auto z = Z1Edit::getInstance();

  if (int5) {
    while (digitalRead(39) == 0) {
      //Serial.print(digitalRead(39));
      uint16_t v = s->readEncBoard(MCP_LB2);
      int5 = false;
      s->encupdate(5, v & 0x0001, v & 0x0002);
      s->encupdate(4, v & 0x0004, v & 0x0008);
      s->encupdate(3, v & 0x0010, v & 0x0020);
      s->encupdate(2, v & 0x0040, v & 0x0080);
      s->encupdate(1, v & 0x2000, v & 0x1000);
      s->encupdate(0, v & 0x8000, v & 0x4000);

      if (v == 0x600) s->button(0);
      if (v == 0x500) s->button(1);
      if (v == 0x400) s->button(2);
      if (v == 0x300) s->button(3);
      if (v == 0x200) s->button(4);
      if (v == 0x100) s->button(5);
      //Serial.print(digitalRead(39));
      //Serial.println(v,HEX);
    }
  }

  if (int4) {
    while (digitalRead(38) == 0) {
      //Serial.print(digitalRead(39));
      uint16_t v = s->readEncBoard(MCP_LB3);
      int4 = false;
      s->encupdate(11, v & 0x0001, v & 0x0002);
      s->encupdate(10, v & 0x0004, v & 0x0008);
      s->encupdate(9, v & 0x0010, v & 0x0020);
      s->encupdate(8, v & 0x0080, v & 0x0040);//flipped
      s->encupdate(7, v & 0x1000, v & 0x2000);//flipped
      s->encupdate(6, v & 0x4000, v & 0x8000);//flipped

      if (v == 0x600) s->button(6);
      if (v == 0x500) s->button(7);
      if (v == 0x400) s->button(8);
      if (v == 0x300) s->button(9);
      if (v == 0x200) s->button(10);
      if (v == 0x100) s->button(11);

      //Serial.print(digitalRead(39));
      //Serial.println(v,HEX);
    }
  }

  if (int3) {
    while (digitalRead(27) == 0) {
      //Serial.print(digitalRead(39));
      uint16_t v = s->readEncBoard(MCP_LB4);
      int3 = false;
      s->encupdate(17, v & 0x0001, v & 0x0002);
      s->encupdate(16, v & 0x0004, v & 0x0008);
      s->encupdate(15, v & 0x0010, v & 0x0020);
      s->encupdate(14, v & 0x0040, v & 0x0080);
      s->encupdate(13, v & 0x2000, v & 0x1000);
      s->encupdate(12, v & 0x8000, v & 0x4000);

      if (v == 0x600) s->button(12);
      if (v == 0x500) s->button(13);
      if (v == 0x400) s->button(14);
      if (v == 0x300) s->button(15);
      if (v == 0x200) s->button(16);
      if (v == 0x100) s->button(17);
      //Serial.print(digitalRead(39));
      //Serial.println(v,HEX);
    }
  }


  if (S10.update()) {
    if (S10.fallingEdge()) {
      Serial.println("S10");
      dumpmode = true;
      wonkPause(true, false);
      z->getCurrPatch();
      
      
    }
  }
  if (S11.update()) {
    if (S11.fallingEdge()) {
      Serial.println("S11");
      z->toggleMod();
    }
  }

  if (SI0.update() || SI1.update() || SI2.update() || SI3.update()) {
    if (SI0.fallingEdge() || SI1.fallingEdge() || SI2.fallingEdge() || SI3.fallingEdge()) {
      int v = digitalRead(7) | (digitalRead(8) << 1) | (digitalRead(28) << 2) | (digitalRead(29) << 3);
      if (millis() - prevtime > 150) {
        prevtime = millis();
        Serial.printf("button %d\r\n", v);
        switch (v) {
          case 9:  //1001
            //z.setEditState(editState::OSC1);
            hideAll();
            z->showOSC1();
            break;
          case 8:  //1000
            hideAll();
            z->showOSC2();
            //z.setEditState(editState::OSC2);
            break;
          case 10:  //1010
            //z.setEditState(editState::OSCBOTH);
            hideAll();
            z->showSubOsc();
            break;
          case 7:  //0111
            //z.setEditState(editState::FILT);
            hideAll();
            z->showFilt();
            break;
          case 14:  //1110
            //z.setEditState(editState::FX);
            hideAll();
            z->showFx();
            break;
          case 13:  //1101
            //z.setEditState(editState::EG);
            hideAll();
            z->showEG();
            break;
          case 12:  //1100
            //z.setEditState(editState::LFO);
            hideAll();
            z->showLFO();
            break;
          case 6:
            z->nextUnisonMode();
            break;
          case 11:  //1011
            z->nextMainMode();
            break;

          default:
            break;
        }
      }
    }
  }

  pot0 = (int)(110.0 - (110.0) * (((double)analogRead(A6) + (double)analogRead(A6) + (double)analogRead(A6)) / (1023.0 * 3.0)));
  pot1 = (int)(110.0 - (110.0) * (((double)analogRead(A7) + (double)analogRead(A7) + (double)analogRead(A7)) / (1023.0 * 3.0)));
  pot2 = (int)(110.0 - (110.0) * (((double)analogRead(A8) + (double)analogRead(A8) + (double)analogRead(A8)) / (1023.0 * 3.0)));

  if (pot0 < 5) pot0 = 0;
  else pot0 = pot0 - 5;
  if (pot1 < 5) pot1 = 0;
  else pot1 = pot1 - 5;
  if (pot2 < 5) pot2 = 0;
  else pot2 = pot2 - 5;
  if (pot0 > 99) pot0 = 99;
  if (pot1 > 99) pot1 = 99;
  if (pot2 > 99) pot2 = 99;

  //z.process();

  // A pot acts when it moves. A value set from the computer (potSet()) stays
  // until the pot has been moved clearly away from where it stood then.
  int potNow[3] = { pot0, pot1, pot2 };
  for (int i = 0; i < 3; ++i) {
    if (potRemote[i]) {
      if (abs(potNow[i] - potAnchor[i]) < POT_TAKEOVER) continue;
      potRemote[i] = false;
    } else if (potNow[i] == potValue[i]) {
      continue;
    }
    Serial.printf("pot%d %d\n", i, potNow[i]);
    potApply(i, potNow[i]);
  }
  pot0_o = pot0;
  pot1_o = pot1;
  pot2_o = pot2;
  wonkTick(millis());
  potShowTick(millis());
  buddyTick(millis());

  processDIN();
  processUSB();
}
