var autowatch, inlets, outlets, jsarguments = ["x", "z1"], inlet = 0, out = [], clock = 0, tasks = [];
function post() {} function outlet() { out.push([].slice.call(arguments)); } function arrayfromargs(a) { return [].slice.call(a); }
function Task(f) { this.f = f; this.at = -1; tasks.push(this); }
Task.prototype.schedule = function (ms) { this.at = clock + (ms || 0); }; Task.prototype.cancel = function () { this.at = -1; };
function advance(ms) { var end = clock + ms; for (;;) { var n = null; for (var i = 0; i < tasks.length; i++) if (tasks[i].at >= 0 && tasks[i].at <= end && (!n || tasks[i].at < n.at)) n = tasks[i]; if (!n) break; clock = n.at; n.at = -1; n.f(); } clock = end; }
this.patcher = { filepath: "" };
load("patchsync.js"); DEBUG = false;   // run from this folder: jsc patchsync-test.js
function hex() { return out.filter(function (o) { return o[0] === 0; }).map(function (o) { return (o[1] < 16 ? "0" : "") + o[1].toString(16); }).join(" "); }
function disp(n) { var v = out.filter(function (o) { return o[0] === n; }); return v.length ? String(v[v.length - 1][2]).replace(/ /g, " ") : ""; }
var fails = 0; function expect(l, got, want) { var ok = got === want; if (!ok) fails++; print((ok ? "ok   " : "FAIL ") + l + (ok ? "" : "\n   got  " + got + "\n   want " + want)); out = []; }
function st(mode, bank, pc, uni, w, pt, dt) { onSysex([0xf0, 0x7d, 0x5a, 0x31, 0x11, mode, uni || 0, w || 0, bank, pc, pt || 0, dt || 0, 0xf7]); }
loadbang(); expect("asks the Buddy for its status on load", hex(), "f0 7d 5a 31 01 f7");
// set load: the saved MPE state arrives before the dials; MPE must wait for the patch
mpe(1); advance(100); dialbank(1); dialpatch(42); advance(1000); expect("on load, the patch goes out first, then the name is read", hex(), "b0 00 00 b0 20 01 c0 2a f0 42 30 46 10 00 f7");
advance(1000); expect("then MPE is entered", hex(), "f0 7d 5a 31 02 01 f7");
st(0, 1, 42); advance(4000); out = [];
banklsb(1); pgm(42); advance(50); currentName = "Lunar Pad"; out = [];
mpe(1); advance(10); expect("MPE button sends the mode command at once", hex(), "f0 7d 5a 31 02 01 f7");
st(1, 1, 42); expect("entering", disp(5) + "|" + disp(6) + "|" + disp(4), "Entering MPE...|1|MPE: Lunar Pad");
st(2, 1, 42); expect("in MPE: shows the source patch and the slot", disp(5), "MPE: B 42 in A 127");
pgm(127); banklsb(0); advance(2000); expect("program change from the Z1 is ignored, no name request", hex() + "|" + pc + "|" + bank, "|42|1");
dialpatch(50); advance(200); expect("dial is locked and put back", hex() + "|" + disp(5) + "|" + JSON.stringify(out.filter(function (o) { return o[0] === 2; }).pop()), '|MPE mode: leave it to change patch|[2,"set",42]');
getsysex(); expect("no save in MPE", hex() + "|" + disp(5), "|MPE mode: leave it to save");
st(2, 0, 7); expect("Buddy reports another source: dials follow, old name dropped", pc + "|" + bank + "|" + disp(4), "7|0|MPE: (name not read)");
mpe(0); out = []; st(0, 0, 7); advance(100); expect("back to normal", disp(5) + "|" + disp(6), "Normal mode|0");
advance(1000); expect("name is read again after leaving", hex(), "f0 42 30 46 10 00 f7");
dialbank(0); dialpatch(127); advance(200); expect("working slot is flagged", disp(5), "Sent A 127 (MPE working slot)");
// state kept in the set
st(0, 0, 127, 2, 40, 10, 25); expect("pots and unison are stored when they change", JSON.stringify(out.filter(function (o) { return o[0] === 7; }).pop()), "[7,[2504,2,40,10,25]]");
st(0, 0, 127, 2, 40, 10, 25); expect("not stored again when nothing changed", out.filter(function (o) { return o[0] === 7; }).length, 0);
// a set being opened: MPE was on, unison 3, pots 40/10/25, Buddy is in Normal mode with other values
buddyMode = -1; loadbang(); out = []; restorebuddy(2504, 3, 40, 10, 25); mpe(1); st(0, 0, 5, 0, 0, 0, 0);
dialbank(1); dialpatch(42); advance(1100); out = [];
advance(150); expect("pots go back after the patch", hex(), "f0 7d 5a 31 04 28 0a 19 f7");
advance(400); expect("then the name is read and MPE entered", hex(), "f0 42 30 46 10 00 f7 f0 7d 5a 31 02 01 f7");
st(1, 1, 42, 0, 40, 10, 25); expect("no unison while entering", hex(), "");
st(2, 1, 42, 0, 40, 10, 25); expect("unison once MPE is on", hex(), "f0 7d 5a 31 03 03 f7");
advance(4000); st(2, 1, 42, 3, 40, 10, 25); expect("the restored state is not stored a second time", out.filter(function (o) { return o[0] === 7; }).length, 0);
st(2, 1, 42, 3, 55, 10, 25); expect("a pot moved on the Buddy afterwards is stored", JSON.stringify(out.filter(function (o) { return o[0] === 7; }).pop()), "[7,[2504,3,55,10,25]]");
print(fails ? fails + " FAILED" : "all passed");
