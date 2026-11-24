// mperemap.js - remaps the three MPE dimensions of a controller (written for
// the Expressive E Osmose, which has no presets of its own as a MIDI
// controller) to other MIDI messages.
//
// Used by "MPE Remap.amxd" as [js mperemap.js]. Max [js], so ES5 only.
//
// The device must have MPE enabled, so Live hands it one note per channel:
// channel 1 is the global channel, channels 2-16 carry one note each with
//   bend   pitch bend        (sideways on the Osmose)
//   slide  CC 74             (key position / timbre)
//   press  channel pressure  (aftertouch)
//
// Each of the three can be sent to one target:
//   Off         dropped
//   Note Bend   per-note pitch bend   (stays MPE)
//   Note Slide  per-note CC 74        (stays MPE)
//   Note Press  per-note pressure     (stays MPE)
//   Poly AT     polyphonic key pressure for the note on that channel
//   Chan AT     channel aftertouch on channel 1
//   Pitch Bend  pitch bend on channel 1
//   CC          a controller on channel 1
// Targets on channel 1 get one value for all notes: the highest slide or
// pressure among the held notes, or the bend of the newest held note.
//
// Inlet 0:
//   int                            raw MIDI bytes straight from [midiin],
//                                  through one [deferlow] so the order is kept
//   p <source> <field> <value>     a control on the device changed
//   preset <name...> | save <name...> | del | restore ... | log
//
// Outlet 0: MIDI bytes to a [midiout] of their own
// Outlet 1: 1 = the patcher passes notes itself, 0 = notes come from here
// Outlet 2: "<source><field> value" to set the device controls
// Outlet 3: messages for the preset menu
// Outlet 4: status text
// Outlet 5: preset and synth name to [pattr], so the menus show them again
//           after loading
// Outlet 6: "<source> ..." messages for the three controller name menus
// Outlet 7: messages for the synth menu
//
// The synth menu only decides which names the controller menus show; the
// names come from mperemap-synths.json next to the device.

autowatch = 1;
inlets = 1;
outlets = 8;

var OUT_MIDI = 0, OUT_GATE = 1, OUT_PARAM = 2, OUT_MENU = 3, OUT_STATUS = 4, OUT_STORE = 5,
    OUT_CCMENU = 6, OUT_SYNTHMENU = 7;

var SOURCES = ["bend", "slide", "press"];
var TARGETS = ["off", "notebend", "noteslide", "notepress", "polyat", "chanat", "pitchbend", "cc"];
var FIELDS = ["target", "cc", "min", "max", "curve"];
var IN_BEND_SEMITONES = 48;   // range of per-note pitch bend as Live delivers it
var STORE_MAGIC = 2503;      // [magic, length of preset name, preset name, synth name]
var STORE_MAGIC_V1 = 2502;   // [magic, preset name]
var PRESET_FILE = "mperemap-presets.json";
var SYNTH_FILE = "mperemap-synths.json";

// ---------------------------------------------------------------- mapping

// A fresh device has everything Off: nothing is sent until a target is chosen.
function defaultMapping() {
    return {
        bend: { target: "off", cc: 1, min: 0, max: 100, curve: 0 },
        slide: { target: "off", cc: 74, min: 0, max: 100, curve: 0 },
        press: { target: "off", cc: 11, min: 0, max: 100, curve: 0 },
        range: 48,     // semitones: see bendPosition()
        synth: "Generic"
    };
}

function copyMapping(m) {
    var out = { range: m.range, synth: m.synth };
    for (var i = 0; i < SOURCES.length; i++) {
        var s = m[SOURCES[i]];
        out[SOURCES[i]] = { target: s.target, cc: s.cc, min: s.min, max: s.max, curve: s.curve };
    }
    return out;
}

function sameMapping(a, b) {
    if (Math.abs(a.range - b.range) > 0.001 || a.synth !== b.synth) return false;
    for (var i = 0; i < SOURCES.length; i++) {
        var x = a[SOURCES[i]], y = b[SOURCES[i]];
        if (x.target !== y.target || x.min !== y.min || x.max !== y.max || x.curve !== y.curve) return false;
        if (x.target === "cc" && x.cc !== y.cc) return false;
    }
    return true;
}

