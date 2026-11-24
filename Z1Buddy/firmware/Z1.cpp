#include "Z1.h"
#include <Arduino.h>
#include <vector>
#include <sstream>
#include "Editable.h"

#include "midinames.h"


#define O1 0x800
#define O2 0x1000
#define E1 0x1800
#define E2 0x2000
#define EGL 0x2800

static const std::vector<std::string> z1OscTypes{ "Std", "Comb", "VPM", "Reso", "Ring", "Cross", "Sync", "Organ", "EP", "Brass", "Reed", "Pluck", "Bowed" };
static const std::vector<std::string> z1OscStdWave{ "Saw", "Pulse" };
static const std::vector<std::string> z1OscOrgWave{ "Sin1", "Sin2", "Sin3", "Tri" };
static const std::vector<std::string> z1OscOrgTrig{ "Single", "Multi" };
static const std::vector<std::string> z1OscStdWaveShapeTbl{ "Clip", "Reso" };
static const std::vector<std::string> z1OscCompIn{ "SC1+N", "SubO+N", "F1+N", "F2+N", "PulseN", "Impuls" };
static const std::vector<std::string> z1OscVPMCarrWave{ "Saw", "Square", "Tri", "Sin" };
static const std::vector<std::string> z1OscVPMModWave{ "Saw", "Square", "Tri", "Sin", "OSC1", "OSC2", "SubOSC", "Filt1", "Filt2" };
static const std::vector<std::string> z1OscResoIn{ "OSC1", "OSC2", "SubOsc", "Noise", "Filt1", "Filt2" };
static const std::vector<std::string> z1OscBrassTypes{ "Brass1", "Brass2", "Brass3", "Horn1", "Horn2", "ReedBr" };
static const std::vector<std::string> z1OscReedTypes{ "HSax1", "HSax2", "HSax3", "SSax1", "SSax1", "DReed1", "DReed2", "Bassoon", "Clarin", "Flute1", "Flute2", "PanFl", "Ocarin", "Shakuh", "Harmo1", "Harmo2", "ReedSn" };
static const std::vector<std::string> z1OscPressEG{ "", "EG1", "EG2", "EG3", "EG4", "A.EG" };
static const std::vector<std::string> z1OscPBType{ "Smooth", "Jump" };
static const std::vector<std::string> z1FiltRout{ "Ser1", "Ser2", "Par" };
static const std::vector<std::string> z1FiltKnob{ "Filt1", "Filt2", "Both" };
static const std::vector<std::string> z1FiltType{ "", "LPF", "HPF", "BPF", "BRF", "2BPF" };
static const std::vector<std::string> z1OffOn{ "Off", "On" };
static const std::vector<std::string> z1TriSin{ "Tri", "Sin" };
static const std::vector<std::string> z1NoiseFilt{ "THRU", "LPF", "HPF", "BPF" };
static const std::vector<std::string> z1SlowFast{ "Slow", "Fast" };
static const std::vector<std::string> z1ControlMode{ "Manual", "Mod" };
static const std::vector<std::string> z1RotaryMode{ "Rotate", "Stop" };
static const std::vector<std::string> z1MTapType{ "Normal", "Cross1", "Cross2", "Pan1", "Pan2" };
static const std::vector<std::string> z1AIUEO{ "A", "I", "U", "E", "O" };
static const std::vector<std::string> z1Lfos{ "LFO1", "LFO2", "LFO3", "LFO4" };
static const std::vector<std::string> z1ModSrc1{ "Off", "EG1", "EG2", "EG3", "EG4", "AmpEG", "LFO1", "LFO2", "LFO3", "LFO4", "Portam", "NoteL", "NoteEx", "NSplH", "NSplL", "VelSft", "VelMed", "VelHrd", "PBend", "AfterT", "MWheel", "At+MW", "At2+MW", "X+-", "X+", "X-", "Y+-", "Y+", "Y-", "Knob1", "Knob2", "Knob3", "Knob4", "Knob5", "ModSw1", "ModSw2", "FootSw", "FootPd", "Damper",
                                                 "Sosten", "Breath", "Vol", "Panpot", "Expres", "PorTim", "PorSW", "M FX", "FX1", "FX2" };
static const std::vector<std::string> z1ModSrc2{ "Off", "NoteL", "NoteEx", "NSplH", "NSplL", "VelSft", "VelMed", "VelHrd", "PBend", "AfterT", "MWheel", "At+MW", "At2+MW", "X+-", "X+", "X-", "Y+-", "Y+", "Y-", "Knob1", "Knob2", "Knob3", "Knob4", "Knob5", "ModSw1", "ModSw2", "FootSw", "FootPd", "Damper",
                                                 "Sosten", "Breath", "Vol", "Panpot", "Expres", "PorTim", "PorSW" };  //numbers however same as in set 1
static const std::vector<std::string> z1FxTypes{ "OvrDrv", "Compr", "PEQ", "Wah", "Excitr", "Decim", "Chorus", "Flangr", "Phaser", "RotarS", "Delay", "Talkin", "MTap", "Ensemb", "Rotary" };
static const std::vector<std::string> z1MFxTypes{ "Delay", "Hall", "Room" };
static const std::vector<std::string> z1DelayTypes{ "Stereo", "Cross" };
static const std::vector<std::string> z1OverdriveMode{ "Overdr", "Dist" };
static const std::vector<std::string> z1EQType{ "Peak", "Shelf" };
static const std::vector<std::string> z1WahMode{ "Auto", "Mod" };
static const std::vector<std::string> z1WahFilt{ "BPF", "LPF" };
static const std::vector<std::string> z1ChoPhase{ "+", "-", "+-" };
static const std::vector<std::string> z1LFOTypes{ "Tri0", "Tri90", "TriRnd", "Sine", "SawU0", "SawU180", "SawD0", "SawD180", "Square", "RndS/H", "RndVec", "StpTr4", "StpTr6", "StpSw4", "StpSw6", "ExpTri", "ExpSwU", "ExpSwD" };
static const std::vector<std::string> z1LFOSyncSW{ "OFF", "Timbre", "Voice" };
static const std::vector<std::string> z1Categories{ "Synth-Hard", "Synth-Soft", "Synth-Lead", "Synth-Motion", "Synth-Bass", "E.Piano", "Organ", "Keyboard", "Bell", "Strings", "Band/Choir", "Brass", "Reed/Wind", "Guitar/Plucked", "Bass", "Percussive", "Argpeggio", "SFX/Other" };


std::string z1ScaleFunc(int i, int prec, float f1, float f2, float f3, float f4) {
  // min max steps minival
  std::stringstream ss;
  ss.setf(std::ios::fixed);
  ss.precision(prec);
  float v = f1 + ((float)i - f4) * ((f2 - f1) / f3);
  Serial.printf("i %d v %f f1 %f f2 %f f3 %f F4 %f\r\n", i, v, f1, f2, f3, f4);
  ss << v;
  std::string s = ss.str();
  return s;
}

std::string z1ExpFunc(int i, int prec, float f1, float f2, float f3, float f4) {
  // min max steps minival
  std::stringstream ss;
  ss.setf(std::ios::fixed);
  ss.precision(2);
  float v = f1 * exp((float)i * f2);
  if (v > 1000.0) {
    v = v / 1000.0;
    ss << v;
    ss << "k";
  } else {
    ss << v;
  }
  Serial.printf("i %d v %f f1 %f f2 %f f3 %f F4 %f\r\n", i, v, f1, f2, f3, f4);

  std::string s = ss.str();
  return s;
}


// In firmware.ino: everything for the Z1 goes through the output queue.
void z1Write(uint8_t b);
void z1Write(const uint8_t* data, size_t n);

// In firmware.ino: the unison button in MPE mode.
extern void mpeUnisonChanged(int voices);

Z1Edit *Z1Edit::instance = nullptr;

Z1Edit::Z1Edit() {
  s = SynthBuddy::getInstance();
  currPageFunc = nullptr;
}

Z1Edit *Z1Edit::getInstance() {
    if(instance==nullptr) instance=new Z1Edit();
    return instance;
}

