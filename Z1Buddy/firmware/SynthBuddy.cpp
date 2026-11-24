#include "SynthBuddy.h"
#include <cstdio>
#include <cstring>
#include <Arduino.h>
#include "Editable.h"
#include <i2c_driver.h>
#include <i2c_driver_wire.h>
//#include <Wire.h>
#define R_START 0x0
#define DIR_NONE 0x0
// Clockwise step.
#define DIR_CW 0x10
// Counter-clockwise step.
#define DIR_CCW 0x20
#define HALF_STEP
#ifdef HALF_STEP
// Use the half-step state table (emits a code at 00 and 11)
#define R_CCW_BEGIN 0x1
#define R_CW_BEGIN 0x2
#define R_START_M 0x3
#define R_CW_BEGIN_M 0x4
#define R_CCW_BEGIN_M 0x5
const unsigned char ttable[6][4] = {
  // R_START (00)
  {R_START_M,            R_CW_BEGIN,     R_CCW_BEGIN,  R_START},
  // R_CCW_BEGIN
  {R_START_M | DIR_CCW, R_START,        R_CCW_BEGIN,  R_START},
  // R_CW_BEGIN
  {R_START_M | DIR_CW,  R_CW_BEGIN,     R_START,      R_START},
  // R_START_M (11)
  {R_START_M,            R_CCW_BEGIN_M,  R_CW_BEGIN_M, R_START},
  // R_CW_BEGIN_M
  {R_START_M,            R_START_M,      R_CW_BEGIN_M, R_START},// | DIR_CW},
  // R_CCW_BEGIN_M
  {R_START_M,            R_CCW_BEGIN_M,  R_START_M,    R_START},// | DIR_CCW},
};
#else
// Use the full-step state table (emits a code at 00 only)
#define R_CW_FINAL 0x1
#define R_CW_BEGIN 0x2
#define R_CW_NEXT 0x3
#define R_CCW_BEGIN 0x4
#define R_CCW_FINAL 0x5
#define R_CCW_NEXT 0x6

const unsigned char ttable[7][4] = {
  // R_START
  {R_START,    R_CW_BEGIN,  R_CCW_BEGIN, R_START},
  // R_CW_FINAL
  {R_CW_NEXT,  R_START,     R_CW_FINAL,  R_START | DIR_CW},
  // R_CW_BEGIN
  {R_CW_NEXT,  R_CW_BEGIN,  R_START,     R_START},
  // R_CW_NEXT
  {R_CW_NEXT,  R_CW_BEGIN,  R_CW_FINAL,  R_START},
  // R_CCW_BEGIN
  {R_CCW_NEXT, R_START,     R_CCW_BEGIN, R_START},
  // R_CCW_FINAL
  {R_CCW_NEXT, R_CCW_FINAL, R_START,     R_START | DIR_CCW},
  // R_CCW_NEXT
  {R_CCW_NEXT, R_CCW_FINAL, R_CCW_BEGIN, R_START},
};
#endif

// The six fields of one display: where each starts and how wide it is.
static constexpr int displayWidth[6] = { 7, 7, 7, 6, 7, 6 };
static constexpr int displayStart[6] = { 0, 7, 14, 21, 27, 34 };

// Centres s in a field w characters wide: half the spare room in front, the
// rest behind. A text longer than the field is written in full, as before.
static void center(const char* s, int w, char* out, size_t outLen) {
  if (outLen == 0) return;
  int len = (int)strlen(s);
  int pad = w - len;
  int left = pad > 0 ? pad / 2 : 0;
  int right = pad > 0 ? pad - left : 0;
  size_t n = 0;
  for (int i = 0; i < left && n + 1 < outLen; ++i) out[n++] = ' ';
  for (int i = 0; i < len && n + 1 < outLen; ++i) out[n++] = s[i];
  for (int i = 0; i < right && n + 1 < outLen; ++i) out[n++] = ' ';
  out[n] = 0;
}

SynthBuddy *SynthBuddy::instance = nullptr;
uint8_t SynthBuddy::state[30];

SynthBuddy::SynthBuddy() {
        ledstate=0;
        // Initialise state.


};

SynthBuddy *SynthBuddy::getInstance() {
    if(instance==nullptr) instance=new SynthBuddy();
    return instance;
}


void SynthBuddy::addDisplay(LiquidCrystal* d) {
    lcds.push_back(d);
}

void SynthBuddy::label(int pos, const char* v) {
    int p=pos/6;
    auto l=lcds[p];
    int mPos=pos-6*p;
    char text[48];
    center(v, displayWidth[mPos], text, sizeof text);
    l->setCursor(displayStart[mPos],0);
    l->print(text);
}

void SynthBuddy::value(int pos, int v) {
    char text[16];
    snprintf(text, sizeof text, "%d", v);
    value(pos, text);
}

void SynthBuddy::value(int pos, float v) {
    char text[24];
    snprintf(text, sizeof text, "%g", (double)v);
    value(pos, text);
}

void SynthBuddy::value(int pos, const char* v) {
    int p=pos/6;
    auto l=lcds[p];
    int mPos=pos-6*p;
    char text[48];
    center(v, displayWidth[mPos], text, sizeof text);
    l->setCursor(displayStart[mPos],1);
    l->print(text);
}

void SynthBuddy::clear(int pos) {
  currentEditable[pos]=nullptr;
  buttonFunc[pos]=nullptr;
  label(pos,"");
  value(pos,"");
}
void SynthBuddy::clear() {
  for (auto& e : currentEditable) e=nullptr;
  for (auto& l : lcds) l->clear();
  for (auto& f : buttonFunc) f=nullptr;
}