// Makes a mapping read from a file safe to use.
function cleanMapping(m) {
    var out = defaultMapping();
    if (!m || typeof m !== "object") return out;
    if (typeof m.range === "number" && m.range > 0) out.range = Math.min(96, m.range);
    if (typeof m.synth === "string" && m.synth) out.synth = m.synth;
    for (var i = 0; i < SOURCES.length; i++) {
        var s = m[SOURCES[i]], d = out[SOURCES[i]];
        if (!s || typeof s !== "object") continue;
        if (TARGETS.indexOf(s.target) >= 0) d.target = s.target;
        if (typeof s.cc === "number") d.cc = clamp(Math.round(s.cc), 0, 127);
        if (typeof s.min === "number") d.min = clamp(Math.round(s.min), 0, 100);
        if (typeof s.max === "number") d.max = clamp(Math.round(s.max), 0, 100);
        else if (typeof s.amount === "number") d.max = clamp(Math.round(s.amount), 0, 100);   // older files
        if (typeof s.curve === "number") d.curve = clamp(Math.round(s.curve), -100, 100);
    }
    return out;
}

var map = defaultMapping();

// Starting points only. Saving a preset under one of these names replaces it.
var FACTORY = {
    "MPE thru": {
        bend: { target: "notebend", cc: 1, min: 0, max: 100, curve: 0 },
        slide: { target: "noteslide", cc: 74, min: 0, max: 100, curve: 0 },
        press: { target: "notepress", cc: 11, min: 0, max: 100, curve: 0 },
        range: 48
    },
    "Poly AT": {
        bend: { target: "pitchbend", cc: 1, min: 0, max: 100, curve: 0 },
        slide: { target: "cc", cc: 1, min: 0, max: 100, curve: 0 },
        press: { target: "polyat", cc: 11, min: 0, max: 100, curve: 0 },
        range: 2
    },
    "Channel AT": {
        bend: { target: "pitchbend", cc: 1, min: 0, max: 100, curve: 0 },
        slide: { target: "cc", cc: 1, min: 0, max: 100, curve: 0 },
        press: { target: "chanat", cc: 11, min: 0, max: 100, curve: 0 },
        range: 2
    }
};

// ---------------------------------------------------------------- state

function clamp(v, lo, hi) { return v < lo ? lo : (v > hi ? hi : v); }

// Per channel, index 1-16.
var held = [];      // pitches sounding on the channel
var vBend = [];     // -8192..8191
var vSlide = [];    // 0..127
var vPress = [];    // 0..127
var got = [];       // which of bend, slide, press the channel has received so far
for (var c0 = 0; c0 <= 16; c0++) { held.push([]); vBend.push(0); vSlide.push(0); vPress.push(0); got.push({}); }

var order = [];         // channels with held notes, oldest first
var notesViaJs = false;

var sent = {};          // stream key -> value last sent

// ---------------------------------------------------------------- synths

// Controller names per synth, read from SYNTH_FILE. A synth only names the
// controllers it knows; the others show as plain numbers, because the usual
// MIDI meaning of a number often does not hold on a given synth.
var synths = { "Generic": { cc: {"0": "Bank Select MSB", "1": "Mod Wheel", "2": "Breath", "4": "Foot Controller", "5": "Portamento Time", "6": "Data Entry MSB", "7": "Volume", "8": "Balance", "10": "Pan", "11": "Expression", "32": "Bank Select LSB", "64": "Sustain Pedal", "65": "Portamento", "66": "Sostenuto", "67": "Soft Pedal", "71": "Resonance", "72": "Release Time", "73": "Attack Time", "74": "Brightness", "91": "Reverb Send", "93": "Chorus Send"} } };
var synthsLoaded = false;

// Reads a JSON file next to the device. Returns undefined when the device
// folder is not known yet (Live can call loadbang before it is), null when
// there is no such file. fileInfo keeps what happened, for the log.
var fileInfo = {};

function readJson(name) {
    var dir = folder();
    if (!dir) { fileInfo[name] = "device folder not known yet"; return undefined; }
    // No file type is given first: with "TEXT" Max can refuse a .json file.
    var f = new File(dir + name, "read");
    if (!f.isopen) f = new File(dir + name, "read", "JSON");
    if (!f.isopen) f = new File(dir + name, "read", "TEXT");
    if (!f.isopen) { fileInfo[name] = "not found: " + dir + name; return null; }
    var text = "", guard = 0;
    while (f.position < f.eof && guard++ < 10000) text += f.readstring(4096);
    f.close();
    fileInfo[name] = "read " + text.length + " characters";
    return JSON.parse(text);
}