void FLASHMEM Z1Edit::init() {



  //EG
  editables[31] = new EditableValue(31, "EG1 Srt", -99, 99, 0, editableType::Integer);
  editables[50] = new EditableValue(50, "EG2 Srt", -99, 99, 0, editableType::Integer);
  editables[69] = new EditableValue(69, "EG3 Srt", -99, 99, 0, editableType::Integer);
  editables[88] = new EditableValue(88, "EG4 Srt", -99, 99, 0, editableType::Integer);
  int u;
  for (int i = 0; i <= 3; i++) {
    u = i * 19;
    editables[32 + u] = new EditableValue(32 + u, "Attack", 0, 99, 0, editableType::Integer);
    editables[33 + u] = new EditableValue(33 + u, "AttLvl", -99, 99, 0, editableType::Integer);
    editables[34 + u] = new EditableValue(34 + u, "Decay", 0, 99, 0, editableType::Integer);
    editables[35 + u] = new EditableValue(35 + u, "Break", -99, 99, 0, editableType::Integer);
    editables[36 + u] = new EditableValue(36 + u, "Slope", 0, 99, 0, editableType::Integer);
    editables[37 + u] = new EditableValue(37 + u, "Sustain", -99, 99, 0, editableType::Integer);
    editables[38 + u] = new EditableValue(38 + u, "RelT", 0, 99, 0, editableType::Integer);
    editables[39 + u] = new EditableValue(39 + u, "Release", -99, 99, 0, editableType::Integer);
    editables[40 + u] = new EditableValue(40 + u, "LvlMod", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
    editables[41 + u] = new EditableValue(41 + u, "int", -99, 99, 0, editableType::Integer);
    editables[42 + u] = new EditableValue(42 + u, "Veloci", -99, 99, 0, editableType::Integer);
    editables[43 + u] = new EditableValue(43 + u, "TMod", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
    editables[44 + u] = new EditableValue(44 + u, "int", -99, 99, 0, editableType::Integer);
    editables[45 + u] = new EditableValue(45 + u, "NodeMod", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
    editables[46 + u] = new EditableValue(46 + u, "AttInt", -99, 99, 0, editableType::Integer);
    editables[47 + u] = new EditableValue(47 + u, "DecInt", -99, 99, 0, editableType::Integer);
    editables[48 + u] = new EditableValue(48 + u, "SlopInt", -99, 99, 0, editableType::Integer);
    editables[49 + u] = new EditableValue(49 + u, "RelInt", -99, 99, 0, editableType::Integer);
  }

  //amp1
  editables[315] = new EditableValue(315, "Trak1", 0, 99, 0, editableType::Integer);
  editables[316] = new EditableValue(316, "LKey1", 0, midiNoteNames.size() - 1, 60, editableType::TextEnum, midiNoteNames);
  editables[317] = new EditableValue(317, "HKey1", 0, midiNoteNames.size() - 1, 60, editableType::TextEnum, midiNoteNames);
  editables[318] = new EditableValue(318, "Lint1", -99, 99, 0, editableType::Integer);
  editables[319] = new EditableValue(319, "Hint1", -99, 99, 0, editableType::Integer);
  editables[320] = new EditableValue(320, "ModEG1", 1, z1OscPressEG.size() - 1, 0, editableType::TextEnum, z1OscPressEG);
  //no 321
  editables[322] = new EditableValue(322, "ModSrc1", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
  editables[323] = new EditableValue(323, "int1", -99, 99, 0, editableType::Integer);
  //amp2
  editables[324] = new EditableValue(324, "Trak2", 0, 99, 0, editableType::Integer);
  editables[325] = new EditableValue(325, "LKey2", 0, midiNoteNames.size() - 1, 60, editableType::TextEnum, midiNoteNames);
  editables[326] = new EditableValue(326, "HKey2", 0, midiNoteNames.size() - 1, 60, editableType::TextEnum, midiNoteNames);
  editables[327] = new EditableValue(327, "Lint2", -99, 99, 0, editableType::Integer);
  editables[328] = new EditableValue(328, "Hint2", -99, 99, 0, editableType::Integer);
  editables[329] = new EditableValue(329, "ModEG2", 1, z1OscPressEG.size() - 1, 0, editableType::TextEnum, z1OscPressEG);
  //no 330
  editables[331] = new EditableValue(331, "ModSrc2", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
  editables[332] = new EditableValue(332, "int2", -99, 99, 0, editableType::Integer);


  //amp EG

  editables[334] = new EditableValue(334, "Attack", 0, 99, 0, editableType::Integer);
  editables[335] = new EditableValue(335, "AttLvl", 0, 99, 0, editableType::Integer);
  editables[336] = new EditableValue(336, "Decay", 0, 99, 0, editableType::Integer);
  editables[337] = new EditableValue(337, "Break", 0, 99, 0, editableType::Integer);
  editables[338] = new EditableValue(338, "Slope", 0, 99, 0, editableType::Integer);
  editables[339] = new EditableValue(339, "Sustain", 0, 99, 0, editableType::Integer);
  editables[340] = new EditableValue(340, "RelT", 0, 99, 0, editableType::Integer);

  editables[342] = new EditableValue(342, "LvlMod", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
  editables[343] = new EditableValue(343, "int", -99, 99, 0, editableType::Integer);
  editables[344] = new EditableValue(344, "Veloci", -99, 99, 0, editableType::Integer);
  editables[345] = new EditableValue(345, "TMod", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
  editables[346] = new EditableValue(346, "int", -99, 99, 0, editableType::Integer);
  editables[347] = new EditableValue(347, "NodeMod", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
  editables[348] = new EditableValue(348, "AttInt", -99, 99, 0, editableType::Integer);
  editables[349] = new EditableValue(349, "AttInt", -99, 99, 0, editableType::Integer);
  editables[350] = new EditableValue(350, "SlopInt", -99, 99, 0, editableType::Integer);
  editables[351] = new EditableValue(351, "RelInt", -99, 99, 0, editableType::Integer);



  //LFO
  for (int i = 0; i <= 3; i++) {
    u = i * 14;
    editables[107 + u] = new EditableValue(107 + u, "Type", 0, z1LFOTypes.size() - 1, 0, editableType::TextEnum, z1LFOTypes);
    editables[108 + u] = new EditableValue(108 + u, "Sync", 0, z1LFOSyncSW.size() - 1, 0, editableType::TextEnum, z1LFOSyncSW);
    editables[109 + u] = new EditableValue(109 + u, "Freq", 0, 199, 0, editableType::Integer);
    editables[110 + u] = new EditableValue(110 + u, "FrqMod1", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
    editables[111 + u] = new EditableValue(111 + u, "int", -99, 99, 0, editableType::Integer);
    editables[112 + u] = new EditableValue(112 + u, "FrqMod2", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
    editables[113 + u] = new EditableValue(113 + u, "int", -99, 99, 0, editableType::Integer);
    editables[114 + u] = new EditableValue(114 + u, "FadeIn", 0, 99, 0, editableType::Integer);
    editables[115 + u] = new EditableValue(115 + u, "AmpMod", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
    editables[116 + u] = new EditableValue(116 + u, "int", -99, 99, 0, editableType::Integer);
    editables[117 + u] = new EditableValue(117 + u, "Offset", -50, 50, 0, editableType::Integer);
    editables[118 + u] = new EditableValue(118 + u, "MSync", 0, z1OffOn.size() - 1, 0, editableType::TextEnum, z1OffOn);
    editables[119 + u] = new EditableValue(119 + u, "MBase", 0, 7, 0, editableType::Integer);
    editables[120 + u] = new EditableValue(120 + u, "MTime", 0, 15, 0, editableType::Integer);
  }

  // OSC1 Common
  editables[174] = new EditableValue(174, "OSC1", 0, z1OscTypes.size() - 1, 0, editableType::TextEnum, z1OscTypes);


  // OSC1 octave
  editables[175] = new EditableValue(175, "Oct", 0, 3, 0, editableType::Integer);
  // OSC1 semitone
  editables[176] = new EditableValue(176, "Semi", -12, 12, 0, editableType::Integer);
  // OSC1 finetune
  editables[177] = new EditableValue(177, "Fine", -50, 50, 0, editableType::Integer);

  // OSC2 Common
  editables[188] = new EditableValue(188, "OSC2", 0, 8, 0, editableType::TextEnum, z1OscTypes);
  editables[189] = new EditableValue(189, "Oct", 0, 3, 0, editableType::Integer);
  editables[190] = new EditableValue(190, "Semi", -12, 12, 0, editableType::Integer);
  editables[191] = new EditableValue(191, "Fine", -50, 50, 0, editableType::Integer);


  for (int i = 0; i <= 1; i++) {
    u = O1;
    if (i == 1) u = O2;
    // OSC1 std
    editables[464 | u] = new EditableValue(464 | u, "Wave", 0, z1OscStdWave.size() - 1, 0, editableType::TextEnum, z1OscStdWave);
    editables[465 | u] = new EditableValue(465 | u, "WEdge", 0, 99, 0, editableType::Integer);
    editables[466 | u] = new EditableValue(466 | u, "WLv", 0, 99, 0, editableType::Integer);
    editables[467 | u] = new EditableValue(467 | u, "TriLv", 0, 99, 0, editableType::Integer);
    editables[468 | u] = new EditableValue(468 | u, "SinLv", 0, 99, 0, editableType::Integer);
    editables[469 | u] = new EditableValue(469 | u, "TriPs", -99, 99, 0, editableType::Integer);
    editables[470 | u] = new EditableValue(470 | u, "WFrm", -99, 99, 0, editableType::Integer);
    editables[475 | u] = new EditableValue(475 | u, "WSInLv", 0, 99, 0, editableType::Integer);
    editables[478 | u] = new EditableValue(478 | u, "WSOff", -99, 99, 0, editableType::Integer);
    editables[479 | u] = new EditableValue(479 | u, "WsTbl", 0, z1OscStdWaveShapeTbl.size() - 1, 0, editableType::TextEnum, z1OscStdWaveShapeTbl);
    editables[480 | u] = new EditableValue(480 | u, "WShape", 0, 99, 0, editableType::Integer);
    editables[483 | u] = new EditableValue(483 | u, "WShBal", 0, 99, 0, editableType::Integer);
    // mod
    editables[471 | u] = new EditableValue(471 | u, "FormLFO", 0, z1Lfos.size() - 1, 0, editableType::TextEnum, z1Lfos);
    editables[472 | u] = new EditableValue(472 | u, "int", -99, 99, 0, editableType::Integer);
    editables[473 | u] = new EditableValue(473 | u, "Form", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
    editables[474 | u] = new EditableValue(474 | u, "int", -99, 99, 0, editableType::Integer);
    editables[476 | u] = new EditableValue(476 | u, "Level", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
    editables[477 | u] = new EditableValue(477 | u, "int", -99, 99, 0, editableType::Integer);
    editables[481 | u] = new EditableValue(481 | u, "Shape", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
    editables[482 | u] = new EditableValue(482 | u, "int", -99, 99, 0, editableType::Integer);
    editables[484 | u] = new EditableValue(484 | u, "Balans", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
    editables[485 | u] = new EditableValue(485 | u, "int", -99, 99, 0, editableType::Integer);

    //Comb
    editables[486 | u] = new EditableValue(486 | u, "Input", 0, z1OscCompIn.size() - 1, 0, editableType::TextEnum, z1OscCompIn);
    editables[487 | u] = new EditableValue(487 | u, "Level", 0, 99, 0, editableType::Integer);
    editables[488 | u] = new EditableValue(488 | u, "Noise", 0, 99, 0, editableType::Integer);
    editables[489 | u] = new EditableValue(489 | u, "Width", 0, 99, 0, editableType::Integer);
    editables[492 | u] = new EditableValue(492 | u, "FeedB", 0, 99, 0, editableType::Integer);
    editables[497 | u] = new EditableValue(497 | u, "HDamp", 0, 99, 0, editableType::Integer);
    //  mod
    editables[490 | u] = new EditableValue(490 | u, "Level", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
    editables[491 | u] = new EditableValue(491 | u, "int", -99, 99, 0, editableType::Integer);
    editables[493 | u] = new EditableValue(493 | u, "FeedB1", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
    editables[494 | u] = new EditableValue(494 | u, "int", -99, 99, 0, editableType::Integer);
    editables[495 | u] = new EditableValue(495 | u, "FeedB2", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
    editables[496 | u] = new EditableValue(496 | u, "int", -99, 99, 0, editableType::Integer);
    editables[498 | u] = new EditableValue(498 | u, "HDamp", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
    editables[499 | u] = new EditableValue(499 | u, "int", -99, 99, 0, editableType::Integer);

    //VPM
    editables[500 | u] = new EditableValue(500 | u, "Carr", 0, z1OscVPMCarrWave.size() - 1, 0, editableType::TextEnum, z1OscVPMCarrWave);
    editables[501 | u] = new EditableValue(501 | u, "Level", 0, 99, 0, editableType::Integer);
    editables[506 | u] = new EditableValue(506 | u, "WShape", 0, 99, 0, editableType::Integer);
    editables[511 | u] = new EditableValue(511 | u, "WSType", 0, 1, 0, editableType::Integer);
    editables[512 | u] = new EditableValue(512 | u, "FeedB", 0, 99, 0, editableType::Integer);
    editables[513 | u] = new EditableValue(513 | u, "FCoarse", 0, 16, 0, editableType::Integer);
    editables[514 | u] = new EditableValue(514 | u, "FFine", -50, 50, 0, editableType::Integer);
    editables[519 | u] = new EditableValue(519 | u, "Modul", 0, z1OscVPMModWave.size() - 1, 0, editableType::TextEnum, z1OscVPMModWave);
    editables[520 | u] = new EditableValue(520 | u, "Level", 0, 99, 0, editableType::Integer);
    //mod
    editables[502 | u] = new EditableValue(502 | u, "CLvl1", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
    editables[503 | u] = new EditableValue(503 | u, "int", -99, 99, 0, editableType::Integer);
    editables[504 | u] = new EditableValue(504 | u, "CLvl2", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
    editables[505 | u] = new EditableValue(505 | u, "int", -99, 99, 0, editableType::Integer);
    editables[507 | u] = new EditableValue(507 | u, "Shape1", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
    editables[508 | u] = new EditableValue(508 | u, "int", -99, 99, 0, editableType::Integer);
    editables[509 | u] = new EditableValue(509 | u, "Shape2", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
    editables[510 | u] = new EditableValue(510 | u, "int", -99, 99, 0, editableType::Integer);
    editables[515 | u] = new EditableValue(515 | u, "MFreq1", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
    editables[516 | u] = new EditableValue(516 | u, "int", -99, 99, 0, editableType::Integer);
    editables[517 | u] = new EditableValue(517 | u, "MFreq2", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
    editables[518 | u] = new EditableValue(518 | u, "int", -99, 99, 0, editableType::Integer);
    editables[521 | u] = new EditableValue(521 | u, "MLvl1", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
    editables[522 | u] = new EditableValue(522 | u, "int", -99, 99, 0, editableType::Integer);
    editables[523 | u] = new EditableValue(523 | u, "MLvl2", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
    editables[524 | u] = new EditableValue(524 | u, "int", -99, 99, 0, editableType::Integer);

    //Resonance
    editables[525 | u] = new EditableValue(525 | u, "Input", 0, z1OscResoIn.size() - 1, 0, editableType::TextEnum, z1OscResoIn);
    editables[526 | u] = new EditableValue(526 | u, "Level", 0, 99, 0, editableType::Integer);
    editables[531 | u] = new EditableValue(531 | u, "Reso1", 0, 99, 0, editableType::Integer);
    editables[532 | u] = new EditableValue(532 | u, "Harm1", 0, 15, 0, editableType::Integer);
    editables[535 | u] = new EditableValue(535 | u, "FFin1", -99, 99, 0, editableType::Integer);
    editables[536 | u] = new EditableValue(536 | u, "Lvl1", 0, 99, 0, editableType::Integer);
    editables[537 | u] = new EditableValue(537 | u, "Reso2", 0, 99, 0, editableType::Integer);
    editables[538 | u] = new EditableValue(538 | u, "Harm2", 0, 15, 0, editableType::Integer);
    editables[541 | u] = new EditableValue(541 | u, "FFin2", -99, 99, 0, editableType::Integer);
    editables[542 | u] = new EditableValue(542 | u, "Lvl2", 0, 99, 0, editableType::Integer);
    editables[543 | u] = new EditableValue(543 | u, "Reso3", 0, 99, 0, editableType::Integer);
    editables[544 | u] = new EditableValue(544 | u, "Harm3", 0, 15, 0, editableType::Integer);
    editables[547 | u] = new EditableValue(547 | u, "FFin3", -99, 99, 0, editableType::Integer);
    editables[548 | u] = new EditableValue(548 | u, "Lvl3", 0, 99, 0, editableType::Integer);
    editables[549 | u] = new EditableValue(549 | u, "Reso4", 0, 99, 0, editableType::Integer);
    editables[550 | u] = new EditableValue(550 | u, "Harm4", 0, 15, 0, editableType::Integer);
    editables[553 | u] = new EditableValue(553 | u, "FFin4", -99, 99, 0, editableType::Integer);
    editables[554 | u] = new EditableValue(554 | u, "Lvl4", 0, 99, 0, editableType::Integer);
    //mod
    editables[527 | u] = new EditableValue(527 | u, "Level1", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
    editables[528 | u] = new EditableValue(528 | u, "int", -99, 99, 0, editableType::Integer);
    editables[529 | u] = new EditableValue(529 | u, "Level2", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
    editables[530 | u] = new EditableValue(530 | u, "int", -99, 99, 0, editableType::Integer);
    editables[533 | u] = new EditableValue(533 | u, "Harm1", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
    editables[534 | u] = new EditableValue(534 | u, "int", -15, 15, 0, editableType::Integer);
    editables[539 | u] = new EditableValue(539 | u, "Harm2", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
    editables[540 | u] = new EditableValue(540 | u, "int", -15, 15, 0, editableType::Integer);
    editables[545 | u] = new EditableValue(545 | u, "Harm3", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
    editables[546 | u] = new EditableValue(546 | u, "int", -15, 15, 0, editableType::Integer);
    editables[551 | u] = new EditableValue(551 | u, "Harm4", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
    editables[552 | u] = new EditableValue(552 | u, "int", -15, 15, 0, editableType::Integer);
    editables[555 | u] = new EditableValue(555 | u, "Reso", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
    editables[556 | u] = new EditableValue(556 | u, "int", -99, 99, 0, editableType::Integer);


    //Ring
    editables[557 | u] = new EditableValue(557 | u, "Input", 0, z1OscResoIn.size() - 1, 0, editableType::TextEnum, z1OscResoIn);
    editables[558 | u] = new EditableValue(558 | u, "Carr", 0, z1OscVPMCarrWave.size() - 1, 0, editableType::TextEnum, z1OscVPMCarrWave);
    editables[559 | u] = new EditableValue(559 | u, "ModDpt", 0, 99, 0, editableType::Integer);
    editables[560 | u] = new EditableValue(560 | u, "DptSrc1", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
    editables[561 | u] = new EditableValue(561 | u, "int", -99, 99, 0, editableType::Integer);
    editables[562 | u] = new EditableValue(562 | u, "DptSrc2", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
    editables[563 | u] = new EditableValue(563 | u, "int", -99, 99, 0, editableType::Integer);
    editables[564 | u] = new EditableValue(564 | u, "Type", 0, 1, 0, editableType::Integer);
    editables[565 | u] = new EditableValue(565 | u, "Edge", 0, 99, 0, editableType::Integer);

    //Cross
    editables[566 | u] = new EditableValue(566 | u, "Input", 0, z1OscResoIn.size() - 1, 0, editableType::TextEnum, z1OscResoIn);
    editables[567 | u] = new EditableValue(567 | u, "Carr", 0, z1OscVPMCarrWave.size() - 1, 0, editableType::TextEnum, z1OscVPMCarrWave);
    editables[568 | u] = new EditableValue(568 | u, "ModDpt", 0, 99, 0, editableType::Integer);
    editables[569 | u] = new EditableValue(569 | u, "DptSrc1", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
    editables[570 | u] = new EditableValue(570 | u, "int", -99, 99, 0, editableType::Integer);
    editables[571 | u] = new EditableValue(571 | u, "DptSrc2", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
    editables[572 | u] = new EditableValue(572 | u, "int", -99, 99, 0, editableType::Integer);
    editables[573 | u] = new EditableValue(573 | u, "Edge", 0, 99, 0, editableType::Integer);

    //Sync
    editables[574 | u] = new EditableValue(574 | u, "Input", 0, z1OscResoIn.size() - 1, 0, editableType::TextEnum, z1OscResoIn);
    editables[575 | u] = new EditableValue(575 | u, "Slave", 0, z1OscVPMCarrWave.size() - 1, 0, editableType::TextEnum, z1OscVPMCarrWave);
    editables[576 | u] = new EditableValue(576 | u, "Edge", 0, 99, 0, editableType::Integer);

    //Organ
    editables[577 | u] = new EditableValue(577 | u, "Wave1", 0, z1OscOrgWave.size() - 1, 0, editableType::TextEnum, z1OscOrgWave);
    editables[578 | u] = new EditableValue(578 | u, "Harm1", 0, 15, 0, editableType::Integer);
    editables[579 | u] = new EditableValue(579 | u, "Fine1", -99, 99, 0, editableType::Integer);
    editables[580 | u] = new EditableValue(580 | u, "Lvl1", 0, 99, 0, editableType::Integer);
    editables[583 | u] = new EditableValue(583 | u, "Perc1", 0, 99, 0, editableType::Integer);
    editables[584 | u] = new EditableValue(584 | u, "Wave2", 0, z1OscOrgWave.size() - 1, 0, editableType::TextEnum, z1OscOrgWave);
    editables[585 | u] = new EditableValue(585 | u, "Harm2", 0, 15, 0, editableType::Integer);
    editables[586 | u] = new EditableValue(586 | u, "Fine2", -99, 99, 0, editableType::Integer);
    editables[587 | u] = new EditableValue(587 | u, "Lvl2", 0, 99, 0, editableType::Integer);
    editables[590 | u] = new EditableValue(590 | u, "Perc2", 0, 99, 0, editableType::Integer);
    editables[591 | u] = new EditableValue(591 | u, "Wave3", 0, z1OscOrgWave.size() - 1, 0, editableType::TextEnum, z1OscOrgWave);
    editables[592 | u] = new EditableValue(592 | u, "Harm3", 0, 15, 0, editableType::Integer);
    editables[593 | u] = new EditableValue(593 | u, "Fine3", -99, 99, 0, editableType::Integer);
    editables[594 | u] = new EditableValue(594 | u, "Lvl3", 0, 99, 0, editableType::Integer);
    editables[597 | u] = new EditableValue(597 | u, "Perc3", 0, 99, 0, editableType::Integer);
    editables[598 | u] = new EditableValue(598 | u, "Trig", 0, z1OscOrgTrig.size() - 1, 0, editableType::TextEnum, z1OscOrgTrig);
    editables[599 | u] = new EditableValue(599 | u, "Decay", 0, 99, 0, editableType::Integer);
    //mod
    editables[581 | u] = new EditableValue(581 | u, "Level1", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
    editables[582 | u] = new EditableValue(582 | u, "int", -99, 99, 0, editableType::Integer);
    editables[588 | u] = new EditableValue(588 | u, "Level2", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
    editables[589 | u] = new EditableValue(589 | u, "int", -99, 99, 0, editableType::Integer);
    editables[595 | u] = new EditableValue(595 | u, "Level3", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
    editables[596 | u] = new EditableValue(596 | u, "int", -99, 99, 0, editableType::Integer);
    editables[600 | u] = new EditableValue(600 | u, "Perc", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
    editables[601 | u] = new EditableValue(601 | u, "int", -99, 99, 0, editableType::Integer);

    //EP
    editables[602 | u] = new EditableValue(602 | u, "Force", 0, 99, 0, editableType::Integer);
    editables[603 | u] = new EditableValue(603 | u, "VelCrv", -1, 99, 0, editableType::Integer);
    editables[604 | u] = new EditableValue(604 | u, "Width", 0, 99, 0, editableType::Integer);
    editables[605 | u] = new EditableValue(605 | u, "Click", 0, 99, 0, editableType::Integer);
    editables[606 | u] = new EditableValue(606 | u, "Decay", 0, 99, 0, editableType::Integer);
    editables[607 | u] = new EditableValue(607 | u, "Rel", 0, 99, 0, editableType::Integer);
    editables[608 | u] = new EditableValue(608 | u, "OverL", 0, 99, 0, editableType::Integer);
    editables[609 | u] = new EditableValue(609 | u, "OverF", 0, 99, 0, editableType::Integer);
    editables[610 | u] = new EditableValue(610 | u, "OverD", 0, 99, 0, editableType::Integer);
    editables[611 | u] = new EditableValue(611 | u, "Pickup", 0, 99, 0, editableType::Integer);
    editables[612 | u] = new EditableValue(612 | u, "PMod", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
    editables[613 | u] = new EditableValue(613 | u, "int", -99, 99, 0, editableType::Integer);
    editables[614 | u] = new EditableValue(614 | u, "EQ F", 0, 49, 0, editableType::Integer);
    editables[615 | u] = new EditableValue(615 | u, "EQ G", -18, 18, 0, editableType::Integer);

    if (i == 0) {
      //brass model
      editables[616 | u] = new EditableValue(616 | u, "Type", 0, z1OscBrassTypes.size() - 1, 0, editableType::TextEnum, z1OscBrassTypes);
      editables[619 | u] = new EditableValue(619 | u, "PresEG", 1, z1OscPressEG.size() - 1, 0, editableType::TextEnum, z1OscPressEG);
      editables[620 | u] = new EditableValue(620 | u, "EGInt", -99, 99, 0, editableType::Integer);
      editables[626 | u] = new EditableValue(626 | u, "LipChr", 0, 99, 0, editableType::Integer);
      editables[632 | u] = new EditableValue(632 | u, "BellT", 0, 99, 0, editableType::Integer);
      editables[633 | u] = new EditableValue(633 | u, "BellR", 0, 99, 0, editableType::Integer);
      editables[634 | u] = new EditableValue(634 | u, "Noise", 0, 99, 0, editableType::Integer);
      editables[645 | u] = new EditableValue(645 | u, "EQ F", 0, 49, 0, editableType::Integer);
      editables[646 | u] = new EditableValue(646 | u, "EQ Q", 0, 29, 0, editableType::Integer);
      editables[647 | u] = new EditableValue(647 | u, "EQ G", -18, 18, 0, editableType::Integer);
      editables[648 | u] = new EditableValue(648 | u, "Streng", 0, 99, 0, editableType::Integer);
      //mod
      editables[621 | u] = new EditableValue(621 | u, "Press1", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
      editables[622 | u] = new EditableValue(622 | u, "int", -99, 99, 0, editableType::Integer);
      editables[623 | u] = new EditableValue(623 | u, "Press2", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
      editables[624 | u] = new EditableValue(624 | u, "int", -99, 99, 0, editableType::Integer);
      editables[627 | u] = new EditableValue(627 | u, "LipChr", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
      editables[628 | u] = new EditableValue(628 | u, "int", -99, 99, 0, editableType::Integer);


      //reed model
      editables[654 | u] = new EditableValue(654 | u, "Type", 0, z1OscReedTypes.size() - 1, 0, editableType::TextEnum, z1OscReedTypes);
      editables[657 | u] = new EditableValue(657 | u, "PresEG", 1, z1OscPressEG.size() - 1, 0, editableType::TextEnum, z1OscPressEG);
      editables[658 | u] = new EditableValue(658 | u, "EGInt", -99, 99, 0, editableType::Integer);
      editables[668 | u] = new EditableValue(668 | u, "Noise", 0, 99, 0, editableType::Integer);
      editables[683 | u] = new EditableValue(683 | u, "Fc", 0, 99, 0, editableType::Integer);
      editables[684 | u] = new EditableValue(684 | u, "Reso", 0, 99, 0, editableType::Integer);
      editables[685 | u] = new EditableValue(685 | u, "EQ F", 0, 49, 0, editableType::Integer);
      editables[686 | u] = new EditableValue(686 | u, "EQ Q", 0, 29, 0, editableType::Integer);
      editables[687 | u] = new EditableValue(687 | u, "EQ G", -18, 18, 0, editableType::Integer);
      editables[689 | u] = new EditableValue(689 | u, "Offset", -99, 99, 0, editableType::Integer);
      editables[690 | u] = new EditableValue(690 | u, "Table", 0, z1OscStdWaveShapeTbl.size() - 1, 0, editableType::TextEnum, z1OscStdWaveShapeTbl);
      editables[691 | u] = new EditableValue(691 | u, "Shape", 0, 99, 0, editableType::Integer);
      //mod
      editables[659 | u] = new EditableValue(659 | u, "Press1", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
      editables[660 | u] = new EditableValue(660 | u, "int", -99, 99, 0, editableType::Integer);
      editables[661 | u] = new EditableValue(661 | u, "Press2", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
      editables[662 | u] = new EditableValue(662 | u, "int", -99, 99, 0, editableType::Integer);
      editables[681 | u] = new EditableValue(681 | u, "Reed", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
      editables[682 | u] = new EditableValue(682 | u, "int", -99, 99, 0, editableType::Integer);
      editables[692 | u] = new EditableValue(692 | u, "Shape", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
      editables[693 | u] = new EditableValue(693 | u, "int", -99, 99, 0, editableType::Integer);

      //plucked
      editables[694 | u] = new EditableValue(694 | u, "AttLvl", 0, 99, 0, editableType::Integer);
      editables[695 | u] = new EditableValue(695 | u, "ALvlVC", -99, 99, 0, editableType::Integer);
      editables[696 | u] = new EditableValue(696 | u, "AttCup", 0, 99, 0, editableType::Integer);
      editables[697 | u] = new EditableValue(697 | u, "AttCuV", -99, 99, 0, editableType::Integer);
      editables[698 | u] = new EditableValue(698 | u, "AttCdn", 0, 99, 0, editableType::Integer);
      editables[699 | u] = new EditableValue(699 | u, "AttCdV", -99, 99, 0, editableType::Integer);
      editables[700 | u] = new EditableValue(700 | u, "AttNoi", 0, 99, 0, editableType::Integer);
      editables[701 | u] = new EditableValue(701 | u, "AttNVC", -99, 99, 0, editableType::Integer);
      editables[702 | u] = new EditableValue(702 | u, "StrPos", 0, 99, 0, editableType::Integer);
      editables[705 | u] = new EditableValue(705 | u, "Disper", 0, 99, 0, editableType::Integer);
      editables[708 | u] = new EditableValue(708 | u, "Dampin", 0, 99, 0, editableType::Integer);
      editables[709 | u] = new EditableValue(709 | u, "DampKB", -99, 99, 0, editableType::Integer);
      editables[712 | u] = new EditableValue(712 | u, "Decay", 0, 99, 0, editableType::Integer);
      editables[713 | u] = new EditableValue(713 | u, "DecKB", -99, 99, 0, editableType::Integer);
      editables[714 | u] = new EditableValue(714 | u, "Releas", 0, 99, 0, editableType::Integer);
      editables[715 | u] = new EditableValue(715 | u, "HarPos", 0, 99, 0, editableType::Integer);
      editables[718 | u] = new EditableValue(718 | u, "PickupS", 0, z1OffOn.size() - 1, 0, editableType::TextEnum, z1OffOn);
      editables[719 | u] = new EditableValue(719 | u, "PupPos", 0, 99, 0, editableType::Integer);
      editables[722 | u] = new EditableValue(722 | u, "EQ F", 0, 49, 0, editableType::Integer);
      editables[723 | u] = new EditableValue(723 | u, "EQ G", -18, 18, 0, editableType::Integer);
      editables[724 | u] = new EditableValue(724 | u, "Boost", 0, 99, 0, editableType::Integer);
      //mod
      editables[703 | u] = new EditableValue(703 | u, "StrPos", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
      editables[704 | u] = new EditableValue(704 | u, "int", -99, 99, 0, editableType::Integer);
      editables[706 | u] = new EditableValue(706 | u, "Disper", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
      editables[707 | u] = new EditableValue(707 | u, "int", -99, 99, 0, editableType::Integer);
      editables[710 | u] = new EditableValue(710 | u, "Dampin", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
      editables[711 | u] = new EditableValue(711 | u, "int", -99, 99, 0, editableType::Integer);
      editables[715 | u] = new EditableValue(715 | u, "HarPos", 0, 99, 0, editableType::Integer);
      editables[716 | u] = new EditableValue(716 | u, "HarMod", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
      editables[717 | u] = new EditableValue(717 | u, "int", -99, 99, 0, editableType::Integer);
      editables[720 | u] = new EditableValue(720 | u, "PupPos", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
      editables[721 | u] = new EditableValue(721 | u, "int", -99, 99, 0, editableType::Integer);


      //bowed
      editables[725 | u] = new EditableValue(725 | u, "BSpdEG", 1, z1OscPressEG.size() - 1, 0, editableType::TextEnum, z1OscPressEG);
      editables[726 | u] = new EditableValue(726 | u, "EGInt", -99, 99, 0, editableType::Integer);
      editables[731 | u] = new EditableValue(731 | u, "BDiff", 0, z1OffOn.size() - 1, 0, editableType::TextEnum, z1OffOn);
      editables[732 | u] = new EditableValue(732 | u, "PresEG", 1, z1OscPressEG.size() - 1, 0, editableType::TextEnum, z1OscPressEG);
      editables[733 | u] = new EditableValue(733 | u, "EGInt", -99, 99, 0, editableType::Integer);
      editables[736 | u] = new EditableValue(736 | u, "Rosin", 0, 99, 0, editableType::Integer);
      editables[737 | u] = new EditableValue(737 | u, "StrPos", 0, 99, 0, editableType::Integer);
      editables[740 | u] = new EditableValue(740 | u, "Dampin", 0, 99, 0, editableType::Integer);
      editables[741 | u] = new EditableValue(741 | u, "DamTrk", 0, midiNoteNames.size() - 1, 60, editableType::TextEnum, midiNoteNames);
      editables[742 | u] = new EditableValue(742 | u, "DamTrL", -99, 99, 0, editableType::Integer);
      editables[743 | u] = new EditableValue(743 | u, "DamTrH", -99, 99, 0, editableType::Integer);
      editables[746 | u] = new EditableValue(746 | u, "Disper", 0, 99, 0, editableType::Integer);
      editables[749 | u] = new EditableValue(749 | u, "Reflec", 0, 99, 0, editableType::Integer);
      editables[752 | u] = new EditableValue(752 | u, "EQ F", 0, 49, 0, editableType::Integer);
      editables[753 | u] = new EditableValue(753 | u, "EQ Q", 0, 29, 0, editableType::Integer);
      editables[754 | u] = new EditableValue(754 | u, "EQ G", -18, 18, 0, editableType::Integer);
      //mod
      editables[727 | u] = new EditableValue(727 | u, "BSpd1", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
      editables[728 | u] = new EditableValue(728 | u, "int", -99, 99, 0, editableType::Integer);
      editables[729 | u] = new EditableValue(729 | u, "BSpd2", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
      editables[730 | u] = new EditableValue(730 | u, "int", -99, 99, 0, editableType::Integer);
      editables[734 | u] = new EditableValue(734 | u, "Press", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
      editables[735 | u] = new EditableValue(735 | u, "int", -99, 99, 0, editableType::Integer);
      editables[738 | u] = new EditableValue(738 | u, "StrPos", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
      editables[739 | u] = new EditableValue(739 | u, "int", -99, 99, 0, editableType::Integer);
      editables[744 | u] = new EditableValue(744 | u, "Dampin", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
      editables[745 | u] = new EditableValue(745 | u, "int", -99, 99, 0, editableType::Integer);
      editables[747 | u] = new EditableValue(747 | u, "Disper", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
      editables[748 | u] = new EditableValue(748 | u, "int", -99, 99, 0, editableType::Integer);
      editables[750 | u] = new EditableValue(750 | u, "Reflec", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
      editables[751 | u] = new EditableValue(751 | u, "int", -99, 99, 0, editableType::Integer);
    }
  }

  //subosc
  editables[202] = new EditableValue(202, "Oct", 0, 3, 0, editableType::Integer);
  editables[203] = new EditableValue(203, "Semi", -12, 12, 0, editableType::Integer);
  editables[204] = new EditableValue(204, "Fine", -50, 50, 0, editableType::Integer);
  editables[205] = new EditableValue(205, "Off", -100, 100, 0, editableType::Integer);
  editables[206] = new EditableValue(206, "Center", 0, 127, 0, editableType::Integer);
  editables[207] = new EditableValue(207, "LSlope", -50, 100, 0, editableType::Integer);
  editables[208] = new EditableValue(208, "HSlope", -50, 100, 0, editableType::Integer);
  editables[209] = new EditableValue(209, "Mod1", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
  editables[210] = new EditableValue(210, "Int", -99, 99, 0, editableType::Integer);
  editables[211] = new EditableValue(211, "ModInt", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
  editables[212] = new EditableValue(212, "Int", -99, 99, 0, editableType::Integer);
  editables[213] = new EditableValue(213, "Mod2", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
  editables[214] = new EditableValue(214, "int", -99, 99, 0, editableType::Integer);
  editables[215] = new EditableValue(215, "WForm", 0, z1OscVPMCarrWave.size() - 1, 0, editableType::TextEnum, z1OscVPMCarrWave);



  //noise
  editables[216] = new EditableValue(216, "Type", 0, z1NoiseFilt.size() - 1, 0, editableType::TextEnum, z1NoiseFilt);
  editables[217] = new EditableValue(217, "Trim", 0, 99, 0, editableType::Integer);
  editables[218] = new EditableValue(218, "Cutoff", 0, 99, 0, editableType::Integer);
  editables[219] = new EditableValue(219, "Mod1", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
  editables[220] = new EditableValue(220, "Int", -99, 99, 0, editableType::Integer);
  editables[221] = new EditableValue(221, "Mod2", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
  editables[222] = new EditableValue(222, "Int", -99, 99, 0, editableType::Integer);
  editables[223] = new EditableValue(223, "Reso", 0, 99, 0, editableType::Integer);
  //filt

  editables[258] = new EditableValue(258, "Route", 0, z1FiltRout.size() - 1, 0, editableType::TextEnum, z1FiltRout);
  editables[259] = new EditableValue(259, "Link", 0, z1OffOn.size() - 1, 0, editableType::TextEnum, z1OffOn);
  editables[260] = new EditableValue(260, "EGKnob", 0, z1FiltKnob.size() - 1, 0, editableType::TextEnum, z1FiltKnob);
  editables[261] = new EditableValue(261, "Filter1", 0, z1FiltType.size() - 1, 0, editableType::TextEnum, z1FiltType);
  editables[288] = new EditableValue(288, "Filter2", 0, z1FiltType.size() - 1, 0, editableType::TextEnum, z1FiltType);
  for (int j = 0; j <= 1; j++) {
    int k = 0;
    if (j == 1) k = 27;

    editables[262 + k] = new EditableValue(262 + k, "Trim", 0, 99, 0, editableType::Integer);
    editables[263 + k] = new EditableValue(263 + k, "Cutoff", 0, 99, 0, editableType::Integer);
    editables[264 + k] = new EditableValue(264 + k, "LowKey", 0, midiNoteNames.size() - 1, 0, editableType::TextEnum, midiNoteNames);
    editables[265 + k] = new EditableValue(265 + k, "HiKey", 0, midiNoteNames.size() - 1, 0, editableType::TextEnum, midiNoteNames);
    editables[266 + k] = new EditableValue(266 + k, "LoInt", -99, 99, 0, editableType::Integer);
    editables[267 + k] = new EditableValue(267 + k, "HiInt", -99, 99, 0, editableType::Integer);
    editables[268 + k] = new EditableValue(268 + k, "ModEG", 1, z1OscPressEG.size() - 1, 0, editableType::TextEnum, z1OscPressEG);
    editables[269 + k] = new EditableValue(269 + k, "Int", -99, 99, 0, editableType::Integer);
    editables[270 + k] = new EditableValue(270 + k, "CutMd1", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
    editables[271 + k] = new EditableValue(271 + k, "Int", -99, 99, 0, editableType::Integer);
    editables[272 + k] = new EditableValue(272 + k, "CutMd2", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
    editables[273 + k] = new EditableValue(273 + k, "Int", -99, 99, 0, editableType::Integer);
    editables[274 + k] = new EditableValue(274 + k, "Reso", 0, 99, 0, editableType::Integer);
    editables[275 + k] = new EditableValue(275 + k, "ResMod", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
    editables[276 + k] = new EditableValue(276 + k, "Int", -99, 99, 0, editableType::Integer);
    editables[277 + k] = new EditableValue(277 + k, "TrimB", 0, 99, 0, editableType::Integer);
    editables[278 + k] = new EditableValue(278 + k, "CutoffB", 0, 99, 0, editableType::Integer);
    editables[279 + k] = new EditableValue(279 + k, "LoKeyB", 0, midiNoteNames.size() - 1, 0, editableType::TextEnum, midiNoteNames);
    editables[280 + k] = new EditableValue(280 + k, "HiKeyB", 0, midiNoteNames.size() - 1, 0, editableType::TextEnum, midiNoteNames);
    editables[281 + k] = new EditableValue(281 + k, "LoInt", -99, 99, 0, editableType::Integer);
    editables[282 + k] = new EditableValue(282 + k, "HiInt", -99, 99, 0, editableType::Integer);
    editables[283 + k] = new EditableValue(283 + k, "EGInB", -99, 99, 0, editableType::Integer);
    editables[284 + k] = new EditableValue(284 + k, "M1IntB", -99, 99, 0, editableType::Integer);
    editables[285 + k] = new EditableValue(285 + k, "M2IntB", -99, 99, 0, editableType::Integer);
    editables[286 + k] = new EditableValue(286 + k, "ResoB", 0, 99, 0, editableType::Integer);
    editables[287 + k] = new EditableValue(287 + k, "IntB", -99, 99, 0, editableType::Integer);
  }

  //fx
  editables[356] = new EditableValue(356, "FXSend", 0, 100, 0, editableType::Integer);
  editables[357] = new EditableValue(357, "Mod", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
  editables[358] = new EditableValue(358, "int", -99, 99, 0, editableType::Integer);
  editables[359] = new EditableValue(359, "FX1", 0, z1FxTypes.size() - 1, 0, editableType::TextEnum, z1FxTypes);
  editables[360] = new EditableValue(360, "FX2", 0, 10, 0, editableType::TextEnum, z1FxTypes);
  editables[361] = new EditableValue(361, "FXMstr", 0, z1MFxTypes.size() - 1, 0, editableType::TextEnum, z1MFxTypes);
  editables[362] = new EditableValue(362, "LFreq", 0, 49, 0, editableType::Integer);
  editables[363] = new EditableValue(363, "LGain", -36, 36, 0, editableType::Integer);
  editables[364] = new EditableValue(364, "HFreq", 0, 49, 0, editableType::Integer);
  editables[365] = new EditableValue(365, "HGain", -36, 36, 0, editableType::Integer);

  //overdrive
  editables[800 | E1] = new EditableValue(800 | E1, "Mode", 0, z1OverdriveMode.size() - 1, 0, editableType::TextEnum, z1OverdriveMode);
  editables[801 | E1] = new EditableValue(801 | E1, "Drive", 0, 99, 0, editableType::Integer);
  editables[802 | E1] = new EditableValue(802 | E1, "Level", 0, 99, 0, editableType::Integer);
  editables[803 | E1] = new EditableValue(803 | E1, "LowCut", 0, 99, 0, editableType::Integer);
  editables[804 | E1] = new EditableValue(804 | E1, "LEQ F", 0, 49, 0, editableType::Integer);
  editables[805 | E1] = new EditableValue(805 | E1, "LEQ Q", 0, 95, 0, editableType::Integer);
  editables[806 | E1] = new EditableValue(806 | E1, "LEQ G", -36, 36, 0, editableType::Integer);
  editables[807 | E1] = new EditableValue(807 | E1, "MLEQ F", 0, 49, 0, editableType::Integer);
  editables[808 | E1] = new EditableValue(808 | E1, "MLEQ Q", 0, 95, 0, editableType::Integer);
  editables[809 | E1] = new EditableValue(809 | E1, "MLEQ G", -36, 36, 0, editableType::Integer);
  editables[810 | E1] = new EditableValue(810 | E1, "MHEQ F", 0, 49, 0, editableType::Integer);
  editables[811 | E1] = new EditableValue(811 | E1, "MHEQ Q", 0, 95, 0, editableType::Integer);
  editables[812 | E1] = new EditableValue(812 | E1, "MHEQ G", -36, 36, 0, editableType::Integer);
  editables[813 | E1] = new EditableValue(813 | E1, "HEQ F", 0, 49, 0, editableType::Integer);
  editables[814 | E1] = new EditableValue(814 | E1, "HEQ Q", 0, 95, 0, editableType::Integer);
  editables[815 | E1] = new EditableValue(815 | E1, "HEQ G", -36, 36, 0, editableType::Integer);
  editables[816 | E1] = new EditableValue(816 | E1, "Balans", 0, 100, 0, editableType::Integer);
  editables[817 | E1] = new EditableValue(817 | E1, "Mod", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
  editables[818 | E1] = new EditableValue(818 | E1, "int", -99, 99, 0, editableType::Integer);

  //compressor
  editables[819 | E1] = new EditableValue(819 | E1, "Sensit", 1, 99, 0, editableType::Integer);
  editables[820 | E1] = new EditableValue(820 | E1, "Attack", 1, 99, 0, editableType::Integer);
  editables[821 | E1] = new EditableValue(821 | E1, "EQTrim", 0, 99, 0, editableType::Integer);
  editables[822 | E1] = new EditableValue(822 | E1, "LEQ G", -36, 36, 0, editableType::Integer);
  editables[823 | E1] = new EditableValue(823 | E1, "HEQ G", -36, 36, 0, editableType::Integer);
  editables[824 | E1] = new EditableValue(824 | E1, "Level", 0, 99, 0, editableType::Integer);
  editables[825 | E1] = new EditableValue(825 | E1, "Balans", 0, 100, 0, editableType::Integer);
  editables[826 | E1] = new EditableValue(826 | E1, "Mod", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
  editables[827 | E1] = new EditableValue(827 | E1, "int", -99, 99, 0, editableType::Integer);

  //eq
  editables[828 | E1] = new EditableValue(828 | E1, "EQTrim", 0, 99, 0, editableType::Integer);
  editables[829 | E1] = new EditableValue(829 | E1, "LEQ T", 0, z1EQType.size() - 1, 0, editableType::TextEnum, z1EQType);
  editables[830 | E1] = new EditableValue(830 | E1, "LEQ F", 0, 49, 0, editableType::Integer);
  editables[831 | E1] = new EditableValue(831 | E1, "LEQ Q", 0, 95, 0, editableType::Integer);
  editables[832 | E1] = new EditableValue(832 | E1, "LEQ G", -36, 36, 0, editableType::Integer);
  editables[833 | E1] = new EditableValue(833 | E1, "MLEQ F", 0, 49, 0, editableType::Integer);
  editables[834 | E1] = new EditableValue(834 | E1, "MLEQ Q", 0, 95, 0, editableType::Integer);
  editables[835 | E1] = new EditableValue(835 | E1, "MLEQ G", -36, 36, 0, editableType::Integer);
  editables[836 | E1] = new EditableValue(836 | E1, "Mod", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
  editables[837 | E1] = new EditableValue(837 | E1, "int", -36, 36, 0, editableType::Integer);
  editables[838 | E1] = new EditableValue(838 | E1, "MHEQ F", 0, 49, 0, editableType::Integer);
  editables[839 | E1] = new EditableValue(839 | E1, "MHEQ Q", 0, 95, 0, editableType::Integer);
  editables[840 | E1] = new EditableValue(840 | E1, "MHEQ G", -36, 36, 0, editableType::Integer);
  editables[841 | E1] = new EditableValue(841 | E1, "HEQ T", 0, z1EQType.size() - 1, 0, editableType::TextEnum, z1EQType);
  editables[842 | E1] = new EditableValue(842 | E1, "HEQ F", 0, 49, 0, editableType::Integer);
  editables[843 | E1] = new EditableValue(843 | E1, "HEQ Q", 0, 95, 0, editableType::Integer);
  editables[844 | E1] = new EditableValue(844 | E1, "HEQ G", -36, 36, 0, editableType::Integer);
  editables[845 | E1] = new EditableValue(845 | E1, "Balans", 0, 100, 0, editableType::Integer);
  editables[846 | E1] = new EditableValue(846 | E1, "Mod", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
  editables[847 | E1] = new EditableValue(847 | E1, "int", -99, 99, 0, editableType::Integer);

  //wah
  editables[848 | E1] = new EditableValue(848 | E1, "FreqB", 0, 99, 0, editableType::Integer);
  editables[849 | E1] = new EditableValue(849 | E1, "FreqT", 0, 99, 0, editableType::Integer);
  editables[850 | E1] = new EditableValue(850 | E1, "Sweep", 0, z1WahMode.size() - 1, 0, editableType::TextEnum, z1WahMode);
  editables[851 | E1] = new EditableValue(851 | E1, "SwpSrc", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
  editables[852 | E1] = new EditableValue(852 | E1, "SwpRes", 0, 99, 0, editableType::Integer);
  editables[853 | E1] = new EditableValue(853 | E1, "EnvSens", 0, 99, 0, editableType::Integer);
  editables[854 | E1] = new EditableValue(854 | E1, "EnvShp", -99, 99, 0, editableType::Integer);
  editables[855 | E1] = new EditableValue(855 | E1, "Reso", 0, 99, 0, editableType::Integer);
  editables[856 | E1] = new EditableValue(856 | E1, "Filter", 0, z1WahFilt.size() - 1, 0, editableType::TextEnum, z1WahFilt);
  editables[857 | E1] = new EditableValue(857 | E1, "Balans", 0, 100, 0, editableType::Integer);
  editables[858 | E1] = new EditableValue(858 | E1, "Mod", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
  editables[859 | E1] = new EditableValue(859 | E1, "int", -99, 99, 0, editableType::Integer);

  //exciter
  editables[860 | E1] = new EditableValue(860 | E1, "Blend", -99, 99, 0, editableType::Integer);
  editables[861 | E1] = new EditableValue(861 | E1, "Mod", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
  editables[862 | E1] = new EditableValue(862 | E1, "int", -99, 99, 0, editableType::Integer);
  editables[863 | E1] = new EditableValue(863 | E1, "Emphat", 0, 99, 0, editableType::Integer);
  editables[864 | E1] = new EditableValue(864 | E1, "Mod", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
  editables[865 | E1] = new EditableValue(865 | E1, "int", -99, 99, 0, editableType::Integer);
  editables[866 | E1] = new EditableValue(866 | E1, "EQTrim", 0, 99, 0, editableType::Integer);
  editables[867 | E1] = new EditableValue(867 | E1, "LEQ G", -36, 36, 0, editableType::Integer);
  editables[868 | E1] = new EditableValue(868 | E1, "HEQ G", -36, 36, 0, editableType::Integer);
  editables[869 | E1] = new EditableValue(869 | E1, "Balans", 0, 100, 0, editableType::Integer);
  editables[870 | E1] = new EditableValue(870 | E1, "Mod", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
  editables[871 | E1] = new EditableValue(871 | E1, "int", -99, 99, 0, editableType::Integer);

  //decimator
  editables[872 | E1] = new EditableValue(872 | E1, "PreLFP", 0, z1OffOn.size() - 1, 0, editableType::TextEnum, z1OffOn);
  editables[873 | E1] = new EditableValue(873 | E1, "Freq", 5, 120, 5, editableType::Integer);
  editables[874 | E1] = new EditableValue(874 | E1, "Mod", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
  editables[875 | E1] = new EditableValue(875 | E1, "int", -120, 120, 0, editableType::Integer);
  editables[876 | E1] = new EditableValue(876 | E1, "Resol", 4, 24, 0, editableType::Integer);
  editables[877 | E1] = new EditableValue(877 | E1, "HDamp", 0, 99, 0, editableType::Integer);
  editables[878 | E1] = new EditableValue(878 | E1, "Level", 0, 99, 0, editableType::Integer);
  editables[879 | E1] = new EditableValue(879 | E1, "Balans", 0, 100, 0, editableType::Integer);
  editables[880 | E1] = new EditableValue(880 | E1, "Mod", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
  editables[881 | E1] = new EditableValue(881 | E1, "int", -99, 99, 0, editableType::Integer);

  //chorus
  editables[882 | E1] = new EditableValue(882 | E1, "Delay", 0, 140, 0, editableType::Integer);
  editables[883 | E1] = new EditableValue(883 | E1, "Wave", 0, z1TriSin.size() - 1, 0, editableType::TextEnum, z1TriSin);
  editables[884 | E1] = new EditableValue(884 | E1, "Freq", 1, 115, 1, editableType::Integer);
  editables[885 | E1] = new EditableValue(885 | E1, "Mod", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
  editables[886 | E1] = new EditableValue(886 | E1, "int", -115, 115, 0, editableType::Integer);
  editables[887 | E1] = new EditableValue(887 | E1, "MSync", 0, z1OffOn.size() - 1, 0, editableType::TextEnum, z1OffOn);
  editables[888 | E1] = new EditableValue(888 | E1, "MBase", 0, 7, 0, editableType::Integer);
  editables[889 | E1] = new EditableValue(889 | E1, "MTime", 0, 15, 0, editableType::Integer);
  editables[890 | E1] = new EditableValue(890 | E1, "Depth", 0, 99, 0, editableType::Integer);
  editables[891 | E1] = new EditableValue(891 | E1, "Mod", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
  editables[892 | E1] = new EditableValue(892 | E1, "int", -99, 99, 0, editableType::Integer);
  editables[893 | E1] = new EditableValue(893 | E1, "EQTrim", 0, 99, 0, editableType::Integer);
  editables[894 | E1] = new EditableValue(894 | E1, "LEQ G", -36, 36, 0, editableType::Integer);
  editables[895 | E1] = new EditableValue(895 | E1, "HEQ G", -36, 36, 0, editableType::Integer);
  editables[896 | E1] = new EditableValue(896 | E1, "Phase", 0, z1ChoPhase.size() - 1, 0, editableType::TextEnum, z1ChoPhase);
  editables[897 | E1] = new EditableValue(897 | E1, "Balans", 0, 100, 0, editableType::Integer);
  editables[898 | E1] = new EditableValue(898 | E1, "Mod", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
  editables[899 | E1] = new EditableValue(899 | E1, "int", -99, 99, 0, editableType::Integer);

  //flanger
  editables[900 | E1] = new EditableValue(900 | E1, "Delay", 0, 140, 0, editableType::Integer);
  editables[901 | E1] = new EditableValue(901 | E1, "Wave", 0, z1TriSin.size() - 1, 0, editableType::TextEnum, z1TriSin);
  editables[902 | E1] = new EditableValue(902 | E1, "Shape", -99, 99, 0, editableType::Integer);
  editables[903 | E1] = new EditableValue(903 | E1, "Freq", 1, 115, 1, editableType::Integer);
  editables[904 | E1] = new EditableValue(904 | E1, "Mod", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
  editables[905 | E1] = new EditableValue(905 | E1, "int", -115, 115, 0, editableType::Integer);
  editables[906 | E1] = new EditableValue(906 | E1, "MSync", 0, z1OffOn.size() - 1, 0, editableType::TextEnum, z1OffOn);
  editables[907 | E1] = new EditableValue(907 | E1, "MBase", 0, 7, 0, editableType::Integer);
  editables[908 | E1] = new EditableValue(908 | E1, "MTime", 0, 15, 0, editableType::Integer);
  editables[909 | E1] = new EditableValue(909 | E1, "Depth", 0, 99, 0, editableType::Integer);
  editables[910 | E1] = new EditableValue(910 | E1, "FeedB", -99, 99, 0, editableType::Integer);
  editables[911 | E1] = new EditableValue(911 | E1, "HDamp", 0, 99, 0, editableType::Integer);
  editables[912 | E1] = new EditableValue(912 | E1, "Phase", 0, z1ChoPhase.size() - 1, 0, editableType::TextEnum, z1ChoPhase);
  editables[913 | E1] = new EditableValue(913 | E1, "Balans", 0, 100, 0, editableType::Integer);
  editables[914 | E1] = new EditableValue(914 | E1, "Mod", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
  editables[915 | E1] = new EditableValue(915 | E1, "int", -99, 99, 0, editableType::Integer);

  //phaser
  editables[916 | E1] = new EditableValue(916 | E1, "Wave", 0, z1TriSin.size() - 1, 0, editableType::TextEnum, z1TriSin);
  editables[917 | E1] = new EditableValue(917 | E1, "Shape", -99, 99, 0, editableType::Integer);
  editables[918 | E1] = new EditableValue(918 | E1, "Freq", 1, 115, 1, editableType::Integer);
  editables[919 | E1] = new EditableValue(919 | E1, "Mod", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
  editables[920 | E1] = new EditableValue(920 | E1, "int", -115, 115, 0, editableType::Integer);
  editables[921 | E1] = new EditableValue(921 | E1, "MSync", 0, z1OffOn.size() - 1, 0, editableType::TextEnum, z1OffOn);
  editables[922 | E1] = new EditableValue(922 | E1, "MBase", 0, 7, 0, editableType::Integer);
  editables[923 | E1] = new EditableValue(923 | E1, "MTime", 0, 15, 0, editableType::Integer);
  editables[924 | E1] = new EditableValue(924 | E1, "Manual", 0, 99, 0, editableType::Integer);
  editables[925 | E1] = new EditableValue(925 | E1, "Depth", 0, 99, 0, editableType::Integer);
  editables[926 | E1] = new EditableValue(926 | E1, "Reso", -99, 99, 0, editableType::Integer);
  editables[927 | E1] = new EditableValue(927 | E1, "HDamp", 0, 99, 0, editableType::Integer);
  editables[928 | E1] = new EditableValue(928 | E1, "Phase", 0, z1ChoPhase.size() - 1, 0, editableType::TextEnum, z1ChoPhase);
  editables[929 | E1] = new EditableValue(929 | E1, "Balans", 0, 100, 0, editableType::Integer);
  editables[930 | E1] = new EditableValue(930 | E1, "Mod", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
  editables[931 | E1] = new EditableValue(931 | E1, "int", -99, 99, 0, editableType::Integer);

  //small rotary
  editables[932 | E1] = new EditableValue(932 | E1, "Speed", 0, z1SlowFast.size() - 1, 0, editableType::TextEnum, z1SlowFast);
  editables[933 | E1] = new EditableValue(933 | E1, "SpdSW", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
  editables[934 | E1] = new EditableValue(934 | E1, "HornAc", 0, 99, 0, editableType::Integer);
  editables[935 | E1] = new EditableValue(935 | E1, "HornRt", 0, 99, 0, editableType::Integer);
  editables[936 | E1] = new EditableValue(936 | E1, "MicDst", 0, 99, 0, editableType::Integer);
  editables[937 | E1] = new EditableValue(937 | E1, "Hrn/Rt", 0, 99, 0, editableType::Integer);
  editables[938 | E1] = new EditableValue(938 | E1, "Balans", 0, 100, 0, editableType::Integer);
  editables[939 | E1] = new EditableValue(939 | E1, "Mod", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
  editables[940 | E1] = new EditableValue(940 | E1, "int", -99, 99, 0, editableType::Integer);

  //Delay
  editables[941 | E1] = new EditableValue(941 | E1, "ILevel", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
  editables[942 | E1] = new EditableValue(942 | E1, "int", -99, 99, 0, editableType::Integer);
  editables[943 | E1] = new EditableValue(943 | E1, "Time", 0, 251, 0, editableType::Integer);
  editables[944 | E1] = new EditableValue(944 | E1, "FeedB", -99, 99, 0, editableType::Integer);
  editables[945 | E1] = new EditableValue(945 | E1, "Mod", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
  editables[946 | E1] = new EditableValue(946 | E1, "int", -99, 99, 0, editableType::Integer);
  editables[947 | E1] = new EditableValue(947 | E1, "LDamp", 0, 99, 0, editableType::Integer);
  editables[948 | E1] = new EditableValue(948 | E1, "HDamp", 0, 99, 0, editableType::Integer);
  editables[949 | E1] = new EditableValue(949 | E1, "Balans", 0, 100, 0, editableType::Integer);
  editables[950 | E1] = new EditableValue(950 | E1, "Mod", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
  editables[951 | E1] = new EditableValue(951 | E1, "int", -99, 99, 0, editableType::Integer);


  //TalkingMod
  editables[952 | E1] = new EditableValue(952 | E1, "Control", 0, z1ControlMode.size() - 1, 0, editableType::TextEnum, z1ControlMode);
  editables[953 | E1] = new EditableValue(953 | E1, "Manual", 0, 99, 0, editableType::Integer);
  editables[954 | E1] = new EditableValue(954 | E1, "Src", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
  editables[955 | E1] = new EditableValue(955 | E1, "Top", 0, z1AIUEO.size() - 1, 0, editableType::TextEnum, z1AIUEO);
  editables[956 | E1] = new EditableValue(956 | E1, "Center", 0, z1AIUEO.size() - 1, 0, editableType::TextEnum, z1AIUEO);
  editables[957 | E1] = new EditableValue(957 | E1, "Bottom", 0, z1AIUEO.size() - 1, 0, editableType::TextEnum, z1AIUEO);
  editables[958 | E1] = new EditableValue(958 | E1, "Formsh", -99, 99, 0, editableType::Integer);
  editables[959 | E1] = new EditableValue(959 | E1, "Reso", 0, 99, 0, editableType::Integer);
  editables[960 | E1] = new EditableValue(960 | E1, "Balans", 0, 100, 0, editableType::Integer);
  editables[961 | E1] = new EditableValue(961 | E1, "Mod", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
  editables[962 | E1] = new EditableValue(962 | E1, "int", -99, 99, 0, editableType::Integer);

  //Multitap
  editables[963 | E1] = new EditableValue(963 | E1, "Type", 0, z1MTapType.size() - 1, 0, editableType::TextEnum, z1MTapType);
  editables[964 | E1] = new EditableValue(964 | E1, "ILevel", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
  editables[965 | E1] = new EditableValue(965 | E1, "int", -99, 99, 0, editableType::Integer);
  editables[966 | E1] = new EditableValue(966 | E1, "Tap1T", 0, 251, 0, editableType::Integer);
  editables[967 | E1] = new EditableValue(967 | E1, "Tap1Lv", 0, 99, 0, editableType::Integer);
  editables[968 | E1] = new EditableValue(968 | E1, "Tap2T", 0, 251, 0, editableType::Integer);
  editables[969 | E1] = new EditableValue(969 | E1, "FeedB", -99, 99, 0, editableType::Integer);
  editables[970 | E1] = new EditableValue(970 | E1, "Mod", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
  editables[971 | E1] = new EditableValue(971 | E1, "int", -99, 99, 0, editableType::Integer);
  editables[972 | E1] = new EditableValue(972 | E1, "LDamp", 0, 99, 0, editableType::Integer);
  editables[973 | E1] = new EditableValue(973 | E1, "HDamp", 0, 99, 0, editableType::Integer);
  editables[974 | E1] = new EditableValue(974 | E1, "Spread", -99, 99, 0, editableType::Integer);
  editables[975 | E1] = new EditableValue(975 | E1, "Mod", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
  editables[976 | E1] = new EditableValue(976 | E1, "int", -99, 99, 0, editableType::Integer);
  editables[977 | E1] = new EditableValue(977 | E1, "Balans", 0, 100, 0, editableType::Integer);
  editables[978 | E1] = new EditableValue(978 | E1, "Mod", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
  editables[979 | E1] = new EditableValue(979 | E1, "int", -99, 99, 0, editableType::Integer);

  //ensemble
  editables[980 | E1] = new EditableValue(980 | E1, "Speed", 0, 99, 0, editableType::Integer);
  editables[981 | E1] = new EditableValue(981 | E1, "Mod", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
  editables[982 | E1] = new EditableValue(982 | E1, "int", -99, 99, 0, editableType::Integer);
  editables[983 | E1] = new EditableValue(983 | E1, "Shimmer", 0, 99, 0, editableType::Integer);
  editables[984 | E1] = new EditableValue(984 | E1, "Depth", 0, 99, 0, editableType::Integer);
  editables[985 | E1] = new EditableValue(985 | E1, "Mod", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
  editables[986 | E1] = new EditableValue(986 | E1, "int", -99, 99, 0, editableType::Integer);
  editables[987 | E1] = new EditableValue(987 | E1, "Balans", 0, 100, 0, editableType::Integer);
  editables[988 | E1] = new EditableValue(988 | E1, "Mod", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
  editables[989 | E1] = new EditableValue(989 | E1, "int", -99, 99, 0, editableType::Integer);


  //rotary
  editables[990 | E1] = new EditableValue(990 | E1, "Speed", 0, z1SlowFast.size() - 1, 0, editableType::TextEnum, z1SlowFast);
  editables[991 | E1] = new EditableValue(991 | E1, "SpdSW", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
  editables[992 | E1] = new EditableValue(992 | E1, "Mode", 0, z1RotaryMode.size() - 1, 0, editableType::TextEnum, z1RotaryMode);
  editables[993 | E1] = new EditableValue(993 | E1, "ModeSW", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
  editables[994 | E1] = new EditableValue(994 | E1, "RotAc", 0, 99, 0, editableType::Integer);
  editables[995 | E1] = new EditableValue(995 | E1, "RotRt", 0, 99, 0, editableType::Integer);
  editables[996 | E1] = new EditableValue(996 | E1, "HornAc", 0, 99, 0, editableType::Integer);
  editables[997 | E1] = new EditableValue(997 | E1, "HornRt", 0, 99, 0, editableType::Integer);
  editables[998 | E1] = new EditableValue(998 | E1, "MicDst", 0, 99, 0, editableType::Integer);
  editables[999 | E1] = new EditableValue(999 | E1, "MicSpr", 0, 99, 0, editableType::Integer);
  editables[1000 | E1] = new EditableValue(1000 | E1, "Hrn/Rt", 0, 99, 0, editableType::Integer);
  editables[1001 | E1] = new EditableValue(1001 | E1, "Balans", 0, 100, 0, editableType::Integer);
  editables[1002 | E1] = new EditableValue(1002 | E1, "Mod", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
  editables[1003 | E1] = new EditableValue(1003 | E1, "int", -99, 99, 0, editableType::Integer);


  //overdrive
  editables[800 | E2] = new EditableValue(800 | E2, "Mode", 0, z1OverdriveMode.size() - 1, 0, editableType::TextEnum, z1OverdriveMode);
  editables[801 | E2] = new EditableValue(801 | E2, "Drive", 0, 99, 0, editableType::Integer);
  editables[802 | E2] = new EditableValue(802 | E2, "Level", 0, 99, 0, editableType::Integer);
  editables[803 | E2] = new EditableValue(803 | E2, "LowCut", 0, 99, 0, editableType::Integer);
  editables[804 | E2] = new EditableValue(804 | E2, "LEQ F", 0, 49, 0, editableType::Integer);
  editables[805 | E2] = new EditableValue(805 | E2, "LEQ Q", 0, 95, 0, editableType::Integer);
  editables[806 | E2] = new EditableValue(806 | E2, "LEQ G", -36, 36, 0, editableType::Integer);
  editables[807 | E2] = new EditableValue(807 | E2, "MLEQ F", 0, 49, 0, editableType::Integer);
  editables[808 | E2] = new EditableValue(808 | E2, "MLEQ Q", 0, 95, 0, editableType::Integer);
  editables[809 | E2] = new EditableValue(809 | E2, "MLEQ G", -36, 36, 0, editableType::Integer);
  editables[810 | E2] = new EditableValue(810 | E2, "MHEQ F", 0, 49, 0, editableType::Integer);
  editables[811 | E2] = new EditableValue(811 | E2, "MHEQ Q", 0, 95, 0, editableType::Integer);
  editables[812 | E2] = new EditableValue(812 | E2, "MHEQ G", -36, 36, 0, editableType::Integer);
  editables[813 | E2] = new EditableValue(813 | E2, "HEQ F", 0, 49, 0, editableType::Integer);
  editables[814 | E2] = new EditableValue(814 | E2, "HEQ Q", 0, 95, 0, editableType::Integer);
  editables[815 | E2] = new EditableValue(815 | E2, "HEQ G", -36, 36, 0, editableType::Integer);
  editables[816 | E2] = new EditableValue(816 | E2, "Balans", 0, 100, 0, editableType::Integer);
  editables[817 | E2] = new EditableValue(817 | E2, "Mod", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
  editables[818 | E2] = new EditableValue(818 | E2, "int", -99, 99, 0, editableType::Integer);

  //compressor
  editables[819 | E2] = new EditableValue(819 | E2, "Sensit", 1, 99, 0, editableType::Integer);
  editables[820 | E2] = new EditableValue(820 | E2, "Attack", 1, 99, 0, editableType::Integer);
  editables[821 | E2] = new EditableValue(821 | E2, "EQTrim", 0, 99, 0, editableType::Integer);
  editables[822 | E2] = new EditableValue(822 | E2, "LEQ G", -36, 36, 0, editableType::Integer);
  editables[823 | E2] = new EditableValue(823 | E2, "HEQ G", -36, 36, 0, editableType::Integer);
  editables[824 | E2] = new EditableValue(824 | E2, "Level", 0, 99, 0, editableType::Integer);
  editables[825 | E2] = new EditableValue(825 | E2, "Balans", 0, 100, 0, editableType::Integer);
  editables[826 | E2] = new EditableValue(826 | E2, "Mod", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
  editables[827 | E2] = new EditableValue(827 | E2, "int", -99, 99, 0, editableType::Integer);

  //eq
  editables[828 | E2] = new EditableValue(828 | E2, "EQTrim", 0, 99, 0, editableType::Integer);
  editables[829 | E2] = new EditableValue(829 | E2, "LEQ T", 0, z1EQType.size() - 1, 0, editableType::TextEnum, z1EQType);
  editables[830 | E2] = new EditableValue(830 | E2, "LEQ F", 0, 49, 0, editableType::Integer);
  editables[831 | E2] = new EditableValue(831 | E2, "LEQ Q", 0, 95, 0, editableType::Integer);
  editables[832 | E2] = new EditableValue(832 | E2, "LEQ G", -36, 36, 0, editableType::Integer);
  editables[833 | E2] = new EditableValue(833 | E2, "MLEQ F", 0, 49, 0, editableType::Integer);
  editables[834 | E2] = new EditableValue(834 | E2, "MLEQ Q", 0, 95, 0, editableType::Integer);
  editables[835 | E2] = new EditableValue(835 | E2, "MLEQ G", -36, 36, 0, editableType::Integer);
  editables[836 | E2] = new EditableValue(836 | E2, "Mod", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
  editables[837 | E2] = new EditableValue(837 | E2, "int", -36, 36, 0, editableType::Integer);
  editables[838 | E2] = new EditableValue(838 | E2, "MHEQ F", 0, 49, 0, editableType::Integer);
  editables[839 | E2] = new EditableValue(839 | E2, "MHEQ Q", 0, 95, 0, editableType::Integer);
  editables[840 | E2] = new EditableValue(840 | E2, "MHEQ G", -36, 36, 0, editableType::Integer);
  editables[841 | E2] = new EditableValue(841 | E2, "HEQ T", 0, z1EQType.size() - 1, 0, editableType::TextEnum, z1EQType);
  editables[842 | E2] = new EditableValue(842 | E2, "HEQ F", 0, 49, 0, editableType::Integer);
  editables[843 | E2] = new EditableValue(843 | E2, "HEQ Q", 0, 95, 0, editableType::Integer);
  editables[844 | E2] = new EditableValue(844 | E2, "HEQ G", -36, 36, 0, editableType::Integer);
  editables[845 | E2] = new EditableValue(845 | E2, "Balans", 0, 100, 0, editableType::Integer);
  editables[846 | E2] = new EditableValue(846 | E2, "Mod", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
  editables[847 | E2] = new EditableValue(847 | E2, "int", -99, 99, 0, editableType::Integer);

  //wah
  editables[848 | E2] = new EditableValue(848 | E2, "FreqB", 0, 99, 0, editableType::Integer);
  editables[849 | E2] = new EditableValue(849 | E2, "FreqT", 0, 99, 0, editableType::Integer);
  editables[850 | E2] = new EditableValue(850 | E2, "Sweep", 0, z1WahMode.size() - 1, 0, editableType::TextEnum, z1WahMode);
  editables[851 | E2] = new EditableValue(851 | E2, "SwpSrc", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
  editables[852 | E2] = new EditableValue(852 | E2, "SwpRes", 0, 99, 0, editableType::Integer);
  editables[853 | E2] = new EditableValue(853 | E2, "EnvSens", 0, 99, 0, editableType::Integer);
  editables[854 | E2] = new EditableValue(854 | E2, "EnvShp", -99, 99, 0, editableType::Integer);
  editables[855 | E2] = new EditableValue(855 | E2, "Reso", 0, 99, 0, editableType::Integer);
  editables[856 | E2] = new EditableValue(856 | E2, "Filter", 0, z1WahFilt.size() - 1, 0, editableType::TextEnum, z1WahFilt);
  editables[857 | E2] = new EditableValue(857 | E2, "Balans", 0, 100, 0, editableType::Integer);
  editables[858 | E2] = new EditableValue(858 | E2, "Mod", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
  editables[859 | E2] = new EditableValue(859 | E2, "int", -99, 99, 0, editableType::Integer);

  //exciter
  editables[860 | E2] = new EditableValue(860 | E2, "Blend", -99, 99, 0, editableType::Integer);
  editables[861 | E2] = new EditableValue(861 | E2, "Mod", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
  editables[862 | E2] = new EditableValue(862 | E2, "int", -99, 99, 0, editableType::Integer);
  editables[863 | E2] = new EditableValue(863 | E2, "Emphat", 0, 99, 0, editableType::Integer);
  editables[864 | E2] = new EditableValue(864 | E2, "Mod", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
  editables[865 | E2] = new EditableValue(865 | E2, "int", -99, 99, 0, editableType::Integer);
  editables[866 | E2] = new EditableValue(866 | E2, "EQTrim", 0, 99, 0, editableType::Integer);
  editables[867 | E2] = new EditableValue(867 | E2, "LEQ G", -36, 36, 0, editableType::Integer);
  editables[868 | E2] = new EditableValue(868 | E2, "HEQ G", -36, 36, 0, editableType::Integer);
  editables[869 | E2] = new EditableValue(869 | E2, "Balans", 0, 100, 0, editableType::Integer);
  editables[870 | E2] = new EditableValue(870 | E2, "Mod", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
  editables[871 | E2] = new EditableValue(871 | E2, "int", -99, 99, 0, editableType::Integer);

  //decimator
  editables[872 | E2] = new EditableValue(872 | E2, "PreLFP", 0, z1OffOn.size() - 1, 0, editableType::TextEnum, z1OffOn);
  editables[873 | E2] = new EditableValue(873 | E2, "Freq", 5, 120, 5, editableType::Integer);
  editables[874 | E2] = new EditableValue(874 | E2, "Mod", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
  editables[875 | E2] = new EditableValue(875 | E2, "int", -120, 120, 0, editableType::Integer);
  editables[876 | E2] = new EditableValue(876 | E2, "Resol", 4, 24, 0, editableType::Integer);
  editables[877 | E2] = new EditableValue(877 | E2, "HDamp", 0, 99, 0, editableType::Integer);
  editables[878 | E2] = new EditableValue(878 | E2, "Level", 0, 99, 0, editableType::Integer);
  editables[879 | E2] = new EditableValue(879 | E2, "Balans", 0, 100, 0, editableType::Integer);
  editables[880 | E2] = new EditableValue(880 | E2, "Mod", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
  editables[881 | E2] = new EditableValue(881 | E2, "int", -99, 99, 0, editableType::Integer);

  //chorus
  editables[882 | E2] = new EditableValue(882 | E2, "Delay", 0, 140, 0, editableType::Integer);
  editables[883 | E2] = new EditableValue(883 | E2, "Wave", 0, z1TriSin.size() - 1, 0, editableType::TextEnum, z1TriSin);
  editables[884 | E2] = new EditableValue(884 | E2, "Freq", 1, 115, 1, editableType::Integer);
  editables[885 | E2] = new EditableValue(885 | E2, "Mod", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
  editables[886 | E2] = new EditableValue(886 | E2, "int", -115, 115, 0, editableType::Integer);
  editables[887 | E2] = new EditableValue(887 | E2, "MSync", 0, z1OffOn.size() - 1, 0, editableType::TextEnum, z1OffOn);
  editables[888 | E2] = new EditableValue(888 | E2, "MBase", 0, 7, 0, editableType::Integer);
  editables[889 | E2] = new EditableValue(889 | E2, "MTime", 0, 15, 0, editableType::Integer);
  editables[890 | E2] = new EditableValue(890 | E2, "Depth", 0, 99, 0, editableType::Integer);
  editables[891 | E2] = new EditableValue(891 | E2, "Mod", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
  editables[892 | E2] = new EditableValue(892 | E2, "int", -99, 99, 0, editableType::Integer);
  editables[893 | E2] = new EditableValue(893 | E2, "EQTrim", 0, 99, 0, editableType::Integer);
  editables[894 | E2] = new EditableValue(894 | E2, "LEQ G", -36, 36, 0, editableType::Integer);
  editables[895 | E2] = new EditableValue(895 | E2, "HEQ G", -36, 36, 0, editableType::Integer);
  editables[896 | E2] = new EditableValue(896 | E2, "Phase", 0, z1ChoPhase.size() - 1, 0, editableType::TextEnum, z1ChoPhase);
  editables[897 | E2] = new EditableValue(897 | E2, "Balans", 0, 100, 0, editableType::Integer);
  editables[898 | E2] = new EditableValue(898 | E2, "Mod", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
  editables[899 | E2] = new EditableValue(899 | E2, "int", -99, 99, 0, editableType::Integer);

  //flanger
  editables[900 | E2] = new EditableValue(900 | E2, "Delay", 0, 140, 0, editableType::Integer);
  editables[901 | E2] = new EditableValue(901 | E2, "Wave", 0, z1TriSin.size() - 1, 0, editableType::TextEnum, z1TriSin);
  editables[902 | E2] = new EditableValue(902 | E2, "Shape", -99, 99, 0, editableType::Integer);
  editables[903 | E2] = new EditableValue(903 | E2, "Freq", 1, 115, 1, editableType::Integer);
  editables[904 | E2] = new EditableValue(904 | E2, "Mod", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
  editables[905 | E2] = new EditableValue(905 | E2, "int", -115, 115, 0, editableType::Integer);
  editables[906 | E2] = new EditableValue(906 | E2, "MSync", 0, z1OffOn.size() - 1, 0, editableType::TextEnum, z1OffOn);
  editables[907 | E2] = new EditableValue(907 | E2, "MBase", 0, 7, 0, editableType::Integer);
  editables[908 | E2] = new EditableValue(908 | E2, "MTime", 0, 15, 0, editableType::Integer);
  editables[909 | E2] = new EditableValue(909 | E2, "Depth", 0, 99, 0, editableType::Integer);
  editables[910 | E2] = new EditableValue(910 | E2, "FeedB", -99, 99, 0, editableType::Integer);
  editables[911 | E2] = new EditableValue(911 | E2, "HDamp", 0, 99, 0, editableType::Integer);
  editables[912 | E2] = new EditableValue(912 | E2, "Phase", 0, z1ChoPhase.size() - 1, 0, editableType::TextEnum, z1ChoPhase);
  editables[913 | E2] = new EditableValue(913 | E2, "Balans", 0, 100, 0, editableType::Integer);
  editables[914 | E2] = new EditableValue(914 | E2, "Mod", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
  editables[915 | E2] = new EditableValue(915 | E2, "int", -99, 99, 0, editableType::Integer);

  //phaser
  editables[916 | E2] = new EditableValue(916 | E2, "Wave", 0, z1TriSin.size() - 1, 0, editableType::TextEnum, z1TriSin);
  editables[917 | E2] = new EditableValue(917 | E2, "Shape", -99, 99, 0, editableType::Integer);
  editables[918 | E2] = new EditableValue(918 | E2, "Freq", 1, 115, 1, editableType::Integer);
  editables[919 | E2] = new EditableValue(919 | E2, "Mod", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
  editables[920 | E2] = new EditableValue(920 | E2, "int", -115, 115, 0, editableType::Integer);
  editables[921 | E2] = new EditableValue(921 | E2, "MSync", 0, z1OffOn.size() - 1, 0, editableType::TextEnum, z1OffOn);
  editables[922 | E2] = new EditableValue(922 | E2, "MBase", 0, 7, 0, editableType::Integer);
  editables[923 | E2] = new EditableValue(923 | E2, "MTime", 0, 15, 0, editableType::Integer);
  editables[924 | E2] = new EditableValue(924 | E2, "Manual", 0, 99, 0, editableType::Integer);
  editables[925 | E2] = new EditableValue(925 | E2, "Depth", 0, 99, 0, editableType::Integer);
  editables[926 | E2] = new EditableValue(926 | E2, "Reso", -99, 99, 0, editableType::Integer);
  editables[927 | E2] = new EditableValue(927 | E2, "HDamp", 0, 99, 0, editableType::Integer);
  editables[928 | E2] = new EditableValue(928 | E2, "Phase", 0, z1ChoPhase.size() - 1, 0, editableType::TextEnum, z1ChoPhase);
  editables[929 | E2] = new EditableValue(929 | E2, "Balans", 0, 100, 0, editableType::Integer);
  editables[930 | E2] = new EditableValue(930 | E2, "Mod", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
  editables[931 | E2] = new EditableValue(931 | E2, "int", -99, 99, 0, editableType::Integer);

  //small rotary
  editables[932 | E2] = new EditableValue(932 | E2, "Speed", 0, z1SlowFast.size() - 1, 0, editableType::TextEnum, z1SlowFast);
  editables[933 | E2] = new EditableValue(933 | E2, "SpdSW", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
  editables[934 | E2] = new EditableValue(934 | E2, "HornAc", 0, 99, 0, editableType::Integer);
  editables[935 | E2] = new EditableValue(935 | E2, "HornRt", 0, 99, 0, editableType::Integer);
  editables[936 | E2] = new EditableValue(936 | E2, "MicDst", 0, 99, 0, editableType::Integer);
  editables[937 | E2] = new EditableValue(937 | E2, "Hrn/Rt", 0, 99, 0, editableType::Integer);
  editables[938 | E2] = new EditableValue(938 | E2, "Balans", 0, 100, 0, editableType::Integer);
  editables[939 | E2] = new EditableValue(939 | E2, "Mod", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
  editables[940 | E2] = new EditableValue(940 | E2, "int", -99, 99, 0, editableType::Integer);

  //Delay
  editables[941 | E2] = new EditableValue(941 | E2, "ILevel", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
  editables[942 | E2] = new EditableValue(942 | E2, "int", -99, 99, 0, editableType::Integer);
  editables[943 | E2] = new EditableValue(943 | E2, "Time", 0, 251, 0, editableType::Integer);
  editables[944 | E2] = new EditableValue(944 | E2, "FeedB", -99, 99, 0, editableType::Integer);
  editables[945 | E2] = new EditableValue(945 | E2, "Mod", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
  editables[946 | E2] = new EditableValue(946 | E2, "int", -99, 99, 0, editableType::Integer);
  editables[947 | E2] = new EditableValue(947 | E2, "LDamp", 0, 99, 0, editableType::Integer);
  editables[948 | E2] = new EditableValue(948 | E2, "HDamp", 0, 99, 0, editableType::Integer);
  editables[949 | E2] = new EditableValue(949 | E2, "Balans", 0, 100, 0, editableType::Integer);
  editables[950 | E2] = new EditableValue(950 | E2, "Mod", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
  editables[951 | E2] = new EditableValue(951 | E2, "int", -99, 99, 0, editableType::Integer);



  //stereo delay
  editables[1004 | EGL] = new EditableValue(1004 | EGL, "Type", 0, z1DelayTypes.size() - 1, 0, editableType::TextEnum, z1DelayTypes);
  editables[1005 | EGL] = new EditableValue(1005 | EGL, "ILevel", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
  editables[1006 | EGL] = new EditableValue(1006 | EGL, "int", -99, 99, 0, editableType::Integer);
  editables[1007 | EGL] = new EditableValue(1007 | EGL, "LeftT", 0, 251, 0, editableType::Integer);
  editables[1008 | EGL] = new EditableValue(1008 | EGL, "RightT", 0, 251, 0, editableType::Integer);
  editables[1009 | EGL] = new EditableValue(1009 | EGL, "FeedBL", -99, 99, 0, editableType::Integer);
  editables[1010 | EGL] = new EditableValue(1010 | EGL, "FeedBR", -99, 99, 0, editableType::Integer);
  editables[1011 | EGL] = new EditableValue(1011 | EGL, "FBMod", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
  editables[1012 | EGL] = new EditableValue(1012 | EGL, "int", -99, 99, 0, editableType::Integer);
  editables[1013 | EGL] = new EditableValue(1013 | EGL, "LDamp", 0, 99, 0, editableType::Integer);
  editables[1014 | EGL] = new EditableValue(1014 | EGL, "HDamp", 0, 99, 0, editableType::Integer);
  editables[1015 | EGL] = new EditableValue(1015 | EGL, "Balans", 0, 100, 0, editableType::Integer);
  editables[1016 | EGL] = new EditableValue(1016 | EGL, "Mod", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
  editables[1017 | EGL] = new EditableValue(1017 | EGL, "int", -99, 99, 0, editableType::Integer);

  //hall
  editables[1018 | EGL] = new EditableValue(1018 | EGL, "Time", 1, 100, 0, editableType::Integer);
  editables[1019 | EGL] = new EditableValue(1019 | EGL, "PreDly", 0, 200, 0, editableType::Integer);
  editables[1020 | EGL] = new EditableValue(1020 | EGL, "PrThru", 0, 99, 0, editableType::Integer);
  editables[1021 | EGL] = new EditableValue(1021 | EGL, "HDamp", 0, 99, 0, editableType::Integer);
  editables[1022 | EGL] = new EditableValue(1022 | EGL, "EQTrim", 0, 99, 0, editableType::Integer);
  editables[1023 | EGL] = new EditableValue(1023 | EGL, "LEQ G", -36, 36, 0, editableType::Integer);
  editables[1024 | EGL] = new EditableValue(1024 | EGL, "HEQ G", -36, 36, 0, editableType::Integer);
  editables[1025 | EGL] = new EditableValue(1025 | EGL, "Balans", 0, 100, 0, editableType::Integer);
  editables[1026 | EGL] = new EditableValue(1026 | EGL, "Mod", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
  editables[1027 | EGL] = new EditableValue(1027 | EGL, "int", -99, 99, 0, editableType::Integer);

  //room
  editables[1028 | EGL] = new EditableValue(1028 | EGL, "Time", 1, 30, 0, editableType::Integer);
  editables[1029 | EGL] = new EditableValue(1029 | EGL, "PreDly", 0, 200, 0, editableType::Integer);
  editables[1030 | EGL] = new EditableValue(1030 | EGL, "PrThru", 0, 99, 0, editableType::Integer);
  editables[1031 | EGL] = new EditableValue(1031 | EGL, "HDamp", 0, 99, 0, editableType::Integer);
  editables[1032 | EGL] = new EditableValue(1032 | EGL, "ERLvl", 0, 99, 0, editableType::Integer);
  editables[1033 | EGL] = new EditableValue(1033 | EGL, "Level", 0, 99, 0, editableType::Integer);
  editables[1034 | EGL] = new EditableValue(1034 | EGL, "EQTrim", 0, 99, 0, editableType::Integer);
  editables[1035 | EGL] = new EditableValue(1035 | EGL, "LEQ G", -36, 36, 0, editableType::Integer);
  editables[1036 | EGL] = new EditableValue(1036 | EGL, "HEQ G", -36, 36, 0, editableType::Integer);
  editables[1037 | EGL] = new EditableValue(1037 | EGL, "Balans", 0, 100, 0, editableType::Integer);
  editables[1038 | EGL] = new EditableValue(1038 | EGL, "Mod", 0, z1ModSrc1.size() - 1, 0, editableType::TextEnum, z1ModSrc1);
  editables[1039 | EGL] = new EditableValue(1039 | EGL, "int", -99, 99, 0, editableType::Integer);

  editables[10001] = new EditableValue(10001, "C74To", 0, 127, 7, editableType::Integer);

  //
  editables[174]->subscribeDefault([&](int key, editableType t, int iVal, float fVal, std::string sVal) {
    if(key<10000)sendZ1ProgParm(0, 1, key, iVal);
  });


  mstate = mainState::NORMAL;
  ustate = unisonState::OFF;
  mpeUstate = unisonState::OFF;
  s->ledsOff();
  s->setLed(0, true);
  Serial.println("init structs done");
  setEditState(editState::OSC1);
  Serial.println("init structs done");
  showOSC1();
  showOSC1Std();
}


void Z1Edit::setMod(bool v) {
  if (estate != editState::LFO) {
    mod = v;
    mod ? s->setLed(11, true) : s->setLed(11, false);
    if (currPageFunc) (this->*Z1Edit::currPageFunc)();
  }
}

void Z1Edit::toggleMod() {
  setMod(!mod);
}

void Z1Edit::setEditState(editState state) {
  estate = state;
  s->setLed(3, false);
  s->setLed(4, false);
  s->setLed(5, false);
  s->setLed(9, false);
  s->setLed(10, false);
  s->setLed(12, false);
  switch (state) {
    case editState::OSC1:
    case editState::OSC1MOD:
      s->setLed(3, true);
      break;
    case editState::OSC2:
    case editState::OSC2MOD:
      s->setLed(4, true);
      break;
    case editState::SUBOSC:
    case editState::SUBOSCMOD:
      s->setLed(3, true);
      s->setLed(4, true);
      break;
    case editState::FILT:
    case editState::FILTMOD:
      s->setLed(5, true);
      break;
    case editState::FX:
    case editState::FXMOD:
      s->setLed(9, true);
      break;
    case editState::EG:
    case editState::EGMOD:
      s->setLed(10, true);
      break;
    case editState::LFO:
    case editState::LFOMOD:
      s->setLed(12, true);
      break;
    default:
      break;
  }
}

// The main mode: Normal -> MPE -> MPE+ -> Normal with the button, or straight
// to a mode with gotoMainMode() (the Buddy sysex from the computer).
void Z1Edit::enterMPE() {
  if (mstate != mainState::NORMAL) return;
  mstate = mainState::MPEPRE;
  s->setLed(0, false);
  s->setLed(1, true);
  s->setLed(2, false);
  Serial.println("enterMPE(): MPEPRE");
  storeForMPE();
}

void Z1Edit::leaveMPE() {
  if (mstate == mainState::NORMAL) return;
  bool wasPlaying = inMPE();
  bool inMulti = (mstate != mainState::MPEPRE);   // the Z1 was told to go to Multi mode
  mstate = mainState::NORMAL;
  if (wasPlaying) {
    // Leave the timbres as they were found: no stacking, no detune.
    mpeUstate = unisonState::OFF;
    mpeUnisonChanged(1);
  }
  if (inMulti) setZ1Mode(false);                  // back to Program mode
  showUnison();
  s->setLed(0, true);
  s->setLed(1, false);
  s->setLed(2, false);
  Serial.println("leaveMPE(): Normal");
}

// The main mode button: Normal <-> MPE. LED 0 is Normal, LED 1 is MPE.
// (There was a third step, MPE+, meant for the Osmose; it is parked. The
// state still exists but nothing leads to it.)
void Z1Edit::nextMainMode() {
  if (mstate == mainState::NORMAL) enterMPE();
  else leaveMPE();                 // from MPE, or on the way in when the Z1 does not answer
}

void Z1Edit::gotoMainMode(int mode) {
  if (mode == 0) leaveMPE();
  else enterMPE();                 // does nothing unless the mode is Normal
}

int Z1Edit::unisonShown() {
  return (int)(inMPE() ? mpeUstate : ustate);
}

void Z1Edit::setUnisonLevel(int level) {
  if (level < 0 || level > 3) return;
  if (inMPE()) {
    mpeUstate = (unisonState)level;
    showUnison();
    mpeUnisonChanged(mpeVoices());
  } else if (mstate == mainState::NORMAL) {
    setUnison((unisonState)level);
  }
}

// Unison lives in the Z1 program: parameter 24 is the type (0 off, 1 = 2
// voices, 2 = 3, 3 = 6) and parameter 25 the switch. Both are program
// parameters (group 1) on the Korg global channel, like every other edit.
//
// In MPE mode the button means something else: how many of the six timbres
// play each note (2, 3 or 6, leaving 3, 2 or 1 notes of polyphony). That is
// done in firmware.ino; the Z1 program's own unison is left alone.

int Z1Edit::mpeVoices() {
  switch (mpeUstate) {
    case unisonState::U2: return 2;
    case unisonState::U3: return 3;
    case unisonState::U6: return 6;
    default: return 1;
  }
}

void Z1Edit::showUnison() {
  unisonState u = inMPE() ? mpeUstate : ustate;
  s->setLed(6, u == unisonState::U2);
  s->setLed(7, u == unisonState::U3);
  s->setLed(8, u == unisonState::U6);
}

void Z1Edit::setUnison(unisonState u) {
  ustate = u;
  showUnison();
  sendZ1ProgParm(0, 1, 24, (int)u);
  sendZ1ProgParm(0, 1, 25, u == unisonState::OFF ? 0 : 1);
}

// From a program dump: byte 21 holds the type in bits 0-1 and the switch in bit 2.
void Z1Edit::unisonFromPatch(uint8_t b) {
  bool on = (b & 0x04) != 0;
  uint8_t type = b & 0x03;
  ustate = (on && type) ? (unisonState)type : unisonState::OFF;
  showUnison();
}

void Z1Edit::nextUnisonMode() {
  if (inMPE()) {
    switch (mpeUstate) {
      case unisonState::OFF: mpeUstate = unisonState::U2; break;
      case unisonState::U2: mpeUstate = unisonState::U3; break;
      case unisonState::U3: mpeUstate = unisonState::U6; break;
      default: mpeUstate = unisonState::OFF; break;
    }
    showUnison();
    mpeUnisonChanged(mpeVoices());
    return;
  }
  if (mstate != mainState::NORMAL) return;   // on the way into MPE
  switch (ustate) {
    case unisonState::OFF: setUnison(unisonState::U2); break;
    case unisonState::U2: setUnison(unisonState::U3); break;
    case unisonState::U3: setUnison(unisonState::U6); break;
    default: setUnison(unisonState::OFF); break;
  }
}

void Z1Edit::showEG() {
  Serial.println("showEG()");

  if (estate != editState::EG) {

    if (estate != editState::EGMOD) egpage = 0;
    setEditState(editState::EG);
    currPageFunc = &Z1Edit::showEG;

  } else {
    if (!mod) {
      if (egpage >= 0) egpage++;
    }
    if (egpage > 4) egpage = 0;
  }
  s->clear();
  if (egpage < 4) {
    int u = egpage * 19;
    if (!mod) {
      setEditState(editState::EG);
      editables[31 + u]->show(0);
      editables[32 + u]->show(1);
      editables[33 + u]->show(2);
      editables[34 + u]->show(3);
      editables[35 + u]->show(4);
      editables[36 + u]->show(5);
      editables[37 + u]->show(6);
      editables[38 + u]->show(7);
      editables[39 + u]->show(8);

      editables[42 + u]->show(9);

    } else {
      setEditState(editState::EGMOD);
      s->label(0, "EG");
      s->value(0, "MOD");
      editables[40 + u]->show(1);
      editables[41 + u]->show(2);
      editables[43 + u]->show(3);
      editables[44 + u]->show(4);
      editables[45 + u]->show(5);
      editables[46 + u]->show(6);
      editables[47 + u]->show(7);
      editables[48 + u]->show(8);
      editables[49 + u]->show(9);
    }
  } else {
    //amp EG
    if (!mod) {
      setEditState(editState::EG);
      s->label(0, "EG AMP");

      editables[334]->show(1);
      editables[335]->show(2);
      editables[336]->show(3);
      editables[337]->show(4);
      editables[338]->show(5);
      editables[339]->show(6);
      editables[340]->show(7);

    } else {
      setEditState(editState::EGMOD);
      s->label(0, "EG AMP");
      s->value(0, "MOD");
      editables[342]->show(1);
      editables[343]->show(2);
      editables[344]->show(3);
      editables[345]->show(4);
      editables[346]->show(5);
      editables[347]->show(6);
      editables[348]->show(7);
      editables[349]->show(8);
      editables[350]->show(9);
      editables[351]->show(9);
    }
  }
}


void Z1Edit::showLFO() {

  std::string t = "LFO";


  if (estate != editState::LFO) {

    lfopage = 0;
    setEditState(editState::LFO);
    currPageFunc = &Z1Edit::showLFO;

  } else {

    if (lfopage >= 0) lfopage++;
    if (lfopage > 3) lfopage = 0;
  }

  t = t + std::to_string(lfopage + 1);
  Serial.println(t.c_str());
  //s->clear();
  int u = lfopage * 14;

  //setEditState(editState::LFO);
  s->label(0, t.c_str());
  s->value(0, "");
  editables[107 + u]->show(1);
  editables[108 + u]->show(2);
  editables[109 + u]->show(3);
  editables[110 + u]->show(4);
  editables[111 + u]->show(5);
  editables[112 + u]->show(6);
  editables[113 + u]->show(7);
  editables[114 + u]->show(8);
  editables[115 + u]->show(9);
  editables[116 + u]->show(10);
  editables[117 + u]->show(11);
  editables[118 + u]->show(12);
  editables[119 + u]->show(13);
  editables[120 + u]->show(14);
}

void Z1Edit::showOSC1Std() {
  Serial.println("showOSC1Std()");

  int u = O1;
  int a = 174;
  int b = 175;
  if (estate == editState::OSC2) {
    u = O2;
    a = 188;
    b = 189;
  }

  if (!mod) {
    editables[a]->show(0);
    editables[a]->setEnabled(true);
    editables[464 | u]->show(1);
    editables[b]->show(2);
    editables[465 | u]->show(3);
    editables[466 | u]->show(4);
    editables[467 | u]->show(5);
    editables[468 | u]->show(6);
    editables[469 | u]->show(7);
    editables[470 | u]->show(8);
    editables[475 | u]->show(9);
    editables[478 | u]->show(10);
    editables[479 | u]->show(11);
    editables[480 | u]->show(12);
    editables[483 | u]->show(13);
    s->clear(14);
    s->clear(15);
    s->clear(16);
    s->clear(17);
  } else {
    editables[a]->setEnabled(false);
    s->value(0, "MOD");
    editables[471 | u]->show(1);
    editables[472 | u]->show(2);
    editables[473 | u]->show(3);
    editables[474 | u]->show(4);
    s->clear(5);
    editables[476 | u]->show(6);
    editables[477 | u]->show(7);
    editables[481 | u]->show(8);
    editables[482 | u]->show(9);
    editables[484 | u]->show(10);
    editables[485 | u]->show(11);
    s->clear(12);
    s->clear(13);
    s->clear(14);
    s->clear(15);
    s->clear(16);
    s->clear(17);
  }
}

void Z1Edit::showOSC1Comb() {
  Serial.println("showOSC1Comb()");
  int u = O1;
  int a = 174;
  int b = 175;
  if (estate == editState::OSC2) {
    u = O2;
    a = 188;
    b = 189;
  }

  if (!mod) {
    editables[a]->show(0);
    editables[a]->setEnabled(true);
    editables[486 | u]->show(1);
    editables[b]->show(2);
    editables[487 | u]->show(3);
    editables[488 | u]->show(4);
    editables[489 | u]->show(5);
    editables[492 | u]->show(6);
    editables[497 | u]->show(7);
    s->clear(8);
    s->clear(9);
    s->clear(10);
    s->clear(11);
    s->clear(12);
    s->clear(13);
    s->clear(14);
    s->clear(15);
    s->clear(16);
    s->clear(17);
  } else {
    editables[a]->setEnabled(false);
    s->value(0, "MOD");
    editables[490 | u]->show(1);
    editables[491 | u]->show(2);
    editables[493 | u]->show(3);
    editables[494 | u]->show(4);
    s->clear(5);
    editables[495 | u]->show(6);
    editables[496 | u]->show(7);
    editables[498 | u]->show(8);
    editables[499 | u]->show(9);
    s->clear(10);
    s->clear(11);
    s->clear(12);
    s->clear(13);
    s->clear(14);
    s->clear(15);
    s->clear(16);
    s->clear(17);
  }
}
void Z1Edit::showOSC1VPM() {
  Serial.println("showOSC1VPM()");
  int u = O1;
  int a = 174;
  int b = 175;
  if (estate == editState::OSC2) {
    u = O2;
    a = 188;
    b = 189;
  }

  if (!mod) {
    editables[a]->show(0);
    editables[a]->setEnabled(true);
    editables[500 | u]->show(1);
    editables[b]->show(2);
    editables[501 | u]->show(3);
    editables[506 | u]->show(4);
    editables[511 | u]->show(5);
    editables[512 | u]->show(6);
    editables[513 | u]->show(7);
    editables[514 | u]->show(8);
    editables[519 | u]->show(9);
    editables[520 | u]->show(10);
    s->clear(11);
    s->clear(12);
    s->clear(13);
    s->clear(14);
    s->clear(15);
    s->clear(16);
    s->clear(17);
  } else {
    editables[a]->setEnabled(false);
    s->value(0, "MOD");
    editables[502 | u]->show(1);
    editables[503 | u]->show(2);
    editables[504 | u]->show(3);
    editables[505 | u]->show(4);
    s->clear(5);
    editables[507 | u]->show(6);
    editables[508 | u]->show(7);
    editables[509 | u]->show(8);
    editables[510 | u]->show(9);
    editables[515 | u]->show(10);
    editables[516 | u]->show(11);
    editables[517 | u]->show(12);
    editables[518 | u]->show(13);
    editables[521 | u]->show(14);
    editables[522 | u]->show(15);
    editables[523 | u]->show(16);
    editables[524 | u]->show(17);
  }
}
void Z1Edit::showOSC1Reso() {
  Serial.println("showOSC1Reso()");
  int u = O1;
  int a = 174;
  int b = 175;
  if (estate == editState::OSC2) {
    u = O2;
    a = 188;
    b = 189;
  }

  s->clear(16);
  s->clear(17);

  if (!mod) {
    editables[a]->show(0);
    editables[a]->setEnabled(true);
    editables[525 | u]->show(1);
    editables[b]->show(2);
    editables[526 | u]->show(3);
    editables[531 | u]->show(4);
    editables[532 | u]->show(5);
    editables[535 | u]->show(6);
    editables[536 | u]->show(7);
    editables[537 | u]->show(8);
    editables[538 | u]->show(9);
    editables[541 | u]->show(10);
    editables[542 | u]->show(11);

    if (page == 0) {
      editables[543 | u]->show(12);
      editables[544 | u]->show(13);
      editables[547 | u]->show(14);
      editables[548 | u]->show(15);
    } else {

      editables[549 | u]->show(12);
      editables[550 | u]->show(13);
      editables[553 | u]->show(14);
      editables[554 | u]->show(15);
    }
    s->subscribeButton(17, [&]() {
      page++;
      if (page > 1) page = 0;
      Serial.printf("page %d\n", page);
      if (currPageFunc) (this->*Z1Edit::currPageFunc)();
    });
  } else {
    editables[a]->setEnabled(false);
    s->value(0, "MOD");
    editables[527 | u]->show(1);
    editables[528 | u]->show(2);
    editables[529 | u]->show(3);
    editables[530 | u]->show(4);
    s->clear(5);
    editables[533 | u]->show(6);
    editables[534 | u]->show(7);
    editables[539 | u]->show(8);
    editables[540 | u]->show(9);
    editables[545 | u]->show(10);
    editables[546 | u]->show(11);
    editables[551 | u]->show(12);
    editables[552 | u]->show(13);
    editables[555 | u]->show(14);
    editables[556 | u]->show(15);
  }
}
void Z1Edit::showOSC1Ring() {
  Serial.println("showOSC1Ring()");
  int u = O1;
  //int a = 174;
  int b = 175;
  if (estate == editState::OSC2) {
    u = O2;
    //a = 188;
    b = 189;
  }

  editables[557 | u]->show(1);
  editables[b]->show(2);
  editables[558 | u]->show(3);
  editables[559 | u]->show(4);
  editables[564 | u]->show(5);
  editables[565 | u]->show(6);
  editables[560 | u]->show(7);
  editables[561 | u]->show(8);
  editables[562 | u]->show(9);
  editables[563 | u]->show(10);
  s->clear(11);
  s->clear(12);
  s->clear(13);
  s->clear(14);
  s->clear(15);
  s->clear(16);
  s->clear(17);
}
void Z1Edit::showOSC1Cross() {
  Serial.println("showOSC1Cross()");
  int u = O1;
  //int a = 174;
  int b = 175;
  if (estate == editState::OSC2) {
    u = O2;
    //a = 188;
    b = 189;
  }

  editables[566 | u]->show(1);
  editables[b]->show(2);
  editables[567 | u]->show(3);
  editables[568 | u]->show(4);
  editables[573 | u]->show(5);
  editables[569 | u]->show(6);
  editables[570 | u]->show(7);
  editables[571 | u]->show(8);
  editables[572 | u]->show(9);

  s->clear(10);
  s->clear(11);
  s->clear(12);
  s->clear(13);
  s->clear(14);
  s->clear(15);
  s->clear(16);
  s->clear(17);
}
void Z1Edit::showOSC1Sync() {
  Serial.println("showOSC1Sync()");
  int u = O1;
  //int a = 174;
  int b = 175;
  if (estate == editState::OSC2) {
    u = O2;
    //a = 188;
    b = 189;
  }

  editables[574 | u]->show(1);
  editables[b]->show(2);
  editables[575 | u]->show(3);
  editables[576 | u]->show(4);
  s->clear(5);
  s->clear(6);
  s->clear(7);
  s->clear(8);
  s->clear(9);
  s->clear(10);
  s->clear(11);
  s->clear(12);
  s->clear(13);
  s->clear(14);
  s->clear(15);
  s->clear(16);
  s->clear(17);
}
void Z1Edit::showOSC1Organ() {
  Serial.println("showOSC1Organ()");
  int u = O1;
  int a = 174;
  int b = 175;
  if (estate == editState::OSC2) {
    u = O2;
    a = 188;
    b = 189;
  }

  if (!mod) {
    editables[a]->show(0);
    editables[a]->setEnabled(true);

    editables[577 | u]->show(1);
    editables[578 | u]->show(2);
    editables[579 | u]->show(3);
    editables[580 | u]->show(4);
    editables[583 | u]->show(5);
    editables[584 | u]->show(6);
    editables[585 | u]->show(7);
    editables[586 | u]->show(8);
    editables[587 | u]->show(9);
    editables[590 | u]->show(10);
    editables[591 | u]->show(11);
    editables[592 | u]->show(12);
    editables[593 | u]->show(13);
    editables[594 | u]->show(14);
    editables[597 | u]->show(15);
    editables[598 | u]->show(16);
    editables[599 | u]->show(17);
    //Oct missing, put in Mod page?
  } else {
    editables[a]->setEnabled(false);
    s->value(0, "MOD");
    s->clear(1);
    editables[b]->show(2);
    editables[581 | u]->show(3);
    editables[582 | u]->show(4);
    s->clear(5);
    editables[588 | u]->show(6);
    editables[589 | u]->show(7);
    editables[595 | u]->show(8);
    editables[596 | u]->show(9);
    editables[600 | u]->show(10);
    editables[601 | u]->show(11);
    s->clear(12);
    s->clear(13);
    s->clear(14);
    s->clear(15);
    s->clear(16);
    s->clear(17);
  }
}
void Z1Edit::showOSC1EP() {
  Serial.println("showOSC1EP()");
  int u = O1;
  //int a = 174;
  int b = 175;
  if (estate == editState::OSC2) {
    u = O2;
    //a = 188;
    b = 189;
  }

  editables[602 | u]->show(1);
  editables[b]->show(2);
  editables[603 | u]->show(3);
  editables[604 | u]->show(4);
  editables[605 | u]->show(5);
  editables[606 | u]->show(6);
  editables[607 | u]->show(7);
  editables[608 | u]->show(8);
  editables[609 | u]->show(9);
  editables[610 | u]->show(10);
  editables[611 | u]->show(11);
  editables[614 | u]->show(12);
  editables[615 | u]->show(13);


  s->clear(14);
  editables[612 | u]->show(15);
  editables[613 | u]->show(16);
  s->clear(17);
}
void Z1Edit::showOSC1Brass() {
  Serial.println("showOSC1Brass()");
  int u = O1;


  if (!mod) {
    editables[174]->show(0);
    editables[174]->setEnabled(true);


    editables[616 | u]->show(1);
    editables[175]->show(2);
    editables[619 | u]->show(3);
    editables[620 | u]->show(4);
    editables[626 | u]->show(5);
    editables[632 | u]->show(6);
    editables[633 | u]->show(7);
    editables[634 | u]->show(8);
    editables[645 | u]->show(9);
    editables[646 | u]->show(10);
    editables[647 | u]->show(11);
    editables[648 | u]->show(12);
    s->clear(13);
    s->clear(14);
    s->clear(15);
    s->clear(16);
    s->clear(17);
  } else {
    editables[174]->setEnabled(false);
    s->value(0, "MOD");
    s->clear(1);
    s->clear(2);
    s->clear(3);
    s->clear(4);
    s->clear(5);

    editables[621 | u]->show(6);
    editables[622 | u]->show(7);
    editables[623 | u]->show(8);
    editables[624 | u]->show(9);
    editables[627 | u]->show(10);
    editables[628 | u]->show(11);

    s->clear(12);
    s->clear(13);
    s->clear(14);
    s->clear(15);
    s->clear(16);
    s->clear(17);
  }
}
void Z1Edit::showOSC1Reed() {
  Serial.println("showOSC1Reed()");
  int u = O1;

  if (!mod) {
    editables[174]->show(0);
    editables[174]->setEnabled(true);
    editables[654 | u]->show(1);
    editables[175]->show(2);

    editables[657 | u]->show(4);
    editables[658 | u]->show(5);
    editables[668 | u]->show(6);
    editables[683 | u]->show(7);
    editables[684 | u]->show(8);
    editables[685 | u]->show(9);
    editables[686 | u]->show(10);
    editables[687 | u]->show(11);
    editables[689 | u]->show(12);
    editables[690 | u]->show(13);
    editables[691 | u]->show(14);
    s->clear(15);
    s->clear(16);
    s->clear(17);
  } else {
    editables[174]->setEnabled(false);
    s->value(0, "MOD");
    s->clear(1);
    s->clear(2);
    s->clear(3);
    s->clear(4);
    s->clear(5);

    editables[659 | u]->show(6);
    editables[660 | u]->show(7);
    editables[661 | u]->show(8);
    editables[662 | u]->show(9);
    editables[681 | u]->show(10);
    editables[682 | u]->show(11);
    editables[692 | u]->show(12);
    editables[693 | u]->show(13);

    s->clear(14);
    s->clear(15);
    s->clear(16);
    s->clear(17);
  }
}
void Z1Edit::showOSC1Pluck() {
  Serial.println("showOSC1Pluck()");
  int u = O1;

  if (!mod) {
    editables[174]->show(0);
    editables[174]->setEnabled(true);
    editables[694 | u]->show(1);
    editables[175]->show(2);
    editables[695 | u]->show(3);
    editables[696 | u]->show(4);
    editables[697 | u]->show(5);
    editables[698 | u]->show(6);
    editables[699 | u]->show(7);
    editables[700 | u]->show(8);
    editables[701 | u]->show(9);
    editables[702 | u]->show(10);
    if (page == 0) {
      editables[705 | u]->show(11);
      editables[708 | u]->show(12);
      editables[709 | u]->show(13);
      editables[712 | u]->show(14);
      editables[713 | u]->show(15);
      editables[714 | u]->show(16);
    } else {
      editables[715 | u]->show(11);
      editables[718 | u]->show(12);
      editables[719 | u]->show(13);
      editables[722 | u]->show(14);
      editables[723 | u]->show(15);
      editables[724 | u]->show(16);
    }
  } else {
    editables[174]->setEnabled(false);
    s->value(0, "MOD");
    s->clear(1);
    editables[703 | u]->show(2);
    editables[704 | u]->show(3);
    editables[706 | u]->show(4);
    editables[707 | u]->show(5);
    editables[710 | u]->show(6);
    editables[711 | u]->show(7);
    editables[716 | u]->show(8);
    editables[717 | u]->show(9);
    editables[720 | u]->show(10);
    editables[721 | u]->show(11);
    s->clear(12);
    s->clear(13);
    s->clear(14);
    s->clear(15);
    s->clear(16);
  }
  s->clear(17);
  s->subscribeButton(17, [&]() {
    page++;
    if (page > 1) page = 0;
    Serial.printf("page %d\n", page);
    if (currPageFunc) (this->*Z1Edit::currPageFunc)();
  });
}
void Z1Edit::showOSC1Bowed() {
  Serial.println("showOSC1Bowed()");
  int u = O1;

  if (!mod) {
    editables[174]->show(0);
    editables[174]->setEnabled(true);
    editables[725 | u]->show(1);
    editables[175]->show(2);
    editables[726 | u]->show(3);
    editables[731 | u]->show(4);
    editables[732 | u]->show(5);
    editables[733 | u]->show(6);
    editables[736 | u]->show(7);
    editables[737 | u]->show(8);
    editables[740 | u]->show(9);
    editables[741 | u]->show(10);
    editables[742 | u]->show(11);
    editables[743 | u]->show(12);
    editables[746 | u]->show(13);
    editables[749 | u]->show(14);
    editables[752 | u]->show(15);
    editables[753 | u]->show(16);
    editables[754 | u]->show(17);
  } else {
    editables[174]->setEnabled(false);
    s->value(0, "MOD");
    s->clear(1);
    editables[727 | u]->show(2);
    editables[728 | u]->show(3);
    editables[729 | u]->show(4);
    editables[730 | u]->show(5);
    editables[734 | u]->show(6);
    editables[735 | u]->show(7);
    editables[738 | u]->show(8);
    editables[739 | u]->show(9);
    editables[744 | u]->show(10);
    editables[745 | u]->show(11);
    editables[747 | u]->show(12);
    editables[748 | u]->show(13);
    editables[750 | u]->show(14);
    editables[751 | u]->show(15);
    s->clear(16);
  }
}


void Z1Edit::showOSC1() {
  Serial.println("showOSC1");
  setEditState(editState::OSC1);
  s->clear();
  //osc1type
  editables[174]->show(0);

  //oct/tune
  editables[175]->show(2);
  static int pmode = 0;
  s->subscribeButton(2, [&]() {
    Serial.printf("piip %d\n", pmode);
    pmode++;
    if (pmode > 2) pmode = 0;
    if (pmode == 0) editables[175]->show(2);
    if (pmode == 1) editables[176]->show(2);
    if (pmode == 2) editables[177]->show(2);
  });
  editables[174]->subscribe([&](int key, editableType t, int iVal, float fVal, std::string sVal) {
    Serial.printf("%s %d %d\n", sVal.c_str(), key, iVal);
    switch (iVal) {
      case 0:
        currPageFunc = &Z1Edit::showOSC1Std;
        break;
      case 1:
        currPageFunc = &Z1Edit::showOSC1Comb;
        break;
      case 2:
        currPageFunc = &Z1Edit::showOSC1VPM;
        break;
      case 3:
        currPageFunc = &Z1Edit::showOSC1Reso;
        break;
      case 4:
        currPageFunc = &Z1Edit::showOSC1Ring;
        break;
      case 5:
        currPageFunc = &Z1Edit::showOSC1Cross;
        break;
      case 6:
        currPageFunc = &Z1Edit::showOSC1Sync;
        break;
      case 7:
        currPageFunc = &Z1Edit::showOSC1Organ;
        break;
      case 8:
        currPageFunc = &Z1Edit::showOSC1EP;
        break;
      case 9:
        currPageFunc = &Z1Edit::showOSC1Brass;
        break;
      case 10:
        currPageFunc = &Z1Edit::showOSC1Reed;
        break;
      case 11:
        currPageFunc = &Z1Edit::showOSC1Pluck;
        break;
      case 12:
        currPageFunc = &Z1Edit::showOSC1Bowed;
        break;
      default:
        break;
    }
    (this->*Z1Edit::currPageFunc)();
    sendZ1ProgParm(0, 1, key, iVal);
  });
  int t=editables[174]->get();
  switch (t) {
    case 0:
      showOSC1Std();
      break;
    case 1:
      showOSC1Comb();
      break;
    case 2:
      showOSC1VPM();
      break;
    case 3:
      showOSC1Reso();
      break;
    case 4:
      showOSC1Ring();
      break;
    case 5:
      showOSC1Cross();
      break;
    case 6:
      showOSC1Sync();
      break;
    case 7:
      showOSC1Organ();
      break;
    case 8:
      showOSC1EP();
      break;
    case 9:
      showOSC1Brass();
      break;
    case 10:
      showOSC1Reed();
      break;
    case 11:
      showOSC1Pluck();
      break;
    case 12:
      showOSC1Bowed();
      break;
  }

}

void Z1Edit::showOSC2() {
  Serial.println("showOSC2");
  setEditState(editState::OSC2);
  s->clear();
  //osc1type
  editables[188]->show(0);

  //oct/tune
  editables[189]->show(2);
  static int pmode = 0;
  s->subscribeButton(2, [&]() {
    pmode++;
    if (pmode > 2) pmode = 0;
    if (pmode == 0) editables[189]->show(2);
    if (pmode == 1) editables[190]->show(2);
    if (pmode == 2) editables[191]->show(2);
  });
  editables[188]->subscribe([&](int key, editableType t, int iVal, float fVal, std::string sVal) {
    Serial.printf("%s %d %d\n", sVal.c_str(), key, iVal);
    switch (iVal) {
      case 0:
        currPageFunc = &Z1Edit::showOSC1Std;
        break;
      case 1:
        currPageFunc = &Z1Edit::showOSC1Comb;
        break;
      case 2:
        currPageFunc = &Z1Edit::showOSC1VPM;
        break;
      case 3:
        currPageFunc = &Z1Edit::showOSC1Reso;
        break;
      case 4:
        currPageFunc = &Z1Edit::showOSC1Ring;
        break;
      case 5:
        currPageFunc = &Z1Edit::showOSC1Cross;
        break;
      case 6:
        currPageFunc = &Z1Edit::showOSC1Sync;
        break;
      case 7:
        currPageFunc = &Z1Edit::showOSC1Organ;
        break;
      case 8:
        currPageFunc = &Z1Edit::showOSC1EP;
        break;
      default:
        break;
    }
    (this->*Z1Edit::currPageFunc)();
    sendZ1ProgParm(0, 1, key, iVal);
  });
  int t=editables[188]->get();
  switch (t) {
    case 0:
      showOSC1Std();
      break;
    case 1:
      showOSC1Comb();
      break;
    case 2:
      showOSC1VPM();
      break;
    case 3:
      showOSC1Reso();
      break;
    case 4:
      showOSC1Ring();
      break;
    case 5:
      showOSC1Cross();
      break;
    case 6:
      showOSC1Sync();
      break;
    case 7:
      showOSC1Organ();
      break;
    case 8:
      showOSC1EP();
      break;
    case 9:
      showOSC1Brass();
      break;
    case 10:
      showOSC1Reed();
      break;
    case 11:
      showOSC1Pluck();
      break;
    case 12:
      showOSC1Bowed();
      break;
  }
}


void Z1Edit::showFilt() {
  Serial.println("showFilt()");
  s->clear();

  if (estate != editState::FILT) {

    if (estate != editState::FILTMOD) fpage = 0;
    setEditState(editState::FILT);
    currPageFunc = &Z1Edit::showFilt;

  } else {
    if (!mod) {
      if (fpage >= 0) fpage++;
    }
    if (fpage > 2) fpage = 0;
  }
  s->clear();
  if (fpage == 2) {
    s->label(0, "FILT");
    s->value(0, "COMMON");
    editables[258]->show(1);
    editables[259]->show(2);
    editables[260]->show(3);



  } else {
    int u = fpage * 27;
    if (!mod) {
      setEditState(editState::FILT);
      editables[261 + u]->show(0);
      editables[262 + u]->show(1);
      editables[263 + u]->show(2);
      editables[264 + u]->show(3);
      editables[265 + u]->show(4);
      editables[266 + u]->show(5);
      editables[267 + u]->show(6);
      editables[274 + u]->show(7);
      editables[277 + u]->show(8);
      editables[278 + u]->show(9);
      editables[279 + u]->show(10);
      editables[280 + u]->show(11);
      editables[281 + u]->show(12);
      editables[282 + u]->show(13);
      editables[286 + u]->show(14);



    } else {
      setEditState(editState::FILTMOD);
      if (u == 0) s->label(0, "FILT1");
      else s->label(0, "FILT2");
      s->value(0, "MOD");
      editables[268 + u]->show(1);
      editables[269 + u]->show(2);
      editables[270 + u]->show(3);
      editables[271 + u]->show(4);
      editables[272 + u]->show(5);
      editables[273 + u]->show(6);
      editables[275 + u]->show(7);
      editables[276 + u]->show(8);
      editables[283 + u]->show(9);
      editables[284 + u]->show(10);
      editables[285 + u]->show(11);
      editables[287 + u]->show(12);
    }
  }
}

void Z1Edit::showSubOsc() {
  Serial.println("showSubOsc()");
  if (estate != editState::SUBOSC) {
    spage = 0;
    setEditState(editState::SUBOSC);
    currPageFunc = nullptr;

  } else {
    spage++;
    if (spage > 2) spage = 0;
  }
  s->clear();
  if (spage == 0) {
    s->label(0, "SubOsc");
    s->value(0, "");
    editables[202]->show(1);
    editables[203]->show(2);
    editables[204]->show(3);
    editables[205]->show(4);
    editables[206]->show(5);
    editables[207]->show(6);
    editables[208]->show(7);
    editables[209]->show(8);
    editables[210]->show(9);
    editables[211]->show(10);
    editables[212]->show(11);
    editables[213]->show(12);
    editables[214]->show(13);
    editables[215]->show(14);
  } else if (spage == 1) {
    //noise
    s->label(0, "Noise");
    s->value(0, "");
    editables[216]->show(1);
    editables[217]->show(2);
    editables[218]->show(3);
    editables[219]->show(4);
    editables[220]->show(5);
    editables[221]->show(6);
    editables[222]->show(7);
    editables[223]->show(8);

  } else if (spage == 2) {
    s->label(0, "AMP");
    s->value(0, "");
    editables[315]->show(1);
    editables[316]->show(2);
    editables[317]->show(3);
    editables[318]->show(4);
    editables[319]->show(5);
    editables[320]->show(6);
    //no 321
    editables[322]->show(7);
    editables[323]->show(8);
    //amp2
    editables[324]->show(9);
    editables[325]->show(10);
    editables[326]->show(11);
    editables[327]->show(12);
    editables[328]->show(13);
    editables[329]->show(14);
    //no 330
    editables[331]->show(15);
    editables[332]->show(16);
  }
}


void Z1Edit::showFx() {
  Serial.println("showFx()");
  if (estate != editState::FX) {
    if (estate != editState::FXMOD) fxpage = 0;
    setEditState(editState::FX);
    currPageFunc = nullptr;

  } else {
    fxpage++;
    if (fxpage == 2 && editables[359]->getIval() > 10) fxpage++;  //fx2 disabled
    if (fxpage > 3) fxpage = 0;
  }
  s->clear();
  editables[362]->setUpdateFunc(z1ExpFunc, 0, 80.0, 0.069327, 0.0);
  editables[364]->setUpdateFunc(z1ExpFunc, 0, 400.0, 0.0692415, 0.0);
  if (fxpage == 0) {
    editables[356]->show(0);
    editables[357]->show(1);
    editables[358]->show(2);
    editables[359]->show(3);
    if (editables[359]->getIval() < 11)
      editables[360]->show(4);
    //else s->clear(4);
    editables[361]->show(5);
    editables[362]->show(6);
    editables[363]->show(7);
    editables[364]->show(8);
    editables[365]->show(9);
    s->clear(10);
    s->clear(11);
    s->clear(12);
    s->clear(13);
    s->clear(14);
    s->clear(15);
    s->clear(16);
    s->clear(17);

  } else if (fxpage == 3) {
    //master effect
    int p = editables[361]->getIval();
    switch (p) {
      case 0:
        currPageFunc = &Z1Edit::showFxDelay;
        break;
      case 1:
        currPageFunc = &Z1Edit::showFxHall;
        break;
      case 2:
        currPageFunc = &Z1Edit::showFxRoom;
        break;
      default:
        break;
    }
    (this->*Z1Edit::currPageFunc)();

  }

  else {
    int p = 0;
    if (fxpage == 1) p = editables[359]->getIval();
    if (fxpage == 2) p = editables[360]->getIval();
    Serial.printf("showFx() page %d, p %d\r\n", fxpage, p);



    switch (p) {
      case 0:
        currPageFunc = &Z1Edit::showFxOverdrive;
        break;
      case 1:
        currPageFunc = &Z1Edit::showFxCompressor;
        break;
      case 2:
        currPageFunc = &Z1Edit::showFxPEQ;
        break;
      case 3:
        currPageFunc = &Z1Edit::showFxWah;
        break;
      case 4:
        currPageFunc = &Z1Edit::showFxExciter;
        break;
      case 5:
        currPageFunc = &Z1Edit::showFxDecimator;
        break;
      case 6:
        currPageFunc = &Z1Edit::showFxChorus;
        break;
      case 7:
        currPageFunc = &Z1Edit::showFxFlanger;
        break;
      case 8:
        currPageFunc = &Z1Edit::showFxPhaser;
        break;
      case 9:
        currPageFunc = &Z1Edit::showFxRotarySmall;
        break;
      case 10:
        currPageFunc = &Z1Edit::showFxDelayMono;
        break;
      case 11:
        currPageFunc = &Z1Edit::showFxTalking;
        break;
      case 12:
        currPageFunc = &Z1Edit::showFxMultitap;
        break;
      case 13:
        currPageFunc = &Z1Edit::showFxEnsemble;
        break;
      case 14:
        currPageFunc = &Z1Edit::showFxRotary;
        break;
      default:
        break;
    }
    (this->*Z1Edit::currPageFunc)();
    //sendZ1ProgParm(0,1,key, iVal);});
  }
}

void Z1Edit::showFxDelay() {
  Serial.println("showFxDelay()");
  s->clear();
  s->label(0, "FXMstr");
  s->value(0, "Delay");


  editables[1007 | EGL]->setUpdateFunc(z1ScaleFunc, 1, 0, 680.0, 251.0);
  editables[1008 | EGL]->setUpdateFunc(z1ScaleFunc, 1, 0, 680.0, 251.0);


  editables[1004 | EGL]->show(1);
  editables[1005 | EGL]->show(2);
  editables[1006 | EGL]->show(3);
  editables[1007 | EGL]->show(4);
  editables[1008 | EGL]->show(5);
  editables[1009 | EGL]->show(6);
  editables[1010 | EGL]->show(7);
  editables[1011 | EGL]->show(8);
  editables[1012 | EGL]->show(9);
  editables[1013 | EGL]->show(10);
  editables[1014 | EGL]->show(11);
  editables[1015 | EGL]->show(12);
  editables[1016 | EGL]->show(13);
  editables[1017 | EGL]->show(14);
}

void Z1Edit::showFxHall() {
  Serial.println("showFxHall()");
  s->clear();
  s->label(0, "FXMstr");
  s->value(0, "Hall");


  editables[1018 | EGL]->setUpdateFunc(z1ScaleFunc, 1, 0, 1.0, 10.0);
  editables[1023 | EGL]->setUpdateFunc(z1ScaleFunc, 1, 0, 1, 2.0);
  editables[1024 | EGL]->setUpdateFunc(z1ScaleFunc, 1, 0, 1, 2.0);

  editables[1018 | EGL]->show(1);
  editables[1019 | EGL]->show(2);
  editables[1020 | EGL]->show(3);
  editables[1021 | EGL]->show(4);
  editables[1022 | EGL]->show(5);
  editables[1023 | EGL]->show(6);
  editables[1024 | EGL]->show(7);
  editables[1025 | EGL]->show(8);
  editables[1026 | EGL]->show(9);
  editables[1027 | EGL]->show(10);
}

void Z1Edit::showFxRoom() {
  Serial.println("showFxRoom()");
  s->clear();
  s->label(0, "FXMstr");
  s->value(0, "Room");


  editables[1028 | EGL]->setUpdateFunc(z1ScaleFunc, 1, 0, 1.0, 10.0);
  editables[1035 | EGL]->setUpdateFunc(z1ScaleFunc, 1, 0, 1, 2.0);
  editables[1036 | EGL]->setUpdateFunc(z1ScaleFunc, 1, 0, 1, 2.0);

  editables[1028 | EGL]->show(1);
  editables[1029 | EGL]->show(2);
  editables[1030 | EGL]->show(3);
  editables[1031 | EGL]->show(4);
  editables[1032 | EGL]->show(5);
  editables[1033 | EGL]->show(6);
  editables[1034 | EGL]->show(7);
  editables[1035 | EGL]->show(8);
  editables[1036 | EGL]->show(9);
  editables[1037 | EGL]->show(10);
  editables[1038 | EGL]->show(11);
  editables[1039 | EGL]->show(12);
}

void Z1Edit::showFxOverdrive() {
  Serial.println("showFxOverdrive()");
  int u = E1;
  if (fxpage == 2) u = E2;
  s->clear();
  char numstr[5];
  sprintf(numstr, "FX%d", fxpage);
  s->label(0, numstr);
  s->value(0, "OvrDrv");


  editables[804 | u]->setUpdateFunc(z1ExpFunc, 0, 20.0, 0.086704, 0.0);
  editables[805 | u]->setUpdateFunc(z1ScaleFunc, 1, 0.5, 10.0, 95.0);
  editables[806 | u]->setUpdateFunc(z1ScaleFunc, 1, 0, 1, 2.0);
  editables[807 | u]->setUpdateFunc(z1ExpFunc, 0, 50.0, 0.115571, 0.0);
  editables[808 | u]->setUpdateFunc(z1ScaleFunc, 1, 0.5, 10.0, 95.0);
  editables[809 | u]->setUpdateFunc(z1ScaleFunc, 1, 0, 1, 2.0);
  editables[810 | u]->setUpdateFunc(z1ExpFunc, 0, 200.0, 0.086704, 0.0);
  editables[811 | u]->setUpdateFunc(z1ScaleFunc, 1, 0.5, 10.0, 95.0);
  editables[812 | u]->setUpdateFunc(z1ScaleFunc, 1, 0, 1, 2.0);
  editables[813 | u]->setUpdateFunc(z1ExpFunc, 0, 500.0, 0.077042, 0.0);
  editables[814 | u]->setUpdateFunc(z1ScaleFunc, 1, 0.5, 10.0, 95.0);
  editables[815 | u]->setUpdateFunc(z1ScaleFunc, 1, 0, 1, 2.0);


  if (!mod) {
    setEditState(editState::FX);
    editables[801 | u]->show(1);
    editables[802 | u]->show(2);
    editables[803 | u]->show(3);
    editables[804 | u]->show(4);
    editables[805 | u]->show(5);
    editables[806 | u]->show(6);
    editables[807 | u]->show(7);
    editables[808 | u]->show(8);
    editables[809 | u]->show(9);
    editables[810 | u]->show(10);
    editables[811 | u]->show(11);
    editables[812 | u]->show(12);
    editables[813 | u]->show(13);
    editables[814 | u]->show(14);
    editables[815 | u]->show(15);
    editables[816 | u]->show(16);
  } else {
    setEditState(editState::FXMOD);
    editables[816 | u]->show(12);
    editables[817 | u]->show(13);
    editables[818 | u]->show(14);
  }
}

void Z1Edit::showFxCompressor() {
  Serial.println("showFxCompressor()");
  int u = E1;
  if (fxpage == 2) u = E2;
  s->clear();
  char numstr[5];
  sprintf(numstr, "FX%d", fxpage);
  s->label(0, numstr);
  s->value(0, "Compres");


  editables[822 | u]->setUpdateFunc(z1ScaleFunc, 1, 0, 1, 2.0);
  editables[823 | u]->setUpdateFunc(z1ScaleFunc, 1, 0, 1, 2.0);

  editables[819 | u]->show(1);
  editables[820 | u]->show(2);
  editables[821 | u]->show(3);
  editables[822 | u]->show(4);
  editables[823 | u]->show(5);
  editables[824 | u]->show(6);
  editables[825 | u]->show(7);
  editables[826 | u]->show(8);
  editables[827 | u]->show(9);
}

void Z1Edit::showFxPEQ() {
  Serial.println("showFxPEQ()");
  int u = E1;
  if (fxpage == 2) u = E2;
  s->clear();
  char numstr[5];
  sprintf(numstr, "FX%d", fxpage);
  s->label(0, numstr);
  s->value(0, "PEQ");



  editables[830 | u]->setUpdateFunc(z1ExpFunc, 0, 20.0, 0.086704, 0.0);
  editables[831 | u]->setUpdateFunc(z1ScaleFunc, 1, 0.5, 10.0, 95.0);
  editables[832 | u]->setUpdateFunc(z1ScaleFunc, 1, 0, 1, 2.0);

  editables[833 | u]->setUpdateFunc(z1ExpFunc, 0, 50.0, 0.115571, 0.0);
  editables[834 | u]->setUpdateFunc(z1ScaleFunc, 1, 0.5, 10.0, 95.0);
  editables[835 | u]->setUpdateFunc(z1ScaleFunc, 1, 0, 1, 2.0);

  editables[838 | u]->setUpdateFunc(z1ExpFunc, 0, 200.0, 0.086704, 0.0);
  editables[839 | u]->setUpdateFunc(z1ScaleFunc, 1, 0.5, 10.0, 95.0);
  editables[840 | u]->setUpdateFunc(z1ScaleFunc, 1, 0, 1, 2.0);

  editables[842 | u]->setUpdateFunc(z1ExpFunc, 0, 500.0, 0.077042, 0.0);
  editables[843 | u]->setUpdateFunc(z1ScaleFunc, 1, 0.5, 10.0, 95.0);
  editables[844 | u]->setUpdateFunc(z1ScaleFunc, 1, 0, 1, 2.0);

  editables[837 | u]->setUpdateFunc(z1ScaleFunc, 1, 0, 1, 2.0);


  if (!mod) {
    setEditState(editState::FX);
    editables[828 | u]->show(1);
    editables[829 | u]->show(2);
    editables[830 | u]->show(3);
    editables[831 | u]->show(4);
    editables[832 | u]->show(5);
    editables[833 | u]->show(6);
    editables[834 | u]->show(7);
    editables[835 | u]->show(8);
    editables[838 | u]->show(9);
    editables[839 | u]->show(10);
    editables[840 | u]->show(11);
    editables[841 | u]->show(12);
    editables[842 | u]->show(13);
    editables[843 | u]->show(14);
    editables[844 | u]->show(15);
    editables[845 | u]->show(16);
  } else {
    setEditState(editState::FXMOD);
    editables[835 | u]->show(8);
    editables[836 | u]->show(9);
    editables[837 | u]->show(10);
    editables[845 | u]->show(12);
    editables[846 | u]->show(13);
    editables[847 | u]->show(14);
  }
}
void Z1Edit::showFxWah() {
  Serial.println("showFxWah()");
  s->clear();
  char numstr[5];
  int u = E1;
  if (fxpage == 2) u = E2;
  sprintf(numstr, "FX%d", fxpage);
  s->label(0, numstr);
  s->value(0, "Wah");
  editables[848 | u]->show(1);
  editables[849 | u]->show(2);
  editables[850 | u]->show(3);
  editables[851 | u]->show(4);
  editables[852 | u]->show(5);
  editables[853 | u]->show(6);
  editables[854 | u]->show(7);
  editables[855 | u]->show(8);
  editables[856 | u]->show(9);
  editables[857 | u]->show(10);
  editables[858 | u]->show(11);
  editables[859 | u]->show(12);
}


void Z1Edit::showFxExciter() {
  Serial.println("showFxExciter()");
  int u = E1;
  if (fxpage == 2) u = E2;
  s->clear();
  char numstr[5];
  sprintf(numstr, "FX%d", fxpage);
  s->label(0, numstr);
  s->value(0, "Excitr");

  editables[867 | u]->setUpdateFunc(z1ScaleFunc, 1, 0, 1, 2.0);
  editables[868 | u]->setUpdateFunc(z1ScaleFunc, 1, 0, 1, 2.0);


  editables[860 | u]->show(1);
  editables[861 | u]->show(2);
  editables[862 | u]->show(3);
  editables[863 | u]->show(4);
  editables[864 | u]->show(5);
  editables[865 | u]->show(6);
  editables[866 | u]->show(7);
  editables[867 | u]->show(8);
  editables[868 | u]->show(9);
  editables[869 | u]->show(10);
  editables[870 | u]->show(11);
  editables[871 | u]->show(12);
}
void Z1Edit::showFxDecimator() {
  Serial.println("showFxDecimator()");
  int u = E1;

  if (fxpage == 2) u = E2;
  s->clear();
  char numstr[5];
  sprintf(numstr, "FX%d", fxpage);
  s->label(0, numstr);
  s->value(0, "Decim");

  editables[873 | u]->setUpdateFunc(z1ScaleFunc, 0, 0, 200.0, 1.0);

  //decimator
  editables[872 | u]->show(1);
  editables[873 | u]->show(2);
  editables[874 | u]->show(3);
  editables[875 | u]->show(4);
  editables[876 | u]->show(5);
  editables[877 | u]->show(6);
  editables[878 | u]->show(7);
  editables[879 | u]->show(8);
  editables[880 | u]->show(9);
  editables[881 | u]->show(10);
}
void Z1Edit::showFxChorus() {
  Serial.println("showFxChorus()");
  int u = E1;
  if (fxpage == 2) u = E2;
  s->clear();
  char numstr[5];
  sprintf(numstr, "FX%d", fxpage);
  s->label(0, numstr);
  s->value(0, "Chorus");


  editables[882 | u]->setUpdateFunc(z1ScaleFunc, 2, 0, 50.0, 140.0);
  editables[884 | u]->setUpdateFunc(z1ScaleFunc, 2, 0.04, 20.0, 114.0, -1.0);
  editables[886 | u]->setUpdateFunc(z1ScaleFunc, 2, 0, 20.0, 115.0);
  editables[894 | u]->setUpdateFunc(z1ScaleFunc, 1, 0, 1, 2.0);
  editables[895 | u]->setUpdateFunc(z1ScaleFunc, 1, 0, 1, 2.0);


  if (!mod) {
    setEditState(editState::FX);
    editables[882 | u]->show(1);
    editables[883 | u]->show(2);
    editables[884 | u]->show(3);
    editables[887 | u]->show(4);
    editables[888 | u]->show(5);
    editables[889 | u]->show(6);
    editables[890 | u]->show(7);
    editables[891 | u]->show(8);
    editables[892 | u]->show(9);
    editables[893 | u]->show(10);
    editables[894 | u]->show(11);
    editables[895 | u]->show(12);
    editables[896 | u]->show(13);
    editables[897 | u]->show(14);
    editables[898 | u]->show(15);
    editables[899 | u]->show(16);
  } else {
    setEditState(editState::FXMOD);
    editables[885 | u]->show(4);
    editables[886 | u]->show(5);
  }
}
void Z1Edit::showFxFlanger() {
  Serial.println("showFxFlanger()");
  int u = E1;
  if (fxpage == 2) u = E2;
  s->clear();
  char numstr[5];
  sprintf(numstr, "FX%d", fxpage);
  s->label(0, numstr);
  s->value(0, "Flangr");


  editables[900 | u]->setUpdateFunc(z1ScaleFunc, 2, 0, 50.0, 140.0);
  editables[903 | u]->setUpdateFunc(z1ScaleFunc, 2, 0.04, 20.0, 114.0, -1.0);
  editables[905 | u]->setUpdateFunc(z1ScaleFunc, 2, 0, 20.0, 115.0);



  editables[900 | u]->show(1);
  editables[901 | u]->show(2);
  editables[902 | u]->show(3);
  editables[903 | u]->show(4);
  editables[904 | u]->show(5);
  editables[905 | u]->show(6);
  editables[906 | u]->show(7);
  editables[907 | u]->show(8);
  editables[908 | u]->show(9);
  editables[909 | u]->show(10);
  editables[910 | u]->show(11);
  editables[911 | u]->show(12);
  editables[912 | u]->show(13);
  editables[913 | u]->show(14);
  editables[914 | u]->show(15);
  editables[915 | u]->show(16);
}
void Z1Edit::showFxPhaser() {
  Serial.println("showFxPhaser()");
  int u = E1;
  if (fxpage == 2) u = E2;
  s->clear();
  char numstr[5];
  sprintf(numstr, "FX%d", fxpage);
  s->label(0, numstr);
  s->value(0, "Phaser");



  editables[918 | u]->setUpdateFunc(z1ScaleFunc, 2, 0.04, 20.0, 114.0, -1.0);
  editables[920 | u]->setUpdateFunc(z1ScaleFunc, 2, 0, 20.0, 115.0);


  editables[916 | u]->show(1);
  editables[917 | u]->show(2);
  editables[918 | u]->show(3);
  editables[919 | u]->show(4);
  editables[920 | u]->show(5);
  editables[921 | u]->show(6);
  editables[922 | u]->show(7);
  editables[923 | u]->show(8);
  editables[924 | u]->show(9);
  editables[925 | u]->show(10);
  editables[926 | u]->show(11);
  editables[927 | u]->show(12);
  editables[928 | u]->show(13);
  editables[929 | u]->show(14);
  editables[930 | u]->show(15);
  editables[931 | u]->show(16);
}
void Z1Edit::showFxRotarySmall() {
  Serial.println("showFxRotarySmall()");
  int u = E1;
  if (fxpage == 2) u = E2;
  s->clear();
  char numstr[5];
  sprintf(numstr, "FX%d", fxpage);
  s->label(0, numstr);
  s->value(0, "RotarS");
  editables[932 | u]->show(1);
  editables[933 | u]->show(2);
  editables[934 | u]->show(3);
  editables[935 | u]->show(4);
  editables[936 | u]->show(5);
  editables[937 | u]->show(6);
  editables[938 | u]->show(7);
  editables[939 | u]->show(8);
  editables[940 | u]->show(9);
}

void Z1Edit::showFxDelayMono() {


  Serial.println("showFxDelayMono()");
  int u = E1;
  if (fxpage == 2) u = E2;
  s->clear();
  char numstr[5];
  sprintf(numstr, "FX%d", fxpage);
  s->label(0, numstr);
  s->value(0, "Delay");

  editables[943 | u]->setUpdateFunc(z1ScaleFunc, 1, 0, 680.0, 251.0);

  editables[941 | u]->show(1);
  editables[942 | u]->show(2);
  editables[943 | u]->show(3);
  editables[944 | u]->show(4);
  editables[945 | u]->show(5);
  editables[946 | u]->show(6);
  editables[947 | u]->show(7);
  editables[948 | u]->show(8);
  editables[949 | u]->show(9);
  editables[950 | u]->show(10);
  editables[951 | u]->show(11);
}



void Z1Edit::showFxTalking() {
  Serial.println("showFxTalking()");
  s->clear();
  char numstr[5];
  sprintf(numstr, "FX%d", fxpage);
  s->label(0, numstr);
  s->value(0, "Talkin");
  editables[952 | E1]->show(1);
  editables[953 | E1]->show(2);
  editables[954 | E1]->show(3);
  editables[955 | E1]->show(4);
  editables[956 | E1]->show(5);
  editables[957 | E1]->show(6);
  editables[958 | E1]->show(7);
  editables[959 | E1]->show(8);
  editables[960 | E1]->show(9);
  editables[961 | E1]->show(10);
  editables[962 | E1]->show(11);
}

void Z1Edit::showFxMultitap() {
  Serial.println("showFxMultitap()");
  s->clear();
  char numstr[5];
  sprintf(numstr, "FX%d", fxpage);
  s->label(0, numstr);
  s->value(0, "MultiT");


  editables[966 | E1]->setUpdateFunc(z1ScaleFunc, 1, 0, 680.0, 251.0);
  editables[968 | E1]->setUpdateFunc(z1ScaleFunc, 1, 0, 680.0, 251.0);

  editables[963 | E1]->show(1);
  editables[964 | E1]->show(2);
  editables[965 | E1]->show(3);
  editables[966 | E1]->show(4);
  editables[967 | E1]->show(5);
  editables[968 | E1]->show(6);
  editables[969 | E1]->show(7);
  editables[970 | E1]->show(8);
  editables[971 | E1]->show(9);
  editables[972 | E1]->show(10);
  editables[973 | E1]->show(11);
  editables[974 | E1]->show(12);
  editables[975 | E1]->show(13);
  editables[976 | E1]->show(14);
  editables[977 | E1]->show(15);
  editables[978 | E1]->show(16);
  editables[979 | E1]->show(17);
}


void Z1Edit::showFxEnsemble() {
  Serial.println("showFxEnsemble()");
  s->clear();
  char numstr[5];
  sprintf(numstr, "FX%d", fxpage);
  s->label(0, numstr);
  s->value(0, "Ensemb");
  editables[980 | E1]->show(1);
  editables[981 | E1]->show(2);
  editables[982 | E1]->show(3);
  editables[983 | E1]->show(4);
  editables[984 | E1]->show(5);
  editables[985 | E1]->show(6);
  editables[986 | E1]->show(7);
  editables[987 | E1]->show(8);
  editables[988 | E1]->show(9);
  editables[989 | E1]->show(10);
}
void Z1Edit::showFxRotary() {
  Serial.println("showFxRotary()");
  s->clear();
  char numstr[5];
  sprintf(numstr, "FX%d", fxpage);
  s->label(0, numstr);
  s->value(0, "Rotary");
  editables[990 | E1]->show(1);
  editables[991 | E1]->show(2);
  editables[992 | E1]->show(3);
  editables[993 | E1]->show(4);
  editables[994 | E1]->show(5);
  editables[995 | E1]->show(6);
  editables[996 | E1]->show(7);
  editables[997 | E1]->show(8);
  editables[998 | E1]->show(9);
  editables[999 | E1]->show(10);
  editables[1000 | E1]->show(11);
  editables[1001 | E1]->show(12);
  editables[1002 | E1]->show(13);
  editables[1003 | E1]->show(14);
}

void Z1Edit::sendZ1ProgParm(uint8_t channel, uint8_t group, int param, int val) {
  Serial.print("sendZ1ProgParm ");
  uint8_t plsb, pmsb, vlsb, vmsb;
  plsb = param & 0x7f;
  pmsb = (param >> 7) & 0x7f;
  vlsb = val & 0x7f;
  vmsb = (val >> 7) & 0x7f;
  uint8_t d[11];
  d[0] = 0xf0;
  d[1] = 0x42;
  d[2] = 0x30 | channel;
  d[3] = 0x46;
  d[4] = 0x41;
  d[5] = group;
  d[6] = plsb;
  d[7] = pmsb;
  d[8] = vlsb;
  d[9] = vmsb;
  d[10] = 0xf7;

  for (int i = 0; i < 11; i++) {
    Serial.printf("%02x ", d[i]);
    z1Write(d[i]);
  }
  Serial.println("");
}

void Z1Edit::getCurrPatch() {
  Serial.println("getCurrPatch()");
  uint8_t d[7];
  d[0] = 0xf0;
  d[1] = 0x42;
  d[2] = 0x30;
  d[3] = 0x46;
  d[4] = 0x10;
  d[5] = 0x00;
  d[6] = 0xf7;
  for (int i = 0; i < 7; i++) z1Write(d[i]);
}

void Z1Edit::getCurrArp() {
  Serial.println("getCurrArp()");
  uint8_t d[7];
  d[0] = 0xf0;
  d[1] = 0x42;
  d[2] = 0x30;
  d[3] = 0x46;
  d[4] = 0x36;
  d[5] = 0x00;
  d[6] = 0xf7;
  for (int i = 0; i < 7; i++) z1Write(d[i]);
}

// Switches the Z1 between Program and Multi mode. Function 4Eh is not in the
// Korg MIDI implementation list; these are the messages the Z1 itself sends
// when the mode is changed on its panel (captured 24 November 2026), and it
// obeys them when they are sent to it. It answers with the arpeggio pattern
// of the new mode (a 298-byte dump).
void Z1Edit::setZ1Mode(bool multi) {
  Serial.println(multi ? "setZ1Mode(): Multi" : "setZ1Mode(): Program");
  const uint8_t toMulti[8] = { 0xf0, 0x42, 0x30, 0x46, 0x4e, 0x12, 0x01, 0xf7 };
  const uint8_t toProgram[8] = { 0xf0, 0x42, 0x30, 0x46, 0x4e, 0x01, 0x00, 0xf7 };
  z1Write(multi ? toMulti : toProgram, 8);
}

void Z1Edit::storeForMPE() {
  Serial.println("storeForMPE()");
  uint8_t d[8];
  d[0] = 0xf0;
  d[1] = 0x42;
  d[2] = 0x30;
  d[3] = 0x46;
  d[4] = 0x11;
  d[5] = 0x00;
  d[6] = 0x7f;  // A127
  d[7] = 0xf7;
  for (int i = 0; i < 8; i++) z1Write(d[i]);
  Serial.println("sent()");
}


void Z1Edit::parseZ1SysExParam(const uint8_t* sx, int len) {
  if (sx[0] == 0x42 && sx[2] == 0x46 && sx[3] == 0x41) {
    
    uint8_t group = sx[4];
    if (group != 1) return;   // only program parameters are in the editor
    int param = sx[5] | (sx[6] << 7);
    uint8_t val = sx[7] | (sx[8] << 7);

    Serial.printf("parseZ1SysExParam group %d param %d %val %d\r\n",group,param,val);
    //try {
    if (editables.count(param)>0)
    editables[param]->set(val);
    //} catch (...) {
    
    else Serial.println("param not found");
    //}
  }
}

void FLASHMEM Z1Edit::parseZ1SysExPreset(const uint8_t* sx, int len) {

  int start = 5;
  int blocks = (len - start) / 8 - 1;
  uint8_t block[8];
  uint8_t d[7];
  uint8_t patch[576];
  int p = 0;
  uint8_t osc1_type;
  EditableValue::updateState(false);


  if (sx[3] == 0x40) {
    Serial.println("prg data");

    for (int j = 0; j <= blocks; j++) {
      block[0] = sx[start++];
      block[1] = sx[start++];
      block[2] = sx[start++];
      block[3] = sx[start++];
      block[4] = sx[start++];
      block[5] = sx[start++];
      block[6] = sx[start++];
      block[7] = sx[start++];

      //for(int i=0;i<8;i++) Serial.printf("%02x ", block[i]);
      //Serial.println("");

      d[0] = block[1] | ((block[0] & 0x01) << 7);
      d[1] = block[2] | ((block[0] & 0x02) << 6);
      d[2] = block[3] | ((block[0] & 0x04) << 5);
      d[3] = block[4] | ((block[0] & 0x08) << 4);
      d[4] = block[5] | ((block[0] & 0x10) << 3);
      d[5] = block[6] | ((block[0] & 0x20) << 2);
      d[6] = block[7] | ((block[0] & 0x40) << 1);

      //Serial.printf("%d :: ", p);
      for (int i = 0; i < 7; i++) {
        patch[p++] = d[i];
        //Serial.printf("%02x ", d[i]);
      }
      //Serial.println("");
    }

    z1Patch zp;
    memcpy(zp.name, &patch[0], 16);





    Serial.printf("Category: %d %s\n", patch[16], z1Categories[patch[16]].c_str());
    /*
    17 user group
    18:("", Bits
        [ ("hold", bits 1)
        , ("key priority", enum_bits 2 ["last", "low", "high"])
        , ("voice assign mode", enum_bits 5
            ["mono multi", "mono single", "poly"])
        ])
    19  ("retrigger controllor", enum mod_source_list_2)
    20  ("retrigger control threshold", ranged 1 127)
    21 ("", Bits
        [ ("unison type", enum_bits 2 ["off", "2", "3", "6"])
        , ("unison sw", bool_bit)
        , ("unison mode", enum_bits 5 ["fixed", "dynamic"])
        ])
      ("unison detune", unsigned 99)
    */
    zp.unison = patch[21];
    unisonFromPatch(patch[21]);
    zp.detune = patch[22];
    /* 23 ("", Bits
        [ ("scale key", enum_bits 4 scale_keys)
        , ("scale type", enum_bits 4 scale_types)
        ])
    24 ("random pitch intensity", unsigned 99)
    */
    zp.detune = patch[24];

    /*("eg", List 4 eg_spec)
    , ("lfo", List 4 lfo_spec)*/
    Serial.printf("EG1 Srt: %d\n", patch[25]);
    int u = 31;
    int v = 25;
    editables[u++]->set(patch[v++]);  //EG1 Srt
    editables[u++]->set(patch[v++]);  //EG1 Attack
    editables[u++]->set(patch[v++]);  //EG1 AttackLevel
    editables[u++]->set(patch[v++]);  //EG1 Decay
    editables[u++]->set(patch[v++]);  //EG1 Break
    editables[u++]->set(patch[v++]);  //EG1 Slope
    editables[u++]->set(patch[v++]);  //EG1 sustain
    editables[u++]->set(patch[v++]);  //EG1 Release Time
    editables[u++]->set(patch[v++]);  //EG1 Release Level
    editables[u++]->set(patch[v++]);  //EG1 Level Mod source
    editables[u++]->set(patch[v++]);  //EG1 Level Mod Level
    editables[u++]->set(patch[v++]);  //EG1 Velocity control
    editables[u++]->set(patch[v++]);  //EG1 Time Mod source
    editables[u++]->set(patch[v++]);  //EG1 Time Mod Level
    editables[u++]->set(patch[v++]);  //EG1 Node Time Mod src
    editables[u++]->set(patch[v++]);  //EG1 att Mod int
    editables[u++]->set(patch[v++]);  //EG1 dec Mod int
    editables[u++]->set(patch[v++]);  //EG1 slope Mod int
    editables[u++]->set(patch[v++]);  //EG1 rel Mod int

    editables[u++]->set(patch[v++]);  //EG2 Srt
    editables[u++]->set(patch[v++]);  //EG2 Attack
    editables[u++]->set(patch[v++]);  //EG2 AttackLevel
    editables[u++]->set(patch[v++]);  //EG2 Decay
    editables[u++]->set(patch[v++]);  //EG2 Break
    editables[u++]->set(patch[v++]);  //EG2 Slope
    editables[u++]->set(patch[v++]);  //EG2 sustain
    editables[u++]->set(patch[v++]);  //EG2 Release Time
    editables[u++]->set(patch[v++]);  //EG2 Release Level
    editables[u++]->set(patch[v++]);  //EG2 Level Mod source
    editables[u++]->set(patch[v++]);  //EG2 Level Mod Level
    editables[u++]->set(patch[v++]);  //EG2 Velocity control
    editables[u++]->set(patch[v++]);  //EG2 Time Mod source
    editables[u++]->set(patch[v++]);  //EG2 Time Mod Level
    editables[u++]->set(patch[v++]);  //EG1 Node Time Mod src
    editables[u++]->set(patch[v++]);  //EG2 att Mod int
    editables[u++]->set(patch[v++]);  //EG2 dec Mod int
    editables[u++]->set(patch[v++]);  //EG2 slope Mod int
    editables[u++]->set(patch[v++]);  //EG2 rel Mod int

    //EG3
    editables[u++]->set(patch[v++]);  //EG1 Srt
    editables[u++]->set(patch[v++]);  //EG1 Attack
    editables[u++]->set(patch[v++]);  //EG1 AttackLevel
    editables[u++]->set(patch[v++]);  //EG1 Decay
    editables[u++]->set(patch[v++]);  //EG1 Break
    editables[u++]->set(patch[v++]);  //EG1 Slope
    editables[u++]->set(patch[v++]);  //EG1 sustain
    editables[u++]->set(patch[v++]);  //EG1 Release Time
    editables[u++]->set(patch[v++]);  //EG1 Release Level
    editables[u++]->set(patch[v++]);  //EG1 Level Mod source
    editables[u++]->set(patch[v++]);  //EG1 Level Mod Level
    editables[u++]->set(patch[v++]);  //EG1 Velocity control
    editables[u++]->set(patch[v++]);  //EG1 Time Mod source
    editables[u++]->set(patch[v++]);  //EG1 Time Mod Level
    editables[u++]->set(patch[v++]);  //EG1 Node Time Mod src
    editables[u++]->set(patch[v++]);  //EG1 att Mod int
    editables[u++]->set(patch[v++]);  //EG1 dec Mod int
    editables[u++]->set(patch[v++]);  //EG1 slope Mod int
    editables[u++]->set(patch[v++]);  //EG1 rel Mod int

    //EG4
    editables[u++]->set(patch[v++]);  //EG2 Srt
    editables[u++]->set(patch[v++]);  //EG2 Attack
    editables[u++]->set(patch[v++]);  //EG2 AttackLevel
    editables[u++]->set(patch[v++]);  //EG2 Decay
    editables[u++]->set(patch[v++]);  //EG2 Break
    editables[u++]->set(patch[v++]);  //EG2 Slope
    editables[u++]->set(patch[v++]);  //EG2 sustain
    editables[u++]->set(patch[v++]);  //EG2 Release Time
    editables[u++]->set(patch[v++]);  //EG2 Release Level
    editables[u++]->set(patch[v++]);  //EG2 Level Mod source
    editables[u++]->set(patch[v++]);  //EG2 Level Mod Level
    editables[u++]->set(patch[v++]);  //EG2 Velocity control
    editables[u++]->set(patch[v++]);  //EG2 Time Mod source
    editables[u++]->set(patch[v++]);  //EG2 Time Mod Level
    editables[u++]->set(patch[v++]);  //EG1 Node Time Mod src
    editables[u++]->set(patch[v++]);  //EG2 att Mod int
    editables[u++]->set(patch[v++]);  //EG2 dec Mod int
    editables[u++]->set(patch[v++]);  //EG2 slope Mod int
    editables[u++]->set(patch[v++]);  //EG2 rel Mod int

    //LFO
    uint8_t lfo_type, msync;
    u = 107;
    lfo_type = patch[v++];
    Serial.printf("lfo1 type: %x\n", lfo_type);
    editables[u++]->set(lfo_type & 0x3f);         //lfo wave form 107
    editables[u++]->set((lfo_type & 0xc0) >> 6);  //lfo key sync 108
    editables[u++]->set(patch[v++]);              //109
    editables[u++]->set(patch[v++]);              //110
    editables[u++]->set(patch[v++]);              //111
    editables[u++]->set(patch[v++]);              //112
    editables[u++]->set(patch[v++]);              //113
    editables[u++]->set(patch[v++]);              //114
    Serial.printf("u: %d  v: %d\n", u, v);
    editables[u++]->set(patch[v++]);  //115
    editables[u++]->set(patch[v++]);  //116
    editables[u++]->set(patch[v++]);  //117
    msync = patch[v++];
    Serial.printf("lfo1 msync: %x\n", msync);
    editables[u++]->set((msync & 0x80) >> 7);  //118
    editables[u++]->set((msync & 0x70) >> 4);  //119
    editables[u++]->set(msync & 0xf);          //120


    //LFO2
    lfo_type = patch[v++];
    Serial.printf("lfo1 type: %x\n", lfo_type);
    editables[u++]->set(lfo_type & 0x3f);         //lfo wave form 107
    editables[u++]->set((lfo_type & 0xc0) >> 6);  //lfo key sync 108
    editables[u++]->set(patch[v++]);              //109
    editables[u++]->set(patch[v++]);              //110
    editables[u++]->set(patch[v++]);              //111
    editables[u++]->set(patch[v++]);              //112
    editables[u++]->set(patch[v++]);              //113
    editables[u++]->set(patch[v++]);              //114
    Serial.printf("u: %d  v: %d\n", u, v);
    editables[u++]->set(patch[v++]);  //115
    editables[u++]->set(patch[v++]);  //116
    editables[u++]->set(patch[v++]);  //117
    msync = patch[v++];
    Serial.printf("lfo2 msync: %x\n", msync);
    editables[u++]->set((msync & 0x80) >> 7);
    editables[u++]->set((msync & 0x70) >> 4);  //119
    editables[u++]->set(msync & 0xf);
    //LFO3
    lfo_type = patch[v++];
    Serial.printf("lfo1 type: %x\n", lfo_type);
    editables[u++]->set(lfo_type & 0x3f);         //lfo wave form 107
    editables[u++]->set((lfo_type & 0xc0) >> 6);  //lfo key sync 108
    editables[u++]->set(patch[v++]);              //109
    editables[u++]->set(patch[v++]);              //110
    editables[u++]->set(patch[v++]);              //111
    editables[u++]->set(patch[v++]);              //112
    editables[u++]->set(patch[v++]);              //113
    editables[u++]->set(patch[v++]);              //114
    Serial.printf("u: %d  v: %d\n", u, v);
    editables[u++]->set(patch[v++]);  //115
    editables[u++]->set(patch[v++]);  //116
    editables[u++]->set(patch[v++]);  //117
    msync = patch[v++];
    Serial.printf("lfo3 msync: %x\n", msync);
    editables[u++]->set((msync & 0x80) >> 7);
    editables[u++]->set((msync & 0x70) >> 4);  //119
    editables[u++]->set(msync & 0xf);
    //LFO4
    lfo_type = patch[v++];
    Serial.printf("lfo4 type: %x\n", lfo_type);
    editables[u++]->set(lfo_type & 0x3f);         //lfo wave form 107
    editables[u++]->set((lfo_type & 0xc0) >> 6);  //lfo key sync 108
    editables[u++]->set(patch[v++]);              //109
    editables[u++]->set(patch[v++]);              //110
    editables[u++]->set(patch[v++]);              //111
    editables[u++]->set(patch[v++]);              //112
    editables[u++]->set(patch[v++]);              //113
    editables[u++]->set(patch[v++]);              //114
    Serial.printf("u: %d  v: %d\n", u, v);
    editables[u++]->set(patch[v++]);  //115
    editables[u++]->set(patch[v++]);  //116
    editables[u++]->set(patch[v++]);  //117
    msync = patch[v++];
    Serial.printf("lfo4 msync: %x\n", msync);
    editables[u++]->set((msync & 0x80) >> 7);
    editables[u++]->set((msync & 0x70) >> 4);  //119
    editables[u++]->set(msync & 0xf);


    //Pitch bend data, 3 bytes, ignore
    /*
    pitch_bend = ("pitch bend", SubSpec
    [ ("intensity +", ranged (-60) 24)
    , ("intensity -", ranged (-60) 24)
    , ("", Bits $
        let vals = ["0", "1/8", "1/4", "1/2"] ++ map showt [1..12] in
        [ ("step +", enum_bits 4 vals)
        , ("step -", enum_bits 4 vals)
        ])
    ])
    */
    v++;
    v++;
    v++;
    /*
    , ("common pitch mod source", enum mod_source_list_1)
    , mod_intensity "common pitch"

    2 bytes
    */
    v++;
    v++;

    /*Portamento
    
    , ("portamento", SubSpec
        [ ("", Bits
            [ ("sw", bool_bit)
            , ("mode", enum_bits 7 ["normal", "fingered"])
            ])
        , ("time", unsigned 99)
        , ("mod source", enum mod_source_list_1)
        , mod_intensity ""
        ])

    4 bytes???

    */
    v++;
    v++;
    v++;
    v++;
    Serial.printf("u: %d  v: %d\n", u, v);

    //OSC 1
    osc1_type = patch[v++];
    Serial.printf("ot1 %d\n", osc1_type);
    uint8_t osc1_d[38];
    memset(osc1_d, 0, 38);
    editables[174]->set(osc1_type);
    editables[175]->set(patch[v++]);  //octave 175
    editables[176]->set(patch[v++]);  //semi 176
    editables[177]->set(patch[v]);    //fine 177
    v = v + 11;
    /*
    editables[u++]->set(patch[v++]); //offset 178
    editables[u++]->set(patch[v++]); //center 179
    editables[u++]->set(patch[v++]); //lslope 180
    editables[u++]->set(patch[v++]); //hslope 181
    editables[u++]->set(patch[v++]); //mod1 src 182
    editables[u++]->set(patch[v++]); //int 183
    editables[u++]->set(patch[v++]); //mod1 int src 184
    editables[u++]->set(patch[v++]); //int 185
    editables[u++]->set(patch[v++]); //mod2 src 186
    editables[u++]->set(patch[v++]); //int 187
    */
    //38 bytes
    Serial.printf("o1 patch data from  %d \n", v);
    memcpy(osc1_d, &patch[v], 38);
    v = v + 38;
    Serial.printf("u: %d  v: %d\n", u, v);
    //OSC 2
    uint8_t osc2_type = patch[v++];
    Serial.printf("ot2 %d\n", osc2_type);
    uint8_t osc2_d[38];
    memset(osc2_d, 0, 38);
    editables[188]->set(osc2_type);
    editables[189]->set(patch[v++]);  //octave
    editables[190]->set(patch[v++]);  //semi
    editables[191]->set(patch[v]);    //fine
    v = v + 11;
    /*
    editables[u++]->set(patch[v++]); //center
    editables[u++]->set(patch[v++]); //lslope
    editables[u++]->set(patch[v++]); //hslope
    editables[u++]->set(patch[v++]); //mod1 src
    editables[u++]->set(patch[v++]); //int
    editables[u++]->set(patch[v++]); //mod1 int src
    editables[u++]->set(patch[v++]); //int
    editables[u++]->set(patch[v++]); //mod2 src
    editables[u++]->set(patch[v++]); //int
    */
    //38 bytes
    Serial.printf("o2 patch data from  %d \n", v);
    memcpy(osc2_d, &patch[v], 38);
    v = v + 38;
    /*
    for (int i = 0; i < 38; i++) {
      Serial.printf("%02x ", osc1_d[i]);
    }
    Serial.println("");
    for (int i = 0; i < 38; i++) {
      Serial.printf("%02x ", osc2_d[i]);
    }
    Serial.println("");
    */


    u = O1;
    int osc_type = osc1_type;
    uint8_t* osc_d = osc1_d;
    for (int i = 0; i <= 1; i++) {
      int j = 0;
      if (i == 1) {
        osc_d = osc2_d;
        osc_type = osc2_type;
        u = O2;
      }
      Serial.printf("osc_type %d\n", osc_type);
      switch (osc_type) {
        case 0:  //Std
          editables[464 | u]->set(osc_d[j++]);
          editables[465 | u]->set(osc_d[j++]);
          editables[466 | u]->set(osc_d[j++]);
          editables[467 | u]->set(osc_d[j++]);
          editables[468 | u]->set(osc_d[j++]);
          editables[469 | u]->set(osc_d[j++]);
          editables[470 | u]->set(osc_d[j++]);
          Serial.printf("lfo %d\n", osc_d[j] - 6);
          editables[471 | u]->set(osc_d[j++] - 6);  //TODO make this send correctly

          editables[472 | u]->set(osc_d[j++]);
          editables[473 | u]->set(osc_d[j++]);
          editables[474 | u]->set(osc_d[j++]);
          editables[475 | u]->set(osc_d[j++]);
          editables[476 | u]->set(osc_d[j++]);
          editables[477 | u]->set(osc_d[j++]);
          editables[478 | u]->set(osc_d[j++]);
          editables[479 | u]->set(osc_d[j++]);
          editables[480 | u]->set(osc_d[j++]);
          editables[481 | u]->set(osc_d[j++]);
          editables[482 | u]->set(osc_d[j++]);
          editables[483 | u]->set(osc_d[j++]);
          editables[484 | u]->set(osc_d[j++]);
          editables[485 | u]->set(osc_d[j]);
          Serial.printf("stdosc done\n");

          break;
        case 1:  //Comb
          editables[486 | u]->set(osc_d[j++]);
          editables[487 | u]->set(osc_d[j++]);
          editables[488 | u]->set(osc_d[j++]);
          editables[489 | u]->set(osc_d[j++]);
          editables[490 | u]->set(osc_d[j++]);
          editables[491 | u]->set(osc_d[j++]);
          editables[492 | u]->set(osc_d[j++]);
          editables[493 | u]->set(osc_d[j++]);
          editables[494 | u]->set(osc_d[j++]);
          editables[495 | u]->set(osc_d[j++]);
          editables[496 | u]->set(osc_d[j++]);
          editables[497 | u]->set(osc_d[j++]);
          editables[498 | u]->set(osc_d[j++]);
          editables[499 | u]->set(osc_d[j]);
          break;
        case 2:  //VPM
          editables[500 | u]->set(osc_d[j++]);
          editables[501 | u]->set(osc_d[j++]);
          editables[502 | u]->set(osc_d[j++]);
          editables[503 | u]->set(osc_d[j++]);
          editables[504 | u]->set(osc_d[j++]);
          editables[505 | u]->set(osc_d[j++]);
          editables[506 | u]->set(osc_d[j++]);
          editables[507 | u]->set(osc_d[j++]);
          editables[508 | u]->set(osc_d[j++]);
          editables[509 | u]->set(osc_d[j++]);
          editables[510 | u]->set(osc_d[j++]);
          editables[511 | u]->set(osc_d[j++]);
          editables[512 | u]->set(osc_d[j++]);
          editables[513 | u]->set(osc_d[j++]);
          editables[514 | u]->set(osc_d[j++]);
          editables[515 | u]->set(osc_d[j++]);
          editables[516 | u]->set(osc_d[j++]);
          editables[517 | u]->set(osc_d[j++]);
          editables[518 | u]->set(osc_d[j++]);
          editables[519 | u]->set(osc_d[j++]);
          editables[520 | u]->set(osc_d[j++]);
          editables[521 | u]->set(osc_d[j++]);
          editables[522 | u]->set(osc_d[j++]);
          editables[523 | u]->set(osc_d[j++]);
          editables[524 | u]->set(osc_d[j]);
          break;
        case 3:  //Reso
          editables[525 | u]->set(osc_d[j++]);
          editables[526 | u]->set(osc_d[j++]);
          editables[527 | u]->set(osc_d[j++]);
          editables[528 | u]->set(osc_d[j++]);
          editables[529 | u]->set(osc_d[j++]);
          editables[530 | u]->set(osc_d[j++]);
          editables[531 | u]->set(osc_d[j++]);
          editables[532 | u]->set(osc_d[j++]);
          editables[533 | u]->set(osc_d[j++]);
          editables[534 | u]->set(osc_d[j++]);
          editables[535 | u]->set(osc_d[j++]);
          editables[536 | u]->set(osc_d[j++]);
          editables[537 | u]->set(osc_d[j++]);
          editables[538 | u]->set(osc_d[j++]);
          editables[539 | u]->set(osc_d[j++]);
          editables[540 | u]->set(osc_d[j++]);
          editables[541 | u]->set(osc_d[j++]);
          editables[542 | u]->set(osc_d[j++]);
          editables[543 | u]->set(osc_d[j++]);
          editables[544 | u]->set(osc_d[j++]);
          editables[545 | u]->set(osc_d[j++]);
          editables[546 | u]->set(osc_d[j++]);
          editables[547 | u]->set(osc_d[j++]);
          editables[548 | u]->set(osc_d[j++]);
          editables[549 | u]->set(osc_d[j++]);
          editables[550 | u]->set(osc_d[j++]);
          editables[551 | u]->set(osc_d[j++]);
          editables[552 | u]->set(osc_d[j++]);
          editables[553 | u]->set(osc_d[j++]);
          editables[554 | u]->set(osc_d[j++]);
          editables[555 | u]->set(osc_d[j++]);
          editables[556 | u]->set(osc_d[j]);
          break;
        case 4:  //Ring
          editables[557 | u]->set(osc_d[j++]);
          editables[558 | u]->set(osc_d[j++]);
          editables[559 | u]->set(osc_d[j++]);
          editables[560 | u]->set(osc_d[j++]);
          editables[561 | u]->set(osc_d[j++]);
          editables[562 | u]->set(osc_d[j++]);
          editables[563 | u]->set(osc_d[j++]);
          editables[564 | u]->set(osc_d[j++]);
          editables[565 | u]->set(osc_d[j]);
          break;
        case 5:  //Cross
          editables[566 | u]->set(osc_d[j++]);
          editables[567 | u]->set(osc_d[j++]);
          editables[568 | u]->set(osc_d[j++]);
          editables[569 | u]->set(osc_d[j++]);
          editables[570 | u]->set(osc_d[j++]);
          editables[571 | u]->set(osc_d[j++]);
          editables[572 | u]->set(osc_d[j++]);
          editables[573 | u]->set(osc_d[j]);
          break;
        case 6:  //Sync
          editables[574 | u]->set(osc_d[j++]);
          editables[575 | u]->set(osc_d[j++]);
          editables[576 | u]->set(osc_d[j]);
          break;
        case 7:  //Organ
          editables[577 | u]->set(osc_d[j++]);
          editables[578 | u]->set(osc_d[j++]);
          editables[579 | u]->set(osc_d[j++]);
          editables[580 | u]->set(osc_d[j++]);
          editables[581 | u]->set(osc_d[j++]);
          editables[582 | u]->set(osc_d[j++]);
          editables[583 | u]->set(osc_d[j++]);
          editables[584 | u]->set(osc_d[j++]);
          editables[585 | u]->set(osc_d[j++]);
          editables[586 | u]->set(osc_d[j++]);
          editables[587 | u]->set(osc_d[j++]);
          editables[588 | u]->set(osc_d[j++]);
          editables[589 | u]->set(osc_d[j++]);
          editables[590 | u]->set(osc_d[j++]);
          editables[591 | u]->set(osc_d[j++]);
          editables[592 | u]->set(osc_d[j++]);
          editables[593 | u]->set(osc_d[j++]);
          editables[594 | u]->set(osc_d[j++]);
          editables[595 | u]->set(osc_d[j++]);
          editables[596 | u]->set(osc_d[j++]);
          editables[597 | u]->set(osc_d[j++]);
          editables[598 | u]->set(osc_d[j++]);
          editables[599 | u]->set(osc_d[j++]);
          editables[600 | u]->set(osc_d[j++]);
          editables[601 | u]->set(osc_d[j]);
          break;
        case 8:  //EP
          editables[602 | u]->set(osc_d[j++]);
          editables[603 | u]->set(osc_d[j++]);
          editables[604 | u]->set(osc_d[j++]);
          editables[605 | u]->set(osc_d[j++]);
          editables[606 | u]->set(osc_d[j++]);
          editables[607 | u]->set(osc_d[j++]);
          editables[608 | u]->set(osc_d[j++]);
          editables[609 | u]->set(osc_d[j++]);
          editables[610 | u]->set(osc_d[j++]);
          editables[611 | u]->set(osc_d[j++]);
          editables[612 | u]->set(osc_d[j++]);
          editables[613 | u]->set(osc_d[j++]);
          editables[614 | u]->set(osc_d[j++]);
          editables[615 | u]->set(osc_d[j]);
          break;
        case 9:  //Brass
          editables[616 | u]->set(osc_d[j++]);
          j++;  //617,618 pitch bend
          editables[619 | u]->set(osc_d[j++]);
          editables[620 | u]->set(osc_d[j++]);
          editables[621 | u]->set(osc_d[j++]);
          editables[622 | u]->set(osc_d[j++]);
          editables[623 | u]->set(osc_d[j++]);
          editables[624 | u]->set(osc_d[j++]);
          j++;  //reserved
          editables[626 | u]->set(osc_d[j++]);
          editables[627 | u]->set(osc_d[j++]);
          editables[628 | u]->set(osc_d[j++]);
          j++;  //reserved 629
          j++;  //reserved 630
          j++;  //reserved 631
          editables[632 | u]->set(osc_d[j++]);
          editables[633 | u]->set(osc_d[j++]);
          editables[634 | u]->set(osc_d[j++]);
          j++;  //reserved 635
          j++;  //reserved 636
          j++;  //reserved 637
          j++;  //reserved 638
          j++;  //reserved 639
          j++;  //reserved 640
          j++;  //reserved 641
          j++;  //reserved 642
          j++;  //reserved 643
          j++;  //reserved 644
          editables[645 | u]->set(osc_d[j++]);
          editables[646 | u]->set(osc_d[j++]);
          editables[647 | u]->set(osc_d[j++]);
          editables[648 | u]->set(osc_d[j++]);
          j++;  //reserved 649
          j++;  //reserved 650
          j++;  //reserved 651
          j++;  //reserved 652
          //reserved 653
          break;
        case 10:  //reed
          editables[654 | u]->set(osc_d[j++]);

          j++;  //655,656 pitch bend
          editables[657 | u]->set(osc_d[j++]);
          editables[658 | u]->set(osc_d[j++]);
          editables[659 | u]->set(osc_d[j++]);
          editables[660 | u]->set(osc_d[j++]);
          editables[661 | u]->set(osc_d[j++]);
          editables[662 | u]->set(osc_d[j++]);
          j++;  //reserved 663
          j++;  //reserved 664
          j++;  //reserved 665
          j++;  //reserved 666
          j++;  //reserved 667
          editables[668 | u]->set(osc_d[j++]);
          j++;  //reserved 669
          j++;  //reserved 670
          j++;  //reserved 671
          j++;  //reserved 672
          j++;  //reserved 673
          j++;  //reserved 674
          j++;  //reserved 675
          j++;  //reserved 676
          j++;  //reserved 677
          j++;  //reserved 678
          j++;  //reserved 679
          j++;  //reserved 680
          editables[681 | u]->set(osc_d[j++]);
          editables[682 | u]->set(osc_d[j++]);
          editables[683 | u]->set(osc_d[j++]);
          editables[684 | u]->set(osc_d[j++]);
          editables[685 | u]->set(osc_d[j++]);
          editables[686 | u]->set(osc_d[j++]);
          editables[687 | u]->set(osc_d[j++]);
          editables[689 | u]->set(osc_d[j++]);


          editables[690 | u]->set((osc_d[j] & 0x80) >> 7);
          editables[691 | u]->set(osc_d[j++] & 0x7f);

          editables[692 | u]->set(osc_d[j++]);
          editables[693 | u]->set(osc_d[j]);


          break;
        case 11:  //plucked
          editables[694 | u]->set(osc_d[j++]);
          editables[695 | u]->set(osc_d[j++]);
          editables[696 | u]->set(osc_d[j++]);
          editables[697 | u]->set(osc_d[j++]);
          editables[698 | u]->set(osc_d[j++]);
          editables[699 | u]->set(osc_d[j++]);
          editables[700 | u]->set(osc_d[j++]);
          editables[701 | u]->set(osc_d[j++]);
          editables[702 | u]->set(osc_d[j++]);
          editables[703 | u]->set(osc_d[j++]);
          editables[704 | u]->set(osc_d[j++]);
          editables[705 | u]->set(osc_d[j++]);
          editables[706 | u]->set(osc_d[j++]);
          editables[707 | u]->set(osc_d[j++]);
          editables[708 | u]->set(osc_d[j++]);
          editables[709 | u]->set(osc_d[j++]);
          editables[710 | u]->set(osc_d[j++]);
          editables[711 | u]->set(osc_d[j++]);
          editables[712 | u]->set(osc_d[j++]);
          editables[713 | u]->set(osc_d[j++]);
          editables[714 | u]->set(osc_d[j++]);
          editables[715 | u]->set(osc_d[j++]);
          editables[716 | u]->set(osc_d[j++]);
          editables[717 | u]->set(osc_d[j++]);
          editables[718 | u]->set(osc_d[j++]);
          editables[719 | u]->set(osc_d[j++]);
          editables[720 | u]->set(osc_d[j++]);
          editables[721 | u]->set(osc_d[j++]);
          editables[722 | u]->set(osc_d[j++]);
          editables[723 | u]->set(osc_d[j++]);
          editables[724 | u]->set(osc_d[j]);
          break;
        case 12:  //bow
          editables[725 | u]->set(osc_d[j++]);
          editables[726 | u]->set(osc_d[j++]);
          editables[727 | u]->set(osc_d[j++]);
          editables[728 | u]->set(osc_d[j++]);
          editables[729 | u]->set(osc_d[j++]);
          editables[730 | u]->set(osc_d[j++]);
          editables[731 | u]->set(osc_d[j++]);
          editables[732 | u]->set(osc_d[j++]);
          editables[733 | u]->set(osc_d[j++]);
          editables[734 | u]->set(osc_d[j++]);
          editables[735 | u]->set(osc_d[j++]);
          editables[736 | u]->set(osc_d[j++]);
          editables[737 | u]->set(osc_d[j++]);
          editables[738 | u]->set(osc_d[j++]);
          editables[739 | u]->set(osc_d[j++]);
          editables[740 | u]->set(osc_d[j++]);
          editables[741 | u]->set(osc_d[j++]);
          editables[742 | u]->set(osc_d[j++]);
          editables[743 | u]->set(osc_d[j++]);
          editables[744 | u]->set(osc_d[j++]);
          editables[745 | u]->set(osc_d[j++]);
          editables[746 | u]->set(osc_d[j++]);
          editables[747 | u]->set(osc_d[j++]);
          editables[748 | u]->set(osc_d[j++]);
          editables[749 | u]->set(osc_d[j++]);
          editables[750 | u]->set(osc_d[j++]);
          editables[751 | u]->set(osc_d[j++]);
          editables[752 | u]->set(osc_d[j++]);
          editables[753 | u]->set(osc_d[j++]);
          editables[754 | u]->set(osc_d[j]);
          break;
      }
    }  //oscs
    Serial.printf("oscs done\r\n");
    //subosc
    editables[202]->set(patch[v++]);
    editables[203]->set(patch[v++]);
    editables[204]->set(patch[v++]);
    editables[205]->set(patch[v++]);
    editables[206]->set(patch[v++]);
    editables[207]->set(patch[v++]);
    editables[208]->set(patch[v++]);
    editables[209]->set(patch[v++]);
    editables[210]->set(patch[v++]);
    editables[211]->set(patch[v++]);
    editables[212]->set(patch[v++]);
    editables[213]->set(patch[v++]);
    editables[214]->set(patch[v++]);
    editables[215]->set(patch[v++]);
    Serial.printf("subosc done\r\n");
    //noise
    editables[216]->set(patch[v++]);
    editables[217]->set(patch[v++]);
    editables[218]->set(patch[v++]);
    editables[219]->set(patch[v++]);
    editables[220]->set(patch[v++]);
    editables[221]->set(patch[v++]);
    editables[222]->set(patch[v++]);
    editables[223]->set(patch[v++]);
    Serial.printf("noise done\r\n");

    //mixer, ignored
    v = v + 31;

    //filt
    //common
    editables[258]->set(patch[v] & 0x03);
    editables[259]->set((patch[v] & 0x04) >> 2);
    editables[260]->set((patch[v++] & 0x18) >> 4);

    //filt1
    Serial.printf("u %d v %d %x\r\n", u, v, patch[v]);
    editables[261]->set(patch[v++]);
    editables[262]->set(patch[v++]);
    editables[263]->set(patch[v++]);
    editables[264]->set(patch[v++]);
    editables[265]->set(patch[v++]);
    editables[266]->set(patch[v++]);
    editables[267]->set(patch[v++]);
    editables[268]->set(patch[v++]);
    editables[269]->set(patch[v++]);
    editables[270]->set(patch[v++]);
    editables[271]->set(patch[v++]);
    editables[272]->set(patch[v++]);
    editables[273]->set(patch[v++]);
    editables[274]->set(patch[v++]);
    editables[275]->set(patch[v++]);
    editables[276]->set(patch[v++]);
    editables[277]->set(patch[v++]);
    editables[278]->set(patch[v++]);
    editables[279]->set(patch[v++]);
    editables[280]->set(patch[v++]);
    editables[281]->set(patch[v++]);
    editables[282]->set(patch[v++]);
    editables[283]->set(patch[v++]);
    editables[284]->set(patch[v++]);
    editables[285]->set(patch[v++]);
    editables[286]->set(patch[v++]);
    editables[287]->set(patch[v++]);
    Serial.printf("u %d v %d %x\r\n", u, v, patch[v]);
    //filt2
    editables[288]->set(patch[v++]);
    editables[289]->set(patch[v++]);
    editables[290]->set(patch[v++]);
    editables[291]->set(patch[v++]);
    editables[292]->set(patch[v++]);
    editables[293]->set(patch[v++]);
    editables[294]->set(patch[v++]);
    editables[295]->set(patch[v++]);
    editables[296]->set(patch[v++]);
    editables[297]->set(patch[v++]);
    editables[298]->set(patch[v++]);
    editables[299]->set(patch[v++]);
    editables[300]->set(patch[v++]);
    editables[301]->set(patch[v++]);
    editables[302]->set(patch[v++]);
    editables[303]->set(patch[v++]);
    editables[304]->set(patch[v++]);
    editables[305]->set(patch[v++]);
    editables[306]->set(patch[v++]);
    editables[307]->set(patch[v++]);
    editables[308]->set(patch[v++]);
    editables[309]->set(patch[v++]);
    editables[310]->set(patch[v++]);
    editables[311]->set(patch[v++]);
    editables[312]->set(patch[v++]);
    editables[313]->set(patch[v++]);
    editables[314]->set(patch[v++]);
    Serial.printf("filt done\r\n");

    editables[315]->set(patch[v++]);
    editables[316]->set(patch[v++]);
    editables[317]->set(patch[v++]);
    editables[318]->set(patch[v++]);
    editables[319]->set(patch[v++]);
    editables[320]->set(patch[v++]);
    v++;
    editables[322]->set(patch[v++]);
    editables[323]->set(patch[v++]);

    editables[324]->set(patch[v++]);
    editables[325]->set(patch[v++]);
    editables[326]->set(patch[v++]);
    editables[327]->set(patch[v++]);
    editables[328]->set(patch[v++]);
    editables[329]->set(patch[v++]);
    v++;
    editables[331]->set(patch[v++]);
    editables[332]->set(patch[v++]);



    Serial.printf("amp done\r\n");
    //amp generic TODO
    v = 385;  //v + 20;  //333 reserved included
    u = 334;
    //amp EG
    Serial.printf("u %d v %d %x\r\n", u, v, patch[v]);
    editables[u++]->set(patch[v++]);  //EG2 Attack
    editables[u++]->set(patch[v++]);  //EG2 AttackLevel
    editables[u++]->set(patch[v++]);  //EG2 Decay
    editables[u++]->set(patch[v++]);  //EG2 Break
    editables[u++]->set(patch[v++]);  //EG2 Slope
    editables[u++]->set(patch[v++]);  //EG2 sustain
    editables[u++]->set(patch[v++]);  //EG2 Release Time
    Serial.printf("u %d v %d %x\r\n", u, v, patch[v]);
#if 1
    u++;
    v++;  //341 reserved
    Serial.printf("u %d v %d %x\r\n", u, v, patch[v]);
    editables[u++]->set(patch[v++]);  //EG2 Level Mod source
    editables[u++]->set(patch[v++]);  //EG2 Level Mod Level
    editables[u++]->set(patch[v++]);  //EG2 Velocity control
    editables[u++]->set(patch[v++]);  //EG2 Time Mod source
    editables[u++]->set(patch[v++]);  //EG2 Time Mod Level
    editables[u++]->set(patch[v++]);  //EG1 Node Time Mod src
    editables[u++]->set(patch[v++]);  //EG2 att Mod int
    editables[u++]->set(patch[v++]);  //EG2 dec Mod int
    editables[u++]->set(patch[v++]);  //EG2 slope Mod int
    editables[u++]->set(patch[v++]);    //EG2 rel Mod int
    Serial.printf("amp done\r\n");



    v = v + 4;


    uint8_t fx1_d[22];
    uint8_t fx2_d[22];
    uint8_t fxm_d[18];

    //effect
    editables[356]->set(patch[v++]);  //fx send
    editables[357]->set(patch[v++]);
    editables[358]->set(patch[v++]);
    Serial.printf("v %d\r\n", v);
    uint8_t fx1 = patch[v++];
    Serial.printf("fx 1 %d\r\n", fx1);
    editables[359]->set(fx1);
    memcpy(fx1_d, &patch[v], 22);
    //for (int a = 0; a < 22; a++) Serial.printf("%02x ", fx1_d[a]);
    //Serial.printf("\r\n");
    v = v + 22;
    uint8_t fx2 = patch[v++];
    Serial.printf("fx 2 %d\r\n", fx2);
    editables[360]->set(fx2);
    memcpy(fx2_d, &patch[v], 22);
    v = v + 22;
    uint8_t fxm = patch[v++];
    Serial.printf("fx master %d\r\n", fxm);
    editables[361]->set(fxm);
    memcpy(fxm_d, &patch[v], 18);
    v = v + 18;
    editables[362]->set(patch[v++]);
    editables[363]->set(patch[v++]);
    editables[364]->set(patch[v++]);
    editables[365]->set(patch[v++]);
    int e = 0;
    //Parse fx1
    switch (fx1) {
      case 0:  //od
        editables[800 | E1]->set(fx1_d[e++]);
        editables[801 | E1]->set(fx1_d[e++]);
        editables[802 | E1]->set(fx1_d[e++]);
        editables[803 | E1]->set(fx1_d[e++]);
        editables[804 | E1]->set(fx1_d[e++]);
        editables[805 | E1]->set(fx1_d[e++]);
        editables[806 | E1]->set(fx1_d[e++]);
        editables[807 | E1]->set(fx1_d[e++]);
        editables[808 | E1]->set(fx1_d[e++]);
        editables[809 | E1]->set(fx1_d[e++]);
        editables[810 | E1]->set(fx1_d[e++]);
        editables[811 | E1]->set(fx1_d[e++]);
        editables[812 | E1]->set(fx1_d[e++]);
        editables[813 | E1]->set(fx1_d[e++]);
        editables[814 | E1]->set(fx1_d[e++]);
        editables[815 | E1]->set(fx1_d[e++]);
        editables[816 | E1]->set(fx1_d[e++]);
        editables[817 | E1]->set(fx1_d[e++]);
        editables[818 | E1]->set(fx1_d[e++]);
        break;
      case 1:  //comp
        editables[819 | E1]->set(fx1_d[e++]);
        editables[820 | E1]->set(fx1_d[e++]);
        editables[821 | E1]->set(fx1_d[e++]);
        editables[822 | E1]->set(fx1_d[e++]);
        editables[823 | E1]->set(fx1_d[e++]);
        editables[824 | E1]->set(fx1_d[e++]);
        editables[825 | E1]->set(fx1_d[e++]);
        editables[826 | E1]->set(fx1_d[e++]);
        editables[827 | E1]->set(fx1_d[e++]);
        break;
      case 2:  //eq
        editables[828 | E1]->set(fx1_d[e++]);
        editables[829 | E1]->set(fx1_d[e++]);
        editables[830 | E1]->set(fx1_d[e++]);
        editables[831 | E1]->set(fx1_d[e++]);
        editables[832 | E1]->set(fx1_d[e++]);
        editables[833 | E1]->set(fx1_d[e++]);
        editables[834 | E1]->set(fx1_d[e++]);
        editables[835 | E1]->set(fx1_d[e++]);
        editables[836 | E1]->set(fx1_d[e++]);
        editables[837 | E1]->set(fx1_d[e++]);
        editables[838 | E1]->set(fx1_d[e++]);
        editables[839 | E1]->set(fx1_d[e++]);
        editables[840 | E1]->set(fx1_d[e++]);
        editables[841 | E1]->set(fx1_d[e++]);
        editables[842 | E1]->set(fx1_d[e++]);
        editables[843 | E1]->set(fx1_d[e++]);
        editables[844 | E1]->set(fx1_d[e++]);
        editables[845 | E1]->set(fx1_d[e++]);
        editables[846 | E1]->set(fx1_d[e++]);
        editables[847 | E1]->set(fx1_d[e++]);
        break;
      case 3:  //wah
        editables[848 | E1]->set(fx1_d[e++]);
        editables[849 | E1]->set(fx1_d[e++]);
        editables[850 | E1]->set(fx1_d[e++]);
        editables[851 | E1]->set(fx1_d[e++]);
        editables[852 | E1]->set(fx1_d[e++]);
        editables[853 | E1]->set(fx1_d[e++]);
        editables[854 | E1]->set(fx1_d[e++]);
        editables[855 | E1]->set(fx1_d[e++]);
        editables[856 | E1]->set(fx1_d[e++]);
        editables[857 | E1]->set(fx1_d[e++]);
        editables[858 | E1]->set(fx1_d[e++]);
        editables[859 | E1]->set(fx1_d[e++]);
        break;
      case 4:  //exc
        editables[860 | E1]->set(fx1_d[e++]);
        editables[861 | E1]->set(fx1_d[e++]);
        editables[862 | E1]->set(fx1_d[e++]);
        editables[863 | E1]->set(fx1_d[e++]);
        editables[864 | E1]->set(fx1_d[e++]);
        editables[865 | E1]->set(fx1_d[e++]);
        editables[866 | E1]->set(fx1_d[e++]);
        editables[867 | E1]->set(fx1_d[e++]);
        editables[868 | E1]->set(fx1_d[e++]);
        editables[869 | E1]->set(fx1_d[e++]);
        editables[870 | E1]->set(fx1_d[e++]);
        editables[871 | E1]->set(fx1_d[e++]);
        break;
      case 5:  //deci
        editables[871 | E1]->set(fx1_d[e++]);
        editables[872 | E1]->set(fx1_d[e++]);
        editables[873 | E1]->set(fx1_d[e++]);
        editables[874 | E1]->set(fx1_d[e++]);
        editables[875 | E1]->set(fx1_d[e++]);
        editables[876 | E1]->set(fx1_d[e++]);
        editables[877 | E1]->set(fx1_d[e++]);
        editables[878 | E1]->set(fx1_d[e++]);
        editables[879 | E1]->set(fx1_d[e++]);
        editables[880 | E1]->set(fx1_d[e++]);
        editables[881 | E1]->set(fx1_d[e++]);
        break;
      case 6:  //chorus
        editables[882 | E1]->set(fx1_d[e++]);
        editables[883 | E1]->set(fx1_d[e++]);
        editables[884 | E1]->set(fx1_d[e++]);
        editables[885 | E1]->set(fx1_d[e++]);
        editables[886 | E1]->set(fx1_d[e++]);
        editables[887 | E1]->set(fx1_d[e]>>7);
        editables[888 | E1]->set((fx1_d[e]&0x70)>>4);
        editables[889 | E1]->set(fx1_d[e++]&0x0f);
        editables[890 | E1]->set(fx1_d[e++]);
        editables[891 | E1]->set(fx1_d[e++]);
        editables[892 | E1]->set(fx1_d[e++]);
        editables[893 | E1]->set(fx1_d[e++]);
        editables[894 | E1]->set(fx1_d[e++]);
        editables[895 | E1]->set(fx1_d[e++]);
        editables[896 | E1]->set(fx1_d[e++]);
        editables[897 | E1]->set(fx1_d[e++]);
        editables[898 | E1]->set(fx1_d[e++]);
        editables[899 | E1]->set(fx1_d[e++]);
        break;
      case 7:  //fla
        editables[900 | E1]->set(fx1_d[e++]);
        editables[901 | E1]->set(fx1_d[e++]);
        editables[902 | E1]->set(fx1_d[e++]);
        editables[903 | E1]->set(fx1_d[e++]);
        editables[904 | E1]->set(fx1_d[e++]);
        editables[905 | E1]->set(fx1_d[e++]);
        editables[906 | E1]->set(fx1_d[e]>>7);
        editables[907 | E1]->set((fx1_d[e]&0x70)>>4);
        editables[908 | E1]->set(fx1_d[e++]&0x0f);
        editables[909 | E1]->set(fx1_d[e++]);
        editables[910 | E1]->set(fx1_d[e++]);
        editables[911 | E1]->set(fx1_d[e++]);
        editables[912 | E1]->set(fx1_d[e++]);
        editables[913 | E1]->set(fx1_d[e++]);
        editables[914 | E1]->set(fx1_d[e++]);
        editables[915 | E1]->set(fx1_d[e++]);
        break;
      case 8:  //pha
        editables[916 | E1]->set(fx1_d[e++]);
        editables[917 | E1]->set(fx1_d[e++]);
        editables[918 | E1]->set(fx1_d[e++]);
        editables[919 | E1]->set(fx1_d[e++]);
        editables[920 | E1]->set(fx1_d[e++]);
        editables[921 | E2]->set(fx1_d[e]>>7);
        editables[922 | E2]->set((fx1_d[e]&0x70)>>4);
        editables[923 | E2]->set(fx1_d[e++]&0x0f);
        editables[924 | E1]->set(fx1_d[e++]);
        editables[925 | E1]->set(fx1_d[e++]);
        editables[926 | E1]->set(fx1_d[e++]);
        editables[927 | E1]->set(fx1_d[e++]);
        editables[928 | E1]->set(fx1_d[e++]);
        editables[929 | E1]->set(fx1_d[e++]);
        editables[930 | E1]->set(fx1_d[e++]);
        editables[931 | E1]->set(fx1_d[e++]);
        break;
      case 9:  //rot
        editables[932 | E1]->set(fx1_d[e++]);
        editables[933 | E1]->set(fx1_d[e++]);
        editables[934 | E1]->set(fx1_d[e++]);
        editables[935 | E1]->set(fx1_d[e++]);
        editables[936 | E1]->set(fx1_d[e++]);
        editables[937 | E1]->set(fx1_d[e++]);
        editables[938 | E1]->set(fx1_d[e++]);
        editables[939 | E1]->set(fx1_d[e++]);
        editables[940 | E1]->set(fx1_d[e++]);
        break;
      case 10:  //del
        editables[941 | E1]->set(fx1_d[e++]);
        editables[942 | E1]->set(fx1_d[e++]);
        editables[943 | E1]->set(fx1_d[e++]);
        editables[944 | E1]->set(fx1_d[e++]);
        editables[945 | E1]->set(fx1_d[e++]);
        editables[946 | E1]->set(fx1_d[e++]);
        editables[947 | E1]->set(fx1_d[e++]);
        editables[948 | E1]->set(fx1_d[e++]);
        editables[949 | E1]->set(fx1_d[e++]);
        editables[950 | E1]->set(fx1_d[e++]);
        editables[951 | E1]->set(fx1_d[e++]);
        break;
      case 11:  //talk
        editables[952 | E1]->set(fx1_d[e++]);
        editables[953 | E1]->set(fx1_d[e++]);
        editables[954 | E1]->set(fx1_d[e++]);
        editables[955 | E1]->set(fx1_d[e++]);
        editables[956 | E1]->set(fx1_d[e++]);
        editables[957 | E1]->set(fx1_d[e++]);
        editables[958 | E1]->set(fx1_d[e++]);
        editables[959 | E1]->set(fx1_d[e++]);
        editables[960 | E1]->set(fx1_d[e++]);
        editables[961 | E1]->set(fx1_d[e++]);
        editables[962 | E1]->set(fx1_d[e++]);
        break;
      case 12:  //multit
        editables[963 | E1]->set(fx1_d[e++]);
        editables[964 | E1]->set(fx1_d[e++]);
        editables[965 | E1]->set(fx1_d[e++]);
        editables[966 | E1]->set(fx1_d[e++]);
        editables[967 | E1]->set(fx1_d[e++]);
        editables[968 | E1]->set(fx1_d[e++]);
        editables[969 | E1]->set(fx1_d[e++]);
        editables[970 | E1]->set(fx1_d[e++]);
        editables[971 | E1]->set(fx1_d[e++]);
        editables[972 | E1]->set(fx1_d[e++]);
        editables[973 | E1]->set(fx1_d[e++]);
        editables[974 | E1]->set(fx1_d[e++]);
        editables[975 | E1]->set(fx1_d[e++]);
        editables[976 | E1]->set(fx1_d[e++]);
        editables[977 | E1]->set(fx1_d[e++]);
        editables[978 | E1]->set(fx1_d[e++]);
        editables[979 | E1]->set(fx1_d[e++]);
        break;
      case 13:  //ens
        editables[980 | E1]->set(fx1_d[e++]);
        editables[981 | E1]->set(fx1_d[e++]);
        editables[982 | E1]->set(fx1_d[e++]);
        editables[983 | E1]->set(fx1_d[e++]);
        editables[984 | E1]->set(fx1_d[e++]);
        editables[985 | E1]->set(fx1_d[e++]);
        editables[986 | E1]->set(fx1_d[e++]);
        editables[987 | E1]->set(fx1_d[e++]);
        editables[988 | E1]->set(fx1_d[e++]);
        editables[989 | E1]->set(fx1_d[e++]);
        break;
      case 14:  //rot
        editables[990 | E1]->set(fx1_d[e++]);
        editables[991 | E1]->set(fx1_d[e++]);
        editables[992 | E1]->set(fx1_d[e++]);
        editables[993 | E1]->set(fx1_d[e++]);
        editables[994 | E1]->set(fx1_d[e++]);
        editables[995 | E1]->set(fx1_d[e++]);
        editables[996 | E1]->set(fx1_d[e++]);
        editables[997 | E1]->set(fx1_d[e++]);
        editables[998 | E1]->set(fx1_d[e++]);
        editables[999 | E1]->set(fx1_d[e++]);
        editables[1000 | E1]->set(fx1_d[e++]);
        editables[1001 | E1]->set(fx1_d[e++]);
        editables[1002 | E1]->set(fx1_d[e++]);
        editables[1003 | E1]->set(fx1_d[e++]);
        break;
    }

    e = 0;
    //if (fx1 <= 10) {
    //Parse fx2
    switch (fx2) {
      case 0:  //od
        editables[800 | E2]->set(fx2_d[e++]);
        editables[801 | E2]->set(fx2_d[e++]);
        editables[802 | E2]->set(fx2_d[e++]);
        editables[803 | E2]->set(fx2_d[e++]);
        editables[804 | E2]->set(fx2_d[e++]);
        editables[805 | E2]->set(fx2_d[e++]);
        editables[806 | E2]->set(fx2_d[e++]);
        editables[807 | E2]->set(fx2_d[e++]);
        editables[808 | E2]->set(fx2_d[e++]);
        editables[809 | E2]->set(fx2_d[e++]);
        editables[810 | E2]->set(fx2_d[e++]);
        editables[811 | E2]->set(fx2_d[e++]);
        editables[812 | E2]->set(fx2_d[e++]);
        editables[813 | E2]->set(fx2_d[e++]);
        editables[814 | E2]->set(fx2_d[e++]);
        editables[815 | E2]->set(fx2_d[e++]);
        editables[816 | E2]->set(fx2_d[e++]);
        editables[817 | E2]->set(fx2_d[e++]);
        editables[818 | E2]->set(fx2_d[e++]);
        break;
      case 1:  //comp
        editables[819 | E2]->set(fx2_d[e++]);
        editables[820 | E2]->set(fx2_d[e++]);
        editables[821 | E2]->set(fx2_d[e++]);
        editables[822 | E2]->set(fx2_d[e++]);
        editables[823 | E2]->set(fx2_d[e++]);
        editables[824 | E2]->set(fx2_d[e++]);
        editables[825 | E2]->set(fx2_d[e++]);
        editables[826 | E2]->set(fx2_d[e++]);
        editables[827 | E2]->set(fx2_d[e++]);
        break;
      case 2:  //eq
        editables[828 | E2]->set(fx2_d[e++]);
        editables[829 | E2]->set(fx2_d[e++]);
        editables[830 | E2]->set(fx2_d[e++]);
        editables[831 | E2]->set(fx2_d[e++]);
        editables[832 | E2]->set(fx2_d[e++]);
        editables[833 | E2]->set(fx2_d[e++]);
        editables[834 | E2]->set(fx2_d[e++]);
        editables[835 | E2]->set(fx2_d[e++]);
        editables[836 | E2]->set(fx2_d[e++]);
        editables[837 | E2]->set(fx2_d[e++]);
        editables[838 | E2]->set(fx2_d[e++]);
        editables[839 | E2]->set(fx2_d[e++]);
        editables[840 | E2]->set(fx2_d[e++]);
        editables[841 | E2]->set(fx2_d[e++]);
        editables[842 | E2]->set(fx2_d[e++]);
        editables[843 | E2]->set(fx2_d[e++]);
        editables[844 | E2]->set(fx2_d[e++]);
        editables[845 | E2]->set(fx2_d[e++]);
        editables[846 | E2]->set(fx2_d[e++]);
        editables[847 | E2]->set(fx2_d[e++]);
        break;
      case 3:  //wah
        editables[848 | E2]->set(fx2_d[e++]);
        editables[849 | E2]->set(fx2_d[e++]);
        editables[850 | E2]->set(fx2_d[e++]);
        editables[851 | E2]->set(fx2_d[e++]);
        editables[852 | E2]->set(fx2_d[e++]);
        editables[853 | E2]->set(fx2_d[e++]);
        editables[854 | E2]->set(fx2_d[e++]);
        editables[855 | E2]->set(fx2_d[e++]);
        editables[856 | E2]->set(fx2_d[e++]);
        editables[857 | E2]->set(fx2_d[e++]);
        editables[858 | E2]->set(fx2_d[e++]);
        editables[859 | E2]->set(fx2_d[e++]);
        break;
      case 4:  //exc
        editables[860 | E2]->set(fx2_d[e++]);
        editables[861 | E2]->set(fx2_d[e++]);
        editables[862 | E2]->set(fx2_d[e++]);
        editables[863 | E2]->set(fx2_d[e++]);
        editables[864 | E2]->set(fx2_d[e++]);
        editables[865 | E2]->set(fx2_d[e++]);
        editables[866 | E2]->set(fx2_d[e++]);
        editables[867 | E2]->set(fx2_d[e++]);
        editables[868 | E2]->set(fx2_d[e++]);
        editables[869 | E2]->set(fx2_d[e++]);
        editables[870 | E2]->set(fx2_d[e++]);
        editables[871 | E2]->set(fx2_d[e++]);
        break;
      case 5:  //deci
        editables[871 | E2]->set(fx2_d[e++]);
        editables[872 | E2]->set(fx2_d[e++]);
        editables[873 | E2]->set(fx2_d[e++]);
        editables[874 | E2]->set(fx2_d[e++]);
        editables[875 | E2]->set(fx2_d[e++]);
        editables[876 | E2]->set(fx2_d[e++]);
        editables[877 | E2]->set(fx2_d[e++]);
        editables[878 | E2]->set(fx2_d[e++]);
        editables[879 | E2]->set(fx2_d[e++]);
        editables[880 | E2]->set(fx2_d[e++]);
        editables[881 | E2]->set(fx2_d[e++]);
        break;
      case 6:  //chorus
        editables[882 | E2]->set(fx2_d[e++]);
        editables[883 | E2]->set(fx2_d[e++]);
        editables[884 | E2]->set(fx2_d[e++]);
        editables[885 | E2]->set(fx2_d[e++]);
        editables[886 | E2]->set(fx2_d[e++]);
        editables[887 | E2]->set(fx2_d[e]>>7);
        editables[888 | E2]->set((fx2_d[e]&0x70)>>4);
        editables[889 | E2]->set(fx2_d[e++]&0x0f);
        editables[890 | E2]->set(fx2_d[e++]);
        editables[891 | E2]->set(fx2_d[e++]);
        editables[892 | E2]->set(fx2_d[e++]);
        editables[893 | E2]->set(fx2_d[e++]);
        editables[894 | E2]->set(fx2_d[e++]);
        editables[895 | E2]->set(fx2_d[e++]);
        editables[896 | E2]->set(fx2_d[e++]);
        editables[897 | E2]->set(fx2_d[e++]);
        editables[898 | E2]->set(fx2_d[e++]);
        editables[899 | E2]->set(fx2_d[e++]);
        break;
      case 7:  //fla
        editables[900 | E2]->set(fx2_d[e++]);
        editables[901 | E2]->set(fx2_d[e++]);
        editables[902 | E2]->set(fx2_d[e++]);
        editables[903 | E2]->set(fx2_d[e++]);
        editables[904 | E2]->set(fx2_d[e++]);
        editables[905 | E2]->set(fx2_d[e++]);
        editables[906 | E2]->set(fx2_d[e]>>7);
        editables[907 | E2]->set((fx2_d[e]&0x70)>>4);
        editables[908 | E2]->set(fx2_d[e++]&0x0f);
        editables[909 | E2]->set(fx2_d[e++]);
        editables[910 | E2]->set(fx2_d[e++]);
        editables[911 | E2]->set(fx2_d[e++]);
        editables[912 | E2]->set(fx2_d[e++]);
        editables[913 | E2]->set(fx2_d[e++]);
        editables[914 | E2]->set(fx2_d[e++]);
        editables[915 | E2]->set(fx2_d[e++]);
        break;
      case 8:  //pha
        editables[916 | E2]->set(fx2_d[e++]);
        editables[917 | E2]->set(fx2_d[e++]);
        editables[918 | E2]->set(fx2_d[e++]);
        editables[919 | E2]->set(fx2_d[e++]);
        editables[920 | E2]->set(fx2_d[e++]);
        editables[921 | E2]->set(fx2_d[e]>>7);
        editables[922 | E2]->set((fx2_d[e]&0x70)>>4);
        editables[923 | E2]->set(fx2_d[e++]&0x0f);
        editables[924 | E2]->set(fx2_d[e++]);
        editables[925 | E2]->set(fx2_d[e++]);
        editables[926 | E2]->set(fx2_d[e++]);
        editables[927 | E2]->set(fx2_d[e++]);
        editables[928 | E2]->set(fx2_d[e++]);
        editables[929 | E2]->set(fx2_d[e++]);
        editables[930 | E2]->set(fx2_d[e++]);
        editables[931 | E2]->set(fx2_d[e++]);
        break;
      case 9:  //rot
        editables[932 | E2]->set(fx2_d[e++]);
        editables[933 | E2]->set(fx2_d[e++]);
        editables[934 | E2]->set(fx2_d[e++]);
        editables[935 | E2]->set(fx2_d[e++]);
        editables[936 | E2]->set(fx2_d[e++]);
        editables[937 | E2]->set(fx2_d[e++]);
        editables[938 | E2]->set(fx2_d[e++]);
        editables[939 | E2]->set(fx2_d[e++]);
        editables[940 | E2]->set(fx2_d[e++]);
        break;
      case 10:  //del
        editables[941 | E2]->set(fx2_d[e++]);
        editables[942 | E2]->set(fx2_d[e++]);
        editables[943 | E2]->set(fx2_d[e++]);
        editables[944 | E2]->set(fx2_d[e++]);
        editables[945 | E2]->set(fx2_d[e++]);
        editables[946 | E2]->set(fx2_d[e++]);
        editables[947 | E2]->set(fx2_d[e++]);
        editables[948 | E2]->set(fx2_d[e++]);
        editables[949 | E2]->set(fx2_d[e++]);
        editables[950 | E2]->set(fx2_d[e++]);
        editables[951 | E2]->set(fx2_d[e++]);
        break;
    }
    //}
    e = 0;
    //parse master fx
    switch (fxm) {
      case 0:  //delay
        editables[1004 | EGL]->set(fxm_d[e++]);
        editables[1005 | EGL]->set(fxm_d[e++]);
        editables[1006 | EGL]->set(fxm_d[e++]);
        editables[1007 | EGL]->set(fxm_d[e++]);
        editables[1008 | EGL]->set(fxm_d[e++]);
        editables[1009 | EGL]->set(fxm_d[e++]);
        editables[1010 | EGL]->set(fxm_d[e++]);
        editables[1011 | EGL]->set(fxm_d[e++]);
        editables[1012 | EGL]->set(fxm_d[e++]);
        editables[1013 | EGL]->set(fxm_d[e++]);
        editables[1014 | EGL]->set(fxm_d[e++]);
        editables[1015 | EGL]->set(fxm_d[e++]);
        editables[1016 | EGL]->set(fxm_d[e++]);
        editables[1017 | EGL]->set(fxm_d[e++]);
        break;
      case 1:  //hall
        editables[1018 | EGL]->set(fxm_d[e++]);
        editables[1019 | EGL]->set(fxm_d[e++]);
        editables[1020 | EGL]->set(fxm_d[e++]);
        editables[1021 | EGL]->set(fxm_d[e++]);
        editables[1022 | EGL]->set(fxm_d[e++]);
        editables[1023 | EGL]->set(fxm_d[e++]);
        editables[1024 | EGL]->set(fxm_d[e++]);
        editables[1025 | EGL]->set(fxm_d[e++]);
        editables[1026 | EGL]->set(fxm_d[e++]);
        editables[1027 | EGL]->set(fxm_d[e++]);
        break;
      case 2:  //room
        editables[1028 | EGL]->set(fxm_d[e++]);
        editables[1029 | EGL]->set(fxm_d[e++]);
        editables[1030 | EGL]->set(fxm_d[e++]);
        editables[1031 | EGL]->set(fxm_d[e++]);
        editables[1032 | EGL]->set(fxm_d[e++]);
        editables[1033 | EGL]->set(fxm_d[e++]);
        editables[1034 | EGL]->set(fxm_d[e++]);
        editables[1035 | EGL]->set(fxm_d[e++]);
        editables[1036 | EGL]->set(fxm_d[e++]);
        editables[1037 | EGL]->set(fxm_d[e++]);
        editables[1038 | EGL]->set(fxm_d[e++]);
        editables[1039 | EGL]->set(fxm_d[e++]);
        break;
    }
#endif


    zp.print();
  }
  EditableValue::updateState(true);
  hideAll();
  showOSC1();
  setMod(false);

  switch (osc1_type) {
    case 0:
      showOSC1Std();
      break;
    case 1:
      showOSC1Comb();
      break;
    case 2:
      showOSC1VPM();
      break;
    case 3:
      showOSC1Reso();
      break;
    case 4:
      showOSC1Ring();
      break;
    case 5:
      showOSC1Cross();
      break;
    case 6:
      showOSC1Sync();
      break;
    case 7:
      showOSC1Organ();
      break;
    case 8:
      showOSC1EP();
      break;
    case 9:
      showOSC1Brass();
      break;
    case 10:
      showOSC1Reed();
      break;
    case 11:
      showOSC1Pluck();
      break;
    case 12:
      showOSC1Bowed();
      break;
  }
  
 

  //go to osc1 after patch dump
  //if (currPageFunc) (this->*Z1Edit::currPageFunc)();
}


void Z1Edit::process() {
}

void z1Patch::print() {
  Serial.printf("name:\r\n");
  Serial.printf("%s\r\n", name);
}
