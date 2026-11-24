#pragma once
#include <Arduino.h>
#include <vector>
#include <functional>
#include <LiquidCrystal.h>
#include "Editable.h"

#define MCP23017_ADDRESS 0x20

#define MCP_LB1 0x20
#define MCP_LB2 0x23
#define MCP_LB3 0x21
#define MCP_LB4 0x25

#define MCP_ENC1 0x21
#define MCP_ENC2 0x24
#define MCP_IO1 0x26

// registers
#define MCP23017_IODIRA 0x00
#define MCP23017_IPOLA 0x02
#define MCP23017_GPINTENA 0x04
#define MCP23017_DEFVALA 0x06
#define MCP23017_INTCONA 0x08
#define MCP23017_IOCONA 0x0A
#define MCP23017_GPPUA 0x0C
#define MCP23017_INTFA 0x0E
#define MCP23017_INTCAPA 0x10
#define MCP23017_GPIOA 0x12
#define MCP23017_OLATA 0x14


#define MCP23017_IODIRB 0x01
#define MCP23017_IPOLB 0x03
#define MCP23017_GPINTENB 0x05
#define MCP23017_DEFVALB 0x07
#define MCP23017_INTCONB 0x09
#define MCP23017_IOCONB 0x0B
#define MCP23017_GPPUB 0x0D
#define MCP23017_INTFB 0x0F
#define MCP23017_INTCAPB 0x11
#define MCP23017_GPIOB 0x13
#define MCP23017_OLATB 0x15

#define MCP23017_INT_ERR 255
#define SCREENPARMS 18




class SynthBuddy {
public:

    static SynthBuddy * getInstance();

    void addDisplay(LiquidCrystal* d);

    void label(int pos, const char* v);
    void value(int pos, const char* v);
    void value(int pos, int v);
    void value(int pos, float v);
    void clear(int pos);
    void clear();

    void setLed(int led, bool v);
    void ledsOff();


    void configEncBoard(uint8_t addr);
    uint16_t readEncBoard(uint8_t addr);
    void encupdate(int enc, uint16_t pin1, uint16_t pin2);
    void button(int btn);
    void subscribeButton(int b, std::function<void()> f) {
      buttonFunc[b]=f;
    };
    


    EditableValue * currentEditable[SCREENPARMS];

private:
    SynthBuddy();
   

    static SynthBuddy * instance;
    static uint8_t state[30];
    //static int position[30];
    //unsigned char state;
    //unsigned char inverter;
    
    std::vector<LiquidCrystal*> lcds;
    int ledstate;
    std::function<void()>buttonFunc[SCREENPARMS];

};