function loadSynths() {
    try {
        var data = readJson(SYNTH_FILE);
        if (data === undefined) return;          // try again later
        synthsLoaded = true;
        if (data && typeof data.synths === "object" && data.synths) {
            for (var n in data.synths) {
                if (data.synths.hasOwnProperty(n) && data.synths[n] && typeof data.synths[n].cc === "object") {
                    synths[n] = data.synths[n];
                }
            }
        }
    } catch (e) {
        synthsLoaded = true;
        fileInfo[SYNTH_FILE] = "error: " + e;
        post("mperemap: could not read " + SYNTH_FILE + ": " + e + "\n");
    }
}

function ccName(n) {
    var syn = synths[map.synth];
    var name = syn && syn.cc ? syn.cc[String(n)] : null;
    return typeof name === "string" ? name : "";
}

function fillSynthMenu() {
    outlet(OUT_SYNTHMENU, "clear");
    var names = [], n;
    for (n in synths) if (synths.hasOwnProperty(n)) names.push(n);
    if (names.indexOf(map.synth) < 0) names.push(map.synth);   // named in a preset, not in the file
    for (var i = 0; i < names.length; i++) outlet(OUT_SYNTHMENU, "append", names[i]);
    outlet(OUT_SYNTHMENU, "setsymbol", map.synth);
}

// The three controller menus list all 128 numbers, so the position in the
// menu is the controller number.
function fillCcMenus() {
    for (var i = 0; i < SOURCES.length; i++) {
        var s = SOURCES[i];
        outlet(OUT_CCMENU, s, "clear");
        for (var n = 0; n < 128; n++) {
            var words = ccName(n).split(/\s+/), msg = [OUT_CCMENU, s, "append", n];
            for (var w = 0; w < words.length; w++) if (words[w]) msg.push(words[w]);
            outlet.apply(this, msg);
        }
        outlet(OUT_CCMENU, s, "set", map[s].cc);
    }
}

// The synth menu was used.
function synth() {
    ensureFiles();
    var name = nameFromArgs(arguments);
    if (!name) return;
    map.synth = name;
    rememberName();
    fillCcMenus();
    showState();
}

// ---------------------------------------------------------------- presets

var store = { version: 1, instruments: {} };
var storeLoaded = false;
var currentName = "";

function folder() {
    var path = String(this.patcher.filepath);
    var cut = path.lastIndexOf("/");
    return cut < 0 ? null : path.substring(0, cut + 1);
}

function loadStore() {
    try {
        var data = readJson(PRESET_FILE);
        if (data === undefined) return;          // try again later
        storeLoaded = true;
        if (data && typeof data.instruments === "object" && data.instruments) store = data;
    } catch (e) {
        storeLoaded = true;
        fileInfo[PRESET_FILE] = "error: " + e;
        post("mperemap: could not read " + PRESET_FILE + ": " + e + "\n");
    }
}

// The files could not be read at loadbang if Live had not given the device
// its folder yet, so every later message tries again until it works.
function ensureFiles() {
    if (!synthsLoaded) {
        loadSynths();
        if (synthsLoaded) { fillSynthMenu(); fillCcMenus(); }
    }
    if (!storeLoaded) {
        loadStore();
        if (storeLoaded) fillMenu();
    }
}

function saveStore() {
    try {
        var dir = folder();
        if (!dir) { post("mperemap: no device folder, presets not written\n"); return false; }
        var f = new File(dir + PRESET_FILE, "write", "TEXT");
        if (!f.isopen) { post("mperemap: could not open " + PRESET_FILE + "\n"); return false; }
        f.eof = 0;
        var text = JSON.stringify(store, null, 1);
        for (var i = 0; i < text.length; i += 2000) f.writestring(text.substring(i, i + 2000));
        f.close();
        return true;
    } catch (e) {
        post("mperemap: could not write " + PRESET_FILE + ": " + e + "\n");
        return false;
    }
}

