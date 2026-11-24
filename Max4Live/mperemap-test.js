var autowatch, inlets, outlets, out = [], clock = 1000, tasks = [], files = {};
function post(s) { print("POST " + s); }
function outlet() { out.push([].slice.call(arguments)); }
function arrayfromargs(a) { return [].slice.call(a); }
function Task(f) { this.f = f; this.at = -1; tasks.push(this); }
Task.prototype.schedule = function (ms) { this.at = clock + (ms || 0); };
Task.prototype.cancel = function () { this.at = -1; };
function advance(ms) { var end = clock + ms; for (;;) { var n = null; for (var i = 0; i < tasks.length; i++) if (tasks[i].at >= 0 && tasks[i].at <= end && (!n || tasks[i].at < n.at)) n = tasks[i]; if (!n) break; clock = Math.max(clock, n.at); n.at = -1; n.f(); } clock = end; }
var RealDate = Date; Date = function () { return { getTime: function () { return clock; }, toString: function () { return "T"; } }; };
function File(path, mode) { this.path = path; this.mode = mode; this.isopen = mode !== "read" || files[path] !== undefined; if (mode === "write" && files[path] === undefined) files[path] = ""; this.position = 0; this.eof = (files[path] || "").length; }
File.prototype.readstring = function (n) { var s = files[this.path].substr(this.position, n); this.position += n; return s; };
File.prototype.writestring = function (s) { if (this.eof === 0 && this.position === 0 && !this.w) files[this.path] = ""; this.w = 1; files[this.path] += s; };
File.prototype.writeline = function (s) { files[this.path] += s + "\n"; };
File.prototype.close = function () {};
this.patcher = { filepath: "/dev/MPE Remap.amxd" };
load("mperemap.js");
function hex(a) { return a.map(function (x) { return (x < 16 ? "0" : "") + x.toString(16); }).join(" "); }
function midi() { var b = out.filter(function (o) { return o[0] === 0; }).map(function (o) { return o[1]; }); return hex(b); }
function other() { return out.filter(function (o) { return o[0] !== 0; }).map(function (o) { return JSON.stringify(o).replace(/ /g, "_"); }).join(" "); }
var fails = 0;
function expect(label, got, want) { var ok = got === want; if (!ok) fails++; print((ok ? "ok   " : "FAIL ") + label + (ok ? "" : "\n   got  " + got + "\n   want " + want)); out = []; }
function setTargets(b, s, pr) { p("bend", "target", TARGETS.indexOf(b)); p("slide", "target", TARGETS.indexOf(s)); p("press", "target", TARGETS.indexOf(pr)); }

