#pragma once
#include <cstdint>  // uint8_t
#include <string>
#include <functional>
#include <map>
//#include <any>
#include <vector>
//#include <Arduino.h>

//using namespace std;


enum class editableType {
  Integer,
  Float,
  TextEnum,
  None
};


class EditableValue {
public:
  EditableValue(int key, std::string name, int min, int max, int def, editableType t, const std::vector<std::string> & svals=std::vector<std::string>());

  static void updateState(bool s) {
      ustate=s;
  };
  void show(int pos);
  void update();
  void set(int v);
  void set(uint8_t v);
  int get();
  void inc();
  void dec();
  void button();
  bool isSigned();
  void setMinMax(int min, int max, int def=0);
  bool isInverted(){return inverted;};
  static void subscribeDefault(std::function<void(int key, editableType t, int iVal, float fVal, std::string sVal)> f) {
    defaultFunc=f;
  };
  void setEnabled(bool e){
    enabled=e;
  };

  void subscribe(std::function<void(int key, editableType t, int iVal, float fVal, std::string sVal)> f) {
    changeFunc=f;
  };
  void setUpdateFunc(std::function<std::string(int iVal, int prec, float p1, float p2, float p3, float p4)> f, int p, float f1=0.0, float f2=1.0, float f3=1.0, float f4=0.0) {
    updateFunc=f;
    fpar1=f1;
    fpar2=f2;
    fpar3=f3;
    fpar4=f4;
    mprec=p;
   
  };
    
  int getIval() {
    return mIVal;
  };

  void hide() {
    visible=false;
  }
  
private:
   int mKey;
  std::string mName;
  int mAddr;
  int mPos;
  static bool ustate;
  editableType mType;
  int mIVal;
  int mIMax;
  int mIMin;
  float mFVal;
  float mFincr;
  std::string mSVal;
  std::vector<std::string>mSVals;
  bool inverted;
  bool enabled;
  bool visible;
  std::function<void(int key, editableType t, int iVal, float fVal, std::string sVal)> changeFunc;
  static std::function<void(int key, editableType t, int iVal, float fVal, std::string sVal)> defaultFunc;
  std::function<void(int btn)>btnFunc;
  //std::function<std::string(int iVal)>updateFunc;
  std::function<std::string(int iVal,int prec, float p1, float p2, float p3, float p4)>updateFunc;
  int mprec;
  float fpar1,fpar2,fpar3,fpar4;
  //
  


};

void hideAll();

extern std::map<int,EditableValue*> editables;