// The file keeps one entry per instrument: "default" is the mapping, and
// "timbres" is reserved for mappings per patch name on that instrument.
function presetMapping(name) {
    var inst = store.instruments[name];
    if (inst && inst["default"]) return cleanMapping(inst["default"]);
    if (FACTORY.hasOwnProperty(name)) return cleanMapping(FACTORY[name]);
    return null;
}

function presetNames() {
    var names = [], n;
    for (n in FACTORY) if (FACTORY.hasOwnProperty(n)) names.push(n);
    var user = [];
    for (n in store.instruments) {
        if (store.instruments.hasOwnProperty(n) && !FACTORY.hasOwnProperty(n)) user.push(n);
    }
    user.sort();
    return names.concat(user);
}

function fillMenu() {
    outlet(OUT_MENU, "clear");
    var names = presetNames();
    for (var i = 0; i < names.length; i++) outlet(OUT_MENU, "append", names[i]);
    if (currentName && presetMapping(currentName)) outlet(OUT_MENU, "setsymbol", currentName);
}

function show(text) {
    outlet(OUT_STATUS, "set", String(text).replace(/ /g, " "));
}

function showState() {
    var saved = currentName ? presetMapping(currentName) : null;
    if (!saved) show("Not saved as a preset");
    else if (sameMapping(saved, map)) show("Preset: " + currentName);
    else show(currentName + " (edited)");
}

function rememberName() {
    var list = [STORE_MAGIC, currentName.length], i;
    for (i = 0; i < currentName.length; i++) list.push(currentName.charCodeAt(i));
    for (i = 0; i < map.synth.length; i++) list.push(map.synth.charCodeAt(i));
    outlet(OUT_STORE, list);
}

function nameFromArgs(args) {
    var parts = [];
    for (var i = 0; i < args.length; i++) parts.push(String(args[i]));
    return parts.join(" ").replace(/^\s+|\s+$/g, "");
}

function loadbang() {
    ensureFiles();
    fillMenu();
    fillSynthMenu();
    fillCcMenus();
    showState();
    outlet(OUT_GATE, notesViaJs ? 0 : 1);
}

// A preset was picked in the menu: set the device controls. They report back
// through p(), which is what changes the mapping.
function preset() {
    ensureFiles();
    var name = nameFromArgs(arguments);
    var m = presetMapping(name);
    if (!m) { show("No preset " + name); return; }
    currentName = name;
    for (var i = 0; i < SOURCES.length; i++) {
        var s = SOURCES[i];
        outlet(OUT_PARAM, s + "target", TARGETS.indexOf(m[s].target));
        outlet(OUT_PARAM, s + "cc", m[s].cc);
        outlet(OUT_PARAM, s + "min", m[s].min);
        outlet(OUT_PARAM, s + "max", m[s].max);
        outlet(OUT_PARAM, s + "curve", m[s].curve);
    }
    outlet(OUT_PARAM, "range", m.range);
    // In case a control did not report back (same value as before).
    map = m;
    rememberName();
    fillSynthMenu();
    fillCcMenus();
    retarget();
    showState();
}

function save() {
    ensureFiles();
    var name = nameFromArgs(arguments);
    if (name === "bang") name = "";            // Save pressed with an empty name box
    name = name || currentName;
    if (!name) { show("Type a name first"); return; }
    var inst = store.instruments[name];
    if (!inst || typeof inst !== "object") inst = { timbres: {} };
    inst["default"] = copyMapping(map);
    store.instruments[name] = inst;
    var ok = saveStore();
    currentName = name;
    rememberName();
    fillMenu();
    show(ok ? "Saved: " + name : "Could not write presets");
}

function del() {
    ensureFiles();
    if (!currentName || !store.instruments[currentName]) { show("Not a saved preset"); return; }
    var name = currentName;
    delete store.instruments[name];
    var ok = saveStore();
    if (!FACTORY.hasOwnProperty(name)) currentName = "";
    rememberName();
    fillMenu();
    show(ok ? "Deleted: " + name : "Could not write presets");
}