loadbang(); print(other()); out = [];
function rest() { advance(20); for (var c = 2; c <= 4; c++) { bend(0, c); slide(0, c); press(0, c); } advance(20); out = []; }
// 1. MPE thru: identical bytes, notes via js, order kept
setTargets("notebend", "noteslide", "notepress"); out = [];
bend(0, 2); slide(64, 2); press(0, 2); note(60, 100, 2); advance(10); press(50, 2); advance(10); bend(-8192, 2); advance(10); bend(8191, 2); advance(10); note(60, 0, 2);
expect("thru", midi(), "e1 00 40 b1 4a 40 d1 00 91 3c 64 d1 32 e1 00 00 e1 7f 7f 81 3c 00");
// 2. no rate limit: every value goes out at once, repeats are dropped
press(1, 2); press(2, 2); press(2, 2); press(3, 2); press(9, 2);
expect("burst, repeats dropped", midi(), "d1 01 d1 02 d1 03 d1 09");
// only what the controller sent goes out: no slide or pressure here
advance(20); bend(100, 3); bend(200, 3); note(62, 90, 3);
expect("values before note", midi(), "e2 64 40 e2 48 41 92 3e 5a");
note(62, 0, 3); rest();
// 3. channel AT: max of held notes, notes direct
setTargets("off", "off", "chanat"); advance(20);
expect("gate opens", other().indexOf("[1,1]") >= 0, true);
note(60, 100, 2); note(64, 100, 3); advance(10); press(40, 2); advance(10); press(90, 3); advance(10); press(10, 3); advance(10); note(60, 0, 2); advance(10); note(64, 0, 3); advance(10);
expect("chan AT max, no notes from js", midi(), "d0 00 d0 28 d0 5a d0 28 d0 0a d0 00");
rest();
// 4. global pitch bend with synth range 2: 2 of 48 semitones = full up
setTargets("pitchbend", "cc", "polyat"); p("slide", "cc", 1); p("range", 2); advance(20); out = [];
note(60, 100, 2); advance(10); bend(Math.round(8192 * 1 / 48), 2); advance(10); bend(Math.round(8192 * 2 / 48), 2); advance(10); bend(8191, 2); advance(10); slide(127, 2); advance(10); press(77, 2); advance(10); note(60, 0, 2); advance(10);
expect("pb range, cc, poly", midi(), "a1 3c 00 e0 00 40 b0 01 00 e0 08 60 e0 78 7f e0 7f 7f b0 01 7f a1 3c 4d e0 00 40");
rest();
// 5. bend to CC is centred; min and max
setTargets("cc", "off", "chanat"); p("bend", "cc", 16); p("range", 1); p("press", "min", 50); p("press", "max", 100); advance(20); out = [];
note(60, 100, 2); advance(10); bend(-Math.round(8192 / 48), 2); advance(10); press(32, 2); advance(10); press(100, 2); advance(10);
expect("bend cc centred, min max", midi(), "d0 40 b0 10 40 b0 10 01 d0 50 d0 72");
p("press", "min", 0); p("press", "curve", 50); advance(20); out = []; press(32, 2); advance(10);
expect("curve +50 lifts low values", midi(), "d0 " + hex([Math.round(Math.sqrt(32 / 127) * 127)]));
note(60, 0, 2); rest();
// 6. retarget returns old streams to rest
setTargets("off", "off", "chanat"); p("press", "curve", 0); advance(20); out = [];
note(60, 100, 2); advance(10); press(80, 2); advance(10); p("press", "target", TARGETS.indexOf("cc")); advance(10);
expect("retarget", midi(), "d0 50 d0 00 b0 0b 50");
note(60, 0, 2); rest();
// 6b. raw midievent input; bend on channel 1 follows the newest held note
setTargets("pitchbend", "off", "off"); p("range", 48); advance(20); out = [];
midievent(0x91, 60, 100); advance(10); midievent(0xe1, 0, 96); advance(10); midievent(0x92, 64, 100); advance(10);
midievent(0xe1, 0, 112); advance(10); midievent(0xe2, 0, 32); advance(10); midievent(0x82, 64, 0); advance(10); midievent(0x81, 60, 64); advance(10);
expect("midievent, newest note bend", midi(), "e0 00 40 e0 00 60 e0 00 40 e0 00 20 e0 00 70 e0 00 40");
rest();
// 6c. raw bytes: running status, realtime in the middle, sysex ignored
setTargets("notebend", "off", "chanat"); p("range", 48); advance(20); out = [];
[0x93, 60, 100, 0xe3, 0, 0xf8, 96, 0, 32, 0xf0, 1, 2, 0xf7, 0xd3, 50, 0xfe, 60, 0x83, 60, 0].forEach(function (b) { msg_int(b); advance(5); });
expect("raw bytes", midi(), "93 3c 64 d0 00 e3 00 60 e3 00 20 d0 32 d0 3c 83 3c 00 d0 00");
rest();
setTargets("off", "off", "cc"); p("range", 1); advance(20); out = [];
// 6d. a controller is never sent unless played, and never returned to 0
setTargets("off", "cc", "off"); p("slide", "cc", 74); advance(20); out = [];
midievent(0x98, 60, 100); advance(10);
expect("no value before the key position is played", midi(), "");
midievent(0xb8, 74, 90); advance(10); midievent(0x88, 60, 0); advance(10);
expect("value sent, kept after the note", midi(), "b0 4a 5a");
p("slide", "target", TARGETS.indexOf("off")); advance(10); midievent(0x98, 60, 100); midievent(0xb8, 74, 20); midievent(0xb0, 74, 5); midievent(0x88, 60, 0); advance(10);
expect("off sends nothing at all", midi(), "");
p("slide", "cc", 1); setTargets("off", "off", "cc"); p("range", 1); advance(20); out = [];
// 6e. inverted range, and bend depth
setTargets("pitchbend", "off", "chanat"); p("range", 48); p("press", "min", 100); p("press", "max", 0); p("bend", "max", 50); advance(20); out = [];
midievent(0x99, 60, 100); midievent(0xd9, 0); midievent(0xd9, 127); midievent(0xe9, 0, 127); midievent(0x89, 60, 0); advance(10);
expect("inverted pressure, half bend", midi(), "d0 7f d0 00 e0 40 5f d0 7f e0 00 40");
p("press", "min", 0); p("press", "max", 100); p("bend", "max", 100); p("slide", "cc", 1); setTargets("off", "off", "cc"); p("range", 1); advance(20); out = [];
// 7. presets: save, file, menu, load, delete, restore
save("UB-Xa"); expect("save", other().indexOf("Saved:_UB-Xa") >= 0 && other().indexOf('"append","UB-Xa"') >= 0, true);
var file = JSON.parse(files["/dev/mperemap-presets.json"]);
expect("file", JSON.stringify(file.instruments["UB-Xa"]), '{"timbres":{},"default":{"range":1,"synth":"Generic","bend":{"target":"off","cc":16,"min":0,"max":100,"curve":0},"slide":{"target":"off","cc":1,"min":0,"max":100,"curve":0},"press":{"target":"cc","cc":11,"min":0,"max":100,"curve":0}}}');
preset("MPE", "thru"); expect("load factory", other().indexOf('[2,"presstarget",3]') >= 0 && other().indexOf("Preset:_MPE_thru") >= 0 && map.range === 48, true);
p("press", "max", 80); expect("edited", other().indexOf("MPE_thru_(edited)") >= 0, true);
preset("UB-Xa"); expect("load user", map.press.target === "cc" && map.range === 1, true);
var st = out; preset("UB-Xa"); var stored = out.filter(function (o) { return o[0] === 5; })[0][1]; out = [];
currentName = ""; restore.apply(this, stored); expect("restore name", currentName + "|" + (other().indexOf('"setsymbol","UB-Xa"') >= 0), "UB-Xa|true");
del(); expect("delete", other().indexOf("Deleted:_UB-Xa") >= 0 && JSON.parse(files["/dev/mperemap-presets.json"]).instruments["UB-Xa"] === undefined, true);
// 8. synth-aware controller names
files["/dev/mperemap-synths.json"] = JSON.stringify({ synths: { "Z1": { cc: { "85": "Filter1 Cutoff", "16": "X-Y Pad (X)" } } } });
loadSynths(); out = []; synth("Z1");
var o8 = other();
expect("synth menus", [o8.indexOf('[6,"press","append",85,"Filter1","Cutoff"]') >= 0, o8.indexOf('[6,"bend","append",74]') >= 0, o8.indexOf('[6,"press","set",11]') >= 0, map.synth].join(), "true,true,true,Z1");
p("press", "cc", 85); expect("cc sets menu", other().indexOf('[6,"press","set",85]') >= 0, true);
save("Z1 test"); out = []; preset("MPE", "thru"); expect("preset brings its synth", map.synth + "|" + (other().indexOf('[7,"setsymbol","Generic"]') >= 0), "Generic|true");
preset("Z1", "test"); var st8 = out.filter(function (o) { return o[0] === 5; })[0][1]; expect("user preset synth", map.synth + "|" + map.press.cc, "Z1|85");
currentName = ""; map.synth = "Generic"; restore.apply(this, st8); expect("restore both names", currentName + "|" + map.synth, "Z1 test|Z1");
restore(2502, 85, 66); expect("restore old format", currentName + "|" + map.synth, "UB|Z1");
log(); expect("log", /input channels seen: (1 )?2 3 4/.test(files["/dev/mperemap-debug.log"]) && other().indexOf("Logged") >= 0, true);
print(fails ? fails + " FAILED" : "all passed");