void SynthBuddy::setLed(int led, bool v) {
    v?ledstate|=(1<<led):ledstate&=~(1<<led);
    Wire.beginTransmission(MCP_LB1);
    Wire.write(MCP23017_GPIOB); // GPIOB
    Wire.write((ledstate>>8)&0xff); // port B
    Wire.endTransmission();
    Wire.beginTransmission(MCP_LB1);
    Wire.write(MCP23017_GPIOA); // GPIOB
    Wire.write(ledstate&0xff); // port A
    Wire.endTransmission();
 }

void SynthBuddy::ledsOff() {
    ledstate=0;
    Wire.beginTransmission(MCP_LB1);
    Wire.write(MCP23017_GPIOB); // GPIOB
    Wire.write(0); // port B
    Wire.endTransmission();
    Wire.beginTransmission(MCP_LB1);
    Wire.write(MCP23017_GPIOA); // GPIOB
    Wire.write(0); // port A
    Wire.endTransmission();
 }

 void SynthBuddy::configEncBoard(uint8_t addr) {
  Wire.beginTransmission(addr);
  Wire.write(MCP23017_GPPUA); // GPPUA register
  Wire.write(0xff); // pullup
  Wire.endTransmission();
  Wire.beginTransmission(addr);
  Wire.write(MCP23017_IPOLA); // IOPOLA register
  Wire.write(0xff); // pullup
  Wire.endTransmission();
  Wire.beginTransmission(addr);
  Wire.write(MCP23017_IODIRA); // IODIRA register
  Wire.write(0xff); // set all of port A to inputs
  Wire.endTransmission();

  Wire.beginTransmission(addr);
  Wire.write(MCP23017_INTCONA); // INTCONA register
  Wire.write(0x00); //
  Wire.endTransmission();
  Wire.beginTransmission(addr);
  Wire.write(MCP23017_IOCONA);
  Wire.write(0x60); //
  Wire.endTransmission();
  Wire.beginTransmission(addr);
  Wire.write(MCP23017_GPINTENA); // INTCONA register
  Wire.write(0xff); //
  Wire.endTransmission();

  Wire.beginTransmission(addr);
  Wire.write(MCP23017_GPPUB); // GPPUA register
  Wire.write(0xff); // pullup
  Wire.endTransmission();
  Wire.beginTransmission(addr);
  Wire.write(MCP23017_IPOLB); // IOPOLA register
  Wire.write(0xff); // pullup
  Wire.endTransmission();
  Wire.beginTransmission(addr);
  Wire.write(MCP23017_IODIRB); // IODIRA register
  Wire.write(0xff); // set all of port A to inputs
  Wire.endTransmission();

  Wire.beginTransmission(addr);
  Wire.write(MCP23017_INTCONB); // INTCONA register
  Wire.write(0x00); //
  Wire.endTransmission();
  Wire.beginTransmission(addr);
  Wire.write(MCP23017_IOCONB);
  Wire.write(0x60); //
  Wire.endTransmission();
  Wire.beginTransmission(addr);
  Wire.write(MCP23017_GPINTENB); // INTCONA register
  Wire.write(0xff); //
  Wire.endTransmission();
  auto t = readEncBoard(addr);
  
}

uint16_t SynthBuddy::readEncBoard(uint8_t addr) {

  Wire.beginTransmission(addr);
  Wire.write(MCP23017_GPIOA);
  Wire.endTransmission();
  Wire.requestFrom(addr, 1); // request one byte of data from MCP20317
  uint16_t tmp = Wire.read();
  Wire.beginTransmission(addr);
  Wire.write(MCP23017_GPIOB);
  Wire.endTransmission();
  Wire.requestFrom(addr, 1); // request one byte of data from MCP20317
  tmp |= Wire.read()<<8;

  return tmp;
}

void SynthBuddy::button(int btn) {
  Serial.printf("button %d\n",btn);
  if(buttonFunc[btn])buttonFunc[btn]();
   //if(currentEditable[btn])currentEditable[btn]->button();
}

void SynthBuddy::encupdate(int enc, uint16_t pin1, uint16_t pin2) {
  
   uint8_t inverter=0;
   uint8_t p1=pin1?1:0;
   uint8_t p2=pin2?1:0;

   unsigned char pinstate = ((inverter ^ p2) << 1) | (inverter ^ p1?1:0);
  // Determine new state from the pins and state table.
  //Serial.print("pinstate ");
  //Serial.print(pinstate,HEX);
  //Serial.print(" ");
  //  Serial.print(p1);
  //Serial.print(" ");
  // Serial.print(p2);


  //Serial.printf("enc %d\r\n",enc);
  
   state[enc] = ttable[state[enc] & 0xf][pinstate];
   //if(pinstate==0)state[enc]=0;
  // Return emit bits, ie the generated event.
   //Serial.print(",state[enc] ");
   //Serial.println(state[enc],HEX);
   Serial.println(enc);
   Serial.println((uint)currentEditable[enc],HEX);
   switch(state[enc] & 0x30) {
    case DIR_CW:        
        if(currentEditable[enc]) currentEditable[enc]->isInverted()?currentEditable[enc]->inc():currentEditable[enc]->dec() ;
        //Serial.println(" inc");
        break;
    case DIR_CCW:
         if(currentEditable[enc]) currentEditable[enc]->isInverted()?currentEditable[enc]->dec():currentEditable[enc]->inc();
        //Serial.println(" dec");
        break;
    default:
        break;

   }
 
}