// Preset name coming back from [pattr] when the Live set loads. The controls
// are restored by Live itself; this only puts the name back in the menu.
function restore() {
    ensureFiles();
    var a = arrayfromargs(arguments);
    if (!a.length) return;
    var magic = Math.round(a[0]), split, name = "", syn = "", i;
    if (magic === STORE_MAGIC) split = 2 + Math.round(a[1]);
    else if (magic === STORE_MAGIC_V1) split = a.length;
    else return;
    for (i = (magic === STORE_MAGIC ? 2 : 1); i < a.length; i++) {
        if (i < split) name += String.fromCharCode(Math.round(a[i]));
        else syn += String.fromCharCode(Math.round(a[i]));
    }
    currentName = name;
    if (syn) map.synth = syn;
    fillMenu();
    fillSynthMenu();
    fillCcMenus();
    showState();
}

// ---------------------------------------------------------------- controls

function p(source, field, value) {
    ensureFiles();
    if (source === "range") {
        value = Number(field);
        if (!(value > 0)) return;
        map.range = value;
    } else {
        var s = map[source];
        if (!s || FIELDS.indexOf(field) < 0) return;
        value = Math.round(value);
        if (field === "target") {
            if (value < 0 || value >= TARGETS.length) return;
            s.target = TARGETS[value];
        } else if (field === "cc") {
            s.cc = clamp(value, 0, 127);
            outlet(OUT_CCMENU, source, "set", s.cc);
        } else if (field === "min") {
            s.min = clamp(value, 0, 100);
        } else if (field === "max") {
            s.max = clamp(value, 0, 100);
        } else {
            s.curve = clamp(value, -100, 100);
        }
    }
    retarget();
    showState();
}

function isNoteTarget(t) { return t === "notebend" || t === "noteslide" || t === "notepress"; }

// After a change of mapping: decide who sends the notes, return streams that
// are no longer fed to rest, and send the current values to the new targets.
function retarget() {
    var viaJs = false, i;
    for (i = 0; i < SOURCES.length; i++) if (isNoteTarget(map[SOURCES[i]].target)) viaJs = true;
    if (viaJs !== notesViaJs) {
        // Per-note streams must stay in order with the notes, so then the
        // notes go through here too. Otherwise the patcher passes them
        // directly, which keeps their timing tight.
        notesViaJs = viaJs;
        outlet(OUT_GATE, notesViaJs ? 0 : 1);
    }
    var live = {};
    for (i = 0; i < SOURCES.length; i++) {
        var s = map[SOURCES[i]];
        if (s.target === "chanat") live["g:at"] = true;
        else if (s.target === "pitchbend") live["g:pb"] = true;
        else if (s.target === "cc") live["g:cc" + s.cc] = true;
    }
    // A stream that is no longer fed: aftertouch and pitch bend go back to
    // rest. A controller is left where it is; 0 is not "nothing" for a synth
    // parameter (on the UB-Xa, CC 74 at 0 closes the filter).
    for (var key in sent) {
        if (!sent.hasOwnProperty(key) || key.charAt(0) !== "g" || live[key]) continue;
        if (key === "g:at") stream(key, [0xd0, 0], 0);
        else if (key === "g:pb") stream(key, [0xe0, 0, 64], 8192);
        delete sent[key];
    }
    updateGlobals();
    for (var ch = 2; ch <= 16; ch++) if (held[ch].length) updateChannel(ch);
}

// ---------------------------------------------------------------- shaping

// Position of a source between 0 and 1 (slide, press) or -1 and 1 (bend).
// The bend range control says how many semitones of bend are full scale:
// the pitch bend range of the receiving synth when the target is a pitch
// bend, or the amount of sideways movement that gives a full controller
// swing otherwise. 48 leaves an MPE bend as it is.
function bendPosition(raw) {
    return clamp(raw / 8192 * IN_BEND_SEMITONES / map.range, -1, 1);
}

// Curve, then the output range. Curve is an exponent: 0 is a straight line,
// +100 is x^0.25 (small values lifted), -100 is x^4 (small values held back).
// Min and Max are in percent of the target's range: slide and pressure run
// from Min to Max (Min above Max turns the direction around). A bend is
// two-sided around its centre, so only Max applies, as its depth.
function shape(source, x) {
    var s = map[source];
    var mag = Math.abs(x);
    if (s.curve !== 0) mag = Math.pow(mag, Math.pow(2, -s.curve / 50));
    if (source === "bend") {
        mag = mag * s.max / 100;
        return x < 0 ? -mag : mag;
    }
    return (s.min + mag * (s.max - s.min)) / 100;
}

