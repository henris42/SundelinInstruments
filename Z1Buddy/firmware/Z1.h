#pragma once
#include "SynthBuddy.h"
#include "Editable.h"
#include <vector>
#include <string>

class z1Patch {
public:
    char name[17];

    uint8_t unison;
    uint8_t detune;
    uint8_t random;

    void print();


    z1Patch()  {
        memset(name,0,sizeof(name));
    }

};


enum class editState {
    OSC1=0,
    OSC2,
    SUBOSC,
    FILT,
    FX,
    EG,
    LFO,
    OSC1MOD,
    OSC2MOD,
    SUBOSCMOD,
    FILTMOD,
    FXMOD,
    EGMOD,
    LFOMOD,
    NONE

};

enum class mainState {
    NORMAL=0,
    MPEPRE,
    MPEPRE2,
    MPEPRE3,
    MPE,
    MPEPLUS  
};

enum class unisonState {
    OFF=0,
    U2,
    U3,
    U6  
};

class Z1Edit {

public:
    Z1Edit();
    static Z1Edit * getInstance();
    void init();
    void process();
    void setEditState(editState state);
    editState geteditState(){return estate;};
    mainState getMainState(){return mstate;};
    void setMainState(mainState s){mstate=s; showUnison();};
    bool inMPE(){return mstate==mainState::MPE || mstate==mainState::MPEPLUS;};
    int mpeVoices();
    void setMod(bool v);
    void toggleMod();
    void nextMainMode();
    void enterMPE();
    void leaveMPE();
    void gotoMainMode(int mode);   // 0 Normal, 1 MPE, 2 MPE+
    int unisonShown();             // 0 off, 1 = 2 voices, 2 = 3, 3 = 6, for the current mode
    void setUnisonLevel(int level);
    void nextUnisonMode();
    void setUnison(unisonState u);
    void unisonFromPatch(uint8_t b);
    void showUnison();

    void sendZ1ProgParm(uint8_t channel, uint8_t group, int param, int val);
    void parseZ1SysExParam(const uint8_t* sx, int len);
    void parseZ1SysExPreset(const uint8_t* sx, int len);
    void getCurrPatch();
    void getCurrArp();
    void storeForMPE();
    void setZ1Mode(bool multi);

    
    SynthBuddy* buddy() {
        return s;
    };

    void showOSC1();
    void showOSC1Std(); 
    void showOSC1Comb(); 
    void showOSC1VPM();
    void showOSC1Reso(); 
    void showOSC1Ring();
    void showOSC1Cross(); 
    void showOSC1Sync();
    void showOSC1Organ(); 
    void showOSC1EP();
    void showOSC1Brass(); 
    void showOSC1Reed();
    void showOSC1Pluck(); 
    void showOSC1Bowed();
    void showOSC2();

    void showSubOsc();


    void showEG();
    void showLFO();
    void showFilt();
    void showFx();
    //void showFx1();
    //void showFx2();
    void showFxMain();
    void showFxOverdrive();
    void showFxCompressor();
    void showFxPEQ();
    void showFxWah();
    void showFxExciter();
    void showFxDecimator();
    void showFxChorus();
    void showFxFlanger();
    void showFxPhaser();
    void showFxRotarySmall();
    void showFxDelayMono();
    void showFxTalking();
    void showFxMultitap();
    void showFxEnsemble();
    void showFxRotary();
    void showFxDelay();
    void showFxHall();
    void showFxRoom();

   // void setPage(std::function<void()> f) {
    //  currPageFunc=f;
    //};

private:

    static Z1Edit * instance;

    editState estate;
    mainState mstate;
    unisonState ustate;      // unison of the Z1 program (Normal mode)
    unisonState mpeUstate;   // timbres stacked per note in MPE mode
    bool mod;
    int page;
    int fxpage;
    int egpage;
    int fpage;
    int lfopage;
    int spage;
    
    //std::function<void()> currPageFunc;
    void (Z1Edit::*currPageFunc)();



    //static std::string z1EQFreqFunc(int i);
    SynthBuddy* s;

    


    

};
