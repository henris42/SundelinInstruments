// The display formatting after the refactor against the code it replaced
// (string streams), which is kept here as the reference.
#include <cstdio>
#include <cstring>
#include <cmath>
#include <string>
#include <sstream>
struct Dbg { void printf(const char*, ...) {} } Serial;
#include "display.inc"
// ---- the old implementations, word for word
static std::string oldCenter(const std::string s, const int w) { std::stringstream ss, spaces; int pad = w - s.size(); for (int i = 0; i < pad / 2; ++i) spaces << " "; ss << spaces.str() << s << spaces.str(); if (pad > 0 && pad % 2 != 0) ss << " "; return ss.str(); }
static std::string oldInt(int v) { std::stringstream ss; ss << v; return ss.str(); }
static std::string oldFloat(float v) { std::stringstream ss; ss << v; return ss.str(); }
static std::string oldScale(int i, int prec, float f1, float f2, float f3, float f4) { std::stringstream ss; ss.setf(std::ios::fixed); ss.precision(prec); float v = f1 + ((float)i - f4) * ((f2 - f1) / f3); ss << v; return ss.str(); }
static std::string oldExp(int i, int prec, float f1, float f2, float f3, float f4) { std::stringstream ss; ss.setf(std::ios::fixed); ss.precision(2); float v = f1 * exp((float)i * f2); if (v > 1000.0) { v = v / 1000.0; ss << v; ss << "k"; } else { ss << v; } return ss.str(); }
int main() {
  int bad = 0, n = 0; char buf[64];
  const char* words[] = { "", "A", "Off", "Cutoff", "PickupS", "EG1 Srt", "toolongname", "-99", "12.5k" };
  for (const char* w : words) for (int width : { 6, 7 }) { center(w, width, buf, sizeof buf); n++; if (oldCenter(w, width) != buf) { bad++; printf("center [%s] w%d: old [%s] new [%s]\n", w, width, oldCenter(w, width).c_str(), buf); } }
  for (int v = -120; v <= 251; v++) { snprintf(buf, sizeof buf, "%d", v); n++; if (oldInt(v) != buf) bad++; }
  for (float v : { 0.0f, 0.1f, -0.3f, 1.5f, 12.25f, 100.0f, 1e-5f, 123456.7f }) { snprintf(buf, sizeof buf, "%g", (double)v); n++; if (oldFloat(v) != buf) { bad++; printf("float %f: old [%s] new [%s]\n", v, oldFloat(v).c_str(), buf); } }
  // every formatter setting that Z1.cpp uses, over the whole value range
  struct { int exp, prec; float f1, f2, f3, f4; } uses[] = { {1,0,80.0f,0.069327f,0,0}, {1,0,400.0f,0.0692415f,0,0}, {0,1,0,680.0f,251.0f,0}, {0,1,0,1,2.0f,0}, {1,0,200.0f,0.086704f,0,0}, {0,1,0.5f,10.0f,95.0f,0}, {0,2,-1.0f,2.0f,150.0f,-50.0f}, {0,1,0,50.0f,140.0f,0} };
  for (auto& u : uses) for (int i = -100; i <= 251; i++) { std::string o; if (u.exp) { z1ExpFunc(i, u.prec, u.f1, u.f2, u.f3, u.f4, buf, sizeof buf); o = oldExp(i, u.prec, u.f1, u.f2, u.f3, u.f4); } else { z1ScaleFunc(i, u.prec, u.f1, u.f2, u.f3, u.f4, buf, sizeof buf); o = oldScale(i, u.prec, u.f1, u.f2, u.f3, u.f4); } n++; if (o != buf) { if (bad < 5) printf("fmt i=%d: old [%s] new [%s]\n", i, o.c_str(), buf); bad++; } }
  printf("%s %d comparisons of new display text against the old code, %d differ\n", bad ? "FAIL" : "ok  ", n, bad);
  printf(bad ? "%d FAILED\n" : "all passed\n", bad); return bad; }