// What one source contributes on one channel, already shaped.
function shaped(source, ch) {
    if (source === "bend") return shape("bend", bendPosition(vBend[ch]));
    return shape(source, (source === "slide" ? vSlide[ch] : vPress[ch]) / 127);
}

// Combines the sources feeding one target. kind: "uni" 0..127 where a bend
// counts by its size, "bi" 0..127 with a bend centred on 64, "pb" 0..16383.
function combine(values, kind) {
    var i, v;
    if (kind === "pb") {
        var sum = 0;
        for (i = 0; i < values.length; i++) sum += values[i].x;
        return clamp(Math.round(8192 + clamp(sum, -1, 1) * 8192), 0, 16383);
    }
    var best = 0, bipolar = false, bi = 0;
    for (i = 0; i < values.length; i++) {
        v = values[i];
        if (v.source === "bend" && kind === "bi") { bipolar = true; bi = v.x; }
        else best = Math.max(best, Math.abs(v.x));
    }
    if (bipolar && best === 0) return clamp(Math.round(64 + bi * 63), 0, 127);
    return clamp(Math.round(best * 127), 0, 127);
}

// ---------------------------------------------------------------- output

function now() { return new Date().getTime(); }

function sendBytes(bytes) {
    for (var i = 0; i < bytes.length; i++) outlet(OUT_MIDI, bytes[i]);
    logEvent("out", bytes);
}

// One stream = one kind of message on one channel (or one key). Repeats of
// the same value are dropped; everything else goes out at once.
function stream(key, bytes, value) {
    if (sent[key] === value) return;
    sent[key] = value;
    sendBytes(bytes);
}

function feeds(values, only) {
    if (!values.length) return false;
    if (!only) return true;
    for (var i = 0; i < values.length; i++) if (values[i].source === only) return true;
    return false;
}

// Per-note streams of one channel. With "only", just the streams that this
// source feeds, so a bend message does not also send slide and pressure.
function updateChannel(ch, only) {
    if (ch < 2) return;
    var c = ch - 1, i, s;
    var nb = [], ns = [], np = [], pa = [];
    for (i = 0; i < SOURCES.length; i++) {
        s = SOURCES[i];
        var t = map[s].target;
        if (!got[ch][s]) continue;           // never send a value nobody played
        if (t === "notebend") nb.push({ source: s, x: shaped(s, ch) });
        else if (t === "noteslide") ns.push({ source: s, x: shaped(s, ch) });
        else if (t === "notepress") np.push({ source: s, x: shaped(s, ch) });
        else if (t === "polyat") pa.push({ source: s, x: shaped(s, ch) });
    }
    var v;
    if (feeds(nb, only)) {
        v = combine(nb, "pb");
        stream("n" + ch + ":pb", [0xe0 | c, v & 0x7f, (v >> 7) & 0x7f], v);
    }
    if (feeds(ns, only)) {
        v = combine(ns, "bi");
        stream("n" + ch + ":sl", [0xb0 | c, 74, v], v);
    }
    if (feeds(np, only)) {
        v = combine(np, "uni");
        stream("n" + ch + ":at", [0xd0 | c, v], v);
    }
    if (feeds(pa, only)) {
        v = combine(pa, "uni");
        for (i = 0; i < held[ch].length; i++) {
            stream("n" + ch + ":pa" + held[ch][i], [0xa0 | c, held[ch][i], v], v);
        }
    }
}

// The channel whose bend counts for the targets on channel 1: the newest
// held note. (Following whichever note moved last does not work: every
// finger on an Osmose wobbles a little all the time, so the bend would jump
// between the notes of a chord.)
function bendChannel() {
    return order.length ? order[order.length - 1] : 0;
}

// What one source contributes to a channel 1 target: the highest slide or
// pressure among the held notes, or the bend of the newest held note.
function globalValue(source) {
    var ch, i;
    if (source === "bend") {
        ch = bendChannel();
        if (ch && got[ch].bend) return { source: source, x: shaped("bend", ch) };
        return { source: source, x: 0, rest: true };      // centre
    }
    var best = -1;
    for (i = 0; i < order.length; i++) {
        ch = order[i];
        if (got[ch][source]) best = Math.max(best, shaped(source, ch));
    }
    if (best >= 0) return { source: source, x: best };
    // No held note has a value. Pressure is then 0. The key position (CC 74)
    // has no rest value: the target keeps what it had.
    return source === "press" ? { source: source, x: shape("press", 0), rest: true } : null;
}

