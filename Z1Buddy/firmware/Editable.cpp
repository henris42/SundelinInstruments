#include "Editable.h"

#include "SynthBuddy.h"

//#include <string>
#include <sstream>

DMAMEM std::map<int,EditableValue*> editables;
std::function<void(int key, editableType t, int iVal, float fVal, std::string sVal)> EditableValue::defaultFunc;

EditableValue::EditableValue(int key, std::string name, int min, int max, int def, editableType t, const std::vector<std::string> & svals) {
    mKey=key;
    mName=name;
    mAddr=key;
    mPos=0;
    mType=t;
    mIVal=def;
    mFVal=0.0;
    mFincr=0.1;
    mSVal="null";
    //editables[mKey]=this;
    mSVals=svals;
    enabled=true;
    ustate=true;

    inverted=false;
    visible=true;
    changeFunc=nullptr;
    btnFunc=nullptr;
    updateFunc=nullptr;
    setMinMax(min, max, def);
    
    
  }

void EditableValue::setMinMax(int min, int max, int def) {
  mIVal=def;
  if(min<max) {
    inverted=false;
    mIMin=min;
    mIMax=max;  
  } else {
    inverted=true;
    mIMax=min;
    mIMin=max;
  }
}

void EditableValue::button(){
  if(btnFunc)btnFunc(mPos);
}

void EditableValue::inc() {
  if(enabled){
    switch(mType) {
      case editableType::Integer:
        if(mIVal<mIMax)mIVal++;
        break;
      case editableType::Float:
        mFVal=mFVal+mFincr;
        
        break;
      case editableType::TextEnum:
        mIVal++;
        //if(mIVal>=mSVals.size())mIVal=mSVals.size()-1;
        if(mIVal>mIMax)mIVal=mIMax;
        mSVal=mSVals[mIVal];
        break;
      case editableType::None:
        break;
    }
    if(changeFunc) {
      changeFunc(mKey, mType, mIVal, mFVal, mSVal);
    } else {
      if(defaultFunc)defaultFunc(mKey, mType, mIVal, mFVal, mSVal);
    }
    update();
  }
}


void EditableValue::dec() {
  if(enabled){
    switch(mType) {
      case editableType::Integer:
        if(mIVal>mIMin)mIVal--;
        break;
      case editableType::Float:
        mFVal=mFVal-mFincr;
        
        break;
      case editableType::TextEnum:
        mIVal--;
        if(mIVal<0)mIVal=0;
        mSVal=mSVals[mIVal];
        break;
      case editableType::None:
        break;
    }
    if(changeFunc) {
      changeFunc(mKey, mType, mIVal, mFVal, mSVal);
    } else {
      if(defaultFunc)defaultFunc(mKey, mType, mIVal, mFVal, mSVal);
    }
    update();
  }
}

bool EditableValue::ustate;

void EditableValue::show(int pos) {
    auto s = SynthBuddy::getInstance();
    mPos=pos;
    visible=true;
    //stringstream ss;
    if(mType!=editableType::None)
        s->label(pos,mName);
    else s->label(pos,"      ");
    s->currentEditable[pos]=this;

    update();


}

int EditableValue::get() {
  return mIVal;
}

bool EditableValue::isSigned() {
  if(mIMin<0)return true;
  else return false;
}

void EditableValue::set(uint8_t v) {
  //Serial.printf("set %d to %d\n",mKey,mIVal);
  if(isSigned()) {
    int8_t tmp=(int8_t)v;
    mIVal=(int)tmp;
  }
    else mIVal=(int)v;
  if(ustate)update();
}

void EditableValue::set(int v) {
  //Serial.printf("set %d to %d\n",mKey,mIVal);
  mIVal=v;
  if(ustate)update();
}

void EditableValue::update() {
    auto s = SynthBuddy::getInstance();
    //Serial.printf("%d %f %f %f %f\n",mIVal,fpar1,fpar2,fpar3,fpar4);
    if(visible) {
    if(updateFunc) {
       s->value(mPos,updateFunc(mIVal,mprec,fpar1,fpar2,fpar3,fpar4));
      return; 
    } 
    stringstream ss;
    
    switch(mType) {
      case editableType::Integer:
        ss<<mIVal;
        s->value(mPos,ss.str());
        break;
      case editableType::Float:
        ss<<mFVal;
         s->value(mPos,ss.str());
        break;
      case editableType::TextEnum:
         s->value(mPos,mSVals[mIVal]);
        break;
      case editableType::None:
         s->value(mPos,"      ");
        break;
    }
    }

}

void hideAll() {
   for (const auto & [key, value] : editables) value->hide();
}
