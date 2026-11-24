#include "Editable.h"

#include "SynthBuddy.h"
#include "debug.h"

EditableSet editables;
EditableValue::ChangeFunc EditableValue::defaultFunc;
bool EditableValue::ustate = true;

// ---------------------------------------------------------------- the set

void EditableSet::init() {
  if (values) return;
  values = new EditableValue[Z1_PARAM_COUNT];   // on the heap: the large memory, not the fast one
  for (size_t i = 0; i < Z1_PARAM_COUNT; ++i) values[i].bind(&Z1_PARAMS[i]);
}

EditableValue* EditableSet::operator[](int key) const {
  int i = z1ParamIndex(key);
  return (i >= 0 && values) ? &values[i] : nullptr;
}

void hideAll() {
  for (auto& value : editables) value.hide();
}

// ---------------------------------------------------------------- one value

void EditableValue::bind(const ParamDef* def) {
  mDef = def;
  mKey = def->key;
  mType = def->type;
  setMinMax(def->min, def->max, def->def);
}

void EditableValue::setMinMax(int min, int max, int def) {
  mIVal = def;
  if (min < max) {
    inverted = false;
    mIMin = min;
    mIMax = max;
  } else {
    inverted = true;
    mIMax = min;
    mIMin = max;
  }
}

const char* EditableValue::listText() const {
  const ParamList* l = mDef ? mDef->list : nullptr;
  if (!l || mIVal < 0 || mIVal >= (int)l->count) return "";
  return l->items[mIVal];
}

void EditableValue::button() {
  if (btnFunc) btnFunc(mPos);
}

// The value was changed with the encoder: tell whoever listens, then redraw.
void EditableValue::changed() {
  const char* text = (mType == editableType::TextEnum) ? listText() : "null";
  if (changeFunc) {
    changeFunc(mKey, mType, mIVal, mFVal, text);
  } else {
    if (defaultFunc) defaultFunc(mKey, mType, mIVal, mFVal, text);
  }
  update();
}

void EditableValue::inc() {
  if (!enabled) return;
  switch (mType) {
    case editableType::Integer:
      if (mIVal < mIMax) mIVal++;
      break;
    case editableType::Float:
      mFVal = mFVal + mFincr;
      break;
    case editableType::TextEnum:
      mIVal++;
      if (mIVal > mIMax) mIVal = mIMax;
      break;
    case editableType::None:
      break;
  }
  changed();
}

void EditableValue::dec() {
  if (!enabled) return;
  switch (mType) {
    case editableType::Integer:
      if (mIVal > mIMin) mIVal--;
      break;
    case editableType::Float:
      mFVal = mFVal - mFincr;
      break;
    case editableType::TextEnum:
      mIVal--;
      if (mIVal < 0) mIVal = 0;
      break;
    case editableType::None:
      break;
  }
  changed();
}

void EditableValue::show(int pos) {
  auto s = SynthBuddy::getInstance();
  mPos = pos;
  visible = true;
  if (mType != editableType::None)
    s->label(pos, mDef ? mDef->name : "");
  else s->label(pos, "      ");
  s->currentEditable[pos] = this;

  update();
}

int EditableValue::get() {
  return mIVal;
}

bool EditableValue::isSigned() {
  if (mIMin < 0) return true;
  else return false;
}

// A byte from a program dump: signed parameters are stored as two's complement.
void EditableValue::set(uint8_t v) {
  if (isSigned()) {
    int8_t tmp = (int8_t)v;
    mIVal = (int)tmp;
  } else mIVal = (int)v;
  if (ustate) update();
}

void EditableValue::set(int v) {
  mIVal = v;
  if (ustate) update();
}

void EditableValue::update() {
  if (!visible) return;
  auto s = SynthBuddy::getInstance();
  if (updateFunc) {
    char text[24];
    updateFunc(mIVal, mprec, fpar1, fpar2, fpar3, fpar4, text, sizeof text);
    s->value(mPos, text);
    return;
  }
  switch (mType) {
    case editableType::Integer:
      s->value(mPos, mIVal);
      break;
    case editableType::Float:
      s->value(mPos, mFVal);
      break;
    case editableType::TextEnum:
      s->value(mPos, listText());
      break;
    case editableType::None:
      s->value(mPos, "      ");
      break;
  }
}