// A target on channel 1 is sent when a source gives it a real value. A rest
// value (no notes) only goes out to bring back a stream that was already
// sent, never to start one.
function wanted(values, only, key) {
    if (!feeds(values, only)) return false;
    for (var i = 0; i < values.length; i++) if (!values[i].rest) return true;
    return sent[key] !== undefined;
}

function updateGlobals(only) {
    var at = [], pb = [], cc = {}, i, s, v, g;
    for (i = 0; i < SOURCES.length; i++) {
        s = SOURCES[i];
        var m = map[s];
        if (m.target !== "chanat" && m.target !== "pitchbend" && m.target !== "cc") continue;
        g = globalValue(s);
        if (!g) continue;
        if (m.target === "chanat") at.push(g);
        else if (m.target === "pitchbend") pb.push(g);
        else {
            if (!cc[m.cc]) cc[m.cc] = [];
            cc[m.cc].push(g);
        }
    }
    if (wanted(at, only, "g:at")) {
        v = combine(at, "uni");
        stream("g:at", [0xd0, v], v);
    }
    if (wanted(pb, only, "g:pb")) {
        v = combine(pb, "pb");
        stream("g:pb", [0xe0, v & 0x7f, (v >> 7) & 0x7f], v);
    }
    for (var n in cc) {
        if (!cc.hasOwnProperty(n) || !wanted(cc[n], only, "g:cc" + n)) continue;
        v = combine(cc[n], "bi");
        stream("g:cc" + n, [0xb0, parseInt(n, 10), v], v);
    }
}

// ---------------------------------------------------------------- MIDI in

function validCh(ch) { return ch >= 1 && ch <= 16; }

// Raw MIDI bytes from [midiin]. The messages are put together here, because
// [midiparse] reports bend, pressure and controllers of an MPE stream as
// channel 1 on its midievent outlet (seen in a log: two notes on channels 4
// and 5, all their expression on channel 1).
var rxStatus = 0, rxNeed = 0, rxData = [];

function msg_int(v) {
    if (v >= 0xf8) return;                       // realtime, may come anywhere
    if (v >= 0x80) {
        rxData = [];
        if (v >= 0xf0) { rxStatus = 0; return; } // sysex and system common
        rxStatus = v;
        rxNeed = ((v & 0xf0) === 0xc0 || (v & 0xf0) === 0xd0) ? 1 : 2;
        return;
    }
    if (!rxStatus) return;
    rxData.push(v);
    if (rxData.length >= rxNeed) {
        var d = rxData;
        rxData = [];                             // running status
        midievent(rxStatus, d[0], d[1]);
    }
}

// One complete MIDI message. Only what is remapped is handled here; the
// patcher passes the rest (and the notes, unless notesViaJs) by itself.
function midievent(status, d1, d2) {
    status = Math.round(status);
    d1 = Math.round(d1 || 0) & 0x7f;
    d2 = Math.round(d2 || 0) & 0x7f;
    var ch = (status & 0x0f) + 1;
    switch (status & 0xf0) {
        case 0x90: note(d1, d2, ch); break;
        case 0x80: note(d1, 0, ch); break;
        case 0xb0: if (d1 === 74) slide(d2, ch); break;
        case 0xd0: press(d1, ch); break;
        case 0xe0: bend(((d2 << 7) | d1) - 8192, ch); break;
    }
}

function note(pitch, vel, ch) {
    pitch = Math.round(pitch); vel = Math.round(vel); ch = Math.round(ch);
    if (!validCh(ch)) return;
    logEvent("in", ["note", pitch, vel, ch]);
    var c = ch - 1, at = held[ch].indexOf(pitch), o;
    if (vel > 0) {
        if (at < 0) held[ch].push(pitch);
        if (ch >= 2) {
            o = order.indexOf(ch);
            if (o >= 0) order.splice(o, 1);
            order.push(ch);
            // The note's own bend, slide and pressure must be there first.
            updateChannel(ch);
        }
        if (notesViaJs) sendBytes([0x90 | c, pitch, vel]);
        if (ch >= 2) { updateChannel(ch); updateGlobals(); }
    } else {
        if (at >= 0) held[ch].splice(at, 1);
        if (notesViaJs) sendBytes([0x80 | c, pitch, 0]);
        delete sent["n" + ch + ":pa" + pitch];
        if (ch >= 2) {
            if (!held[ch].length) {
                o = order.indexOf(ch);
                if (o >= 0) order.splice(o, 1);
            }
            updateGlobals();
        }
    }
}

