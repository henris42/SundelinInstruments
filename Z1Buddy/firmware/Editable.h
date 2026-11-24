#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>

#include "z1_params.h"

// One parameter of the editor: its current value and how it is shown. What
// the parameter is (key, name, range, list of names) comes from the constant
// table in z1_params.h; only what changes while the Buddy runs lives here.
class EditableValue {
public:
  // Called when the value was changed with an encoder.
  using ChangeFunc = std::function<void(int key, editableType t, int iVal, float fVal, const char* sVal)>;
  // Writes the value as text for the display into out.
  using FormatFunc = void (*)(int iVal, int prec, float p1, float p2, float p3, float p4, char* out, size_t outLen);

  EditableValue() = default;
  void bind(const ParamDef* def);

  static void updateState(bool s) {
    ustate = s;
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
  void setMinMax(int min, int max, int def = 0);
  bool isInverted() {
    return inverted;
  };
  static void subscribeDefault(ChangeFunc f) {
    defaultFunc = f;
  };
  void setEnabled(bool e) {
    enabled = e;
  };

  void subscribe(ChangeFunc f) {
    changeFunc = f;
  };
  void setUpdateFunc(FormatFunc f, int p, float f1 = 0.0, float f2 = 1.0, float f3 = 1.0, float f4 = 0.0) {
    updateFunc = f;
    fpar1 = f1;
    fpar2 = f2;
    fpar3 = f3;
    fpar4 = f4;
    mprec = p;
  };

  int getIval() {
    return mIVal;
  };

  void hide() {
    visible = false;
  }

  const ParamDef* def() const {
    return mDef;
  }

private:
  const char* listText() const;   // the name for the current value of a list parameter
  void changed();

  const ParamDef* mDef = nullptr;
  int mKey = 0;
  int mPos = 0;
  static bool ustate;
  editableType mType = editableType::None;
  int mIVal = 0;
  int mIMax = 0;
  int mIMin = 0;
  float mFVal = 0.0f;
  float mFincr = 0.1f;
  bool inverted = false;
  bool enabled = true;
  bool visible = true;
  ChangeFunc changeFunc;
  static ChangeFunc defaultFunc;
  void (*btnFunc)(int btn) = nullptr;
  FormatFunc updateFunc = nullptr;
  int mprec = 0;
  float fpar1 = 0.0f, fpar2 = 1.0f, fpar3 = 1.0f, fpar4 = 0.0f;
};

// All parameters, found by key: editables[263]->set(60).
// A key that is not in the table gives nullptr; count() tells without that.
class EditableSet {
public:
  void init();                               // once, at start-up
  EditableValue* operator[](int key) const;
  size_t count(int key) const {
    return z1ParamIndex(key) >= 0 ? 1 : 0;
  }
  EditableValue* begin() const {
    return values;
  }
  EditableValue* end() const {
    return values + (values ? Z1_PARAM_COUNT : 0);
  }

private:
  EditableValue* values = nullptr;
};

void hideAll();

extern EditableSet editables;