function bend(v, ch) {
    v = Math.round(v); ch = Math.round(ch);
    if (!validCh(ch)) return;
    logEvent("in", ["bend", v, ch]);
    if (ch === 1) {              // global bend: passed on as it is, unless Off
        if (map.bend.target === "off") return;
        var u = clamp(v + 8192, 0, 16383);
        sendBytes([0xe0, u & 0x7f, (u >> 7) & 0x7f]);
        return;
    }
    got[ch].bend = true;
    vBend[ch] = clamp(v, -8192, 8191);
    updateChannel(ch, "bend");
    updateGlobals("bend");
}

function slide(v, ch) {
    v = Math.round(v); ch = Math.round(ch);
    if (!validCh(ch)) return;
    logEvent("in", ["slide", v, ch]);
    if (ch === 1) {
        if (map.slide.target !== "off") sendBytes([0xb0, 74, clamp(v, 0, 127)]);
        return;
    }
    got[ch].slide = true;
    vSlide[ch] = clamp(v, 0, 127);
    updateChannel(ch, "slide");
    updateGlobals("slide");
}

function press(v, ch) {
    v = Math.round(v); ch = Math.round(ch);
    if (!validCh(ch)) return;
    logEvent("in", ["press", v, ch]);
    if (ch === 1) {
        if (map.press.target !== "off") sendBytes([0xd0, clamp(v, 0, 127)]);
        return;
    }
    got[ch].press = true;
    vPress[ch] = clamp(v, 0, 127);
    updateChannel(ch, "press");
    updateGlobals("press");
}

// ---------------------------------------------------------------- debug log

// The last events in and out are kept in memory. The Log button appends them
// to mperemap-debug.log next to the device.
var LOG_SIZE = 600;
var events = [];
var chanSeen = {};

function logEvent(dir, data) {
    if (dir === "in") chanSeen[data[data.length - 1]] = true;
    events.push([now(), dir, data]);
    if (events.length > LOG_SIZE) events.shift();
}

function hexOf(bytes) {
    var s = [];
    for (var i = 0; i < bytes.length; i++) s.push((bytes[i] < 16 ? "0" : "") + bytes[i].toString(16));
    return s.join(" ");
}

function log() {
    ensureFiles();
    var chans = [], k;
    for (k in chanSeen) if (chanSeen.hasOwnProperty(k)) chans.push(parseInt(k, 10));
    chans.sort(function (a, b) { return a - b; });
    var lines = ["--- " + new Date().toString() + " | preset [" + currentName + "] | notes " +
        (notesViaJs ? "through js" : "direct") + " | input channels seen: " + chans.join(" ")];
    lines.push("mapping " + JSON.stringify(map));
    lines.push("files " + JSON.stringify(fileInfo) + " | device " + String(this.patcher.filepath));
    var t0 = events.length ? events[0][0] : 0;
    for (var i = 0; i < events.length; i++) {
        var e = events[i];
        lines.push((e[0] - t0) + " " + e[1] + " " + (e[1] === "out" ? hexOf(e[2]) : e[2].join(" ")));
    }
    try {
        var dir = folder();
        if (!dir) { show("No device folder"); return; }
        var f = new File(dir + "mperemap-debug.log", "readwrite", "TEXT");
        if (!f.isopen) f = new File(dir + "mperemap-debug.log", "write", "TEXT");
        if (!f.isopen) { show("Could not open log"); return; }
        f.position = f.eof;
        for (var j = 0; j < lines.length; j++) f.writeline(lines[j]);
        f.close();
        show("Logged " + events.length + " events");
    } catch (err) {
        show("Log failed");
        post("mperemap: log failed: " + err + "\n");
    }
}
