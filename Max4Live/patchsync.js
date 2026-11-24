// patchsync.js - bank/patch sync and sysex save/restore for external synths.
//
// Used by the "Patch Sync <synth>.amxd" devices as [js patchsync.js <synth>].
// Written for the Max 8 [js] object, so ES5 only.
//
// Inlet 0: messages from the patcher
//   dialbank n / dialpatch n   the user (or Live) moved a dial
//   banklsb n / bankmsb n / pgm n   bank select and program change from the synth
//   resend                     send the current bank and patch again
//   getsysex                   request a dump and save it in the Live set
//   sendsysex                  send the saved dump to the synth
//   auto 0/1                   read the patch name after every program change
//   restore ...                saved dump coming back from [pattr] on load
//   mpe 0/1                    (Z1 Buddy) leave or enter MPE mode
//   restorebuddy ...           (Z1 Buddy) saved unison and pots coming back from [pattr]
// Inlet 1: one complete sysex message per list (F0 ... F7), or single bytes
//
// Outlet 0: raw MIDI bytes to [midiout]
// Outlet 1: "set n" to the bank dial      Outlet 2: "set n" to the patch dial
// Outlet 3: packed dump to [pattr]        Outlet 4: patch name display
// Outlet 5: status display
// Outlet 6: "set 0/1" to the MPE button (Z1 Buddy)
// Outlet 7: unison and pots of the Z1 Buddy to [pattr]

autowatch = 1;
inlets = 2;
outlets = 8;

var OUT_MIDI = 0, OUT_BANK = 1, OUT_PATCH = 2, OUT_STORE = 3, OUT_NAME = 4, OUT_STATUS = 5, OUT_MPE = 6, OUT_BUDDY = 7;
var STORE_MAGIC = 2501;      // first value of the list kept in [pattr]
var DIAL_DEBOUNCE_MS = 30;   // lets a bank and a patch change go out as one message group
var NAME_FETCH_MS = 1000;    // default wait after the last program change before asking for the name

// ---------------------------------------------------------------- helpers

// 8 MIDI bytes -> 7 data bytes. The first byte of each group carries the top
// bits of the following seven (bit 0 belongs to the first of them). Korg and
// Behringer both use this layout.
function unpack7(enc) {
    var out = [];
    for (var i = 0; i < enc.length; i += 8) {
        var flags = enc[i];
        for (var j = 1; j <= 7 && i + j < enc.length; j++) {
            var v = enc[i + j] & 0x7f;
            if (flags & (1 << (j - 1))) v |= 0x80;
            out.push(v);
        }
    }
    return out;
}

function asciiName(codes) {
    var s = "";
    for (var i = 0; i < codes.length; i++) {
        var c = codes[i];
        s += (c >= 0x20 && c < 0x7f) ? String.fromCharCode(c) : " ";
    }
    return s.replace(/\s+$/, "").replace(/^\s+/, "");
}

function isBehringer(m) {
    return m.length >= 12 && m[1] === 0x00 && m[2] === 0x20 && m[3] === 0x32;
}

// ---------------------------------------------------------------- synths

var SYNTHS = {
    // Behringer UB-Xa / UB-Xa D. Messages start F0 00 20 32 and carry a type
    // byte at index 10: 01 = header, 02 = data packet, 03 = request.
    // The patch format changed with firmware v2; the request and the name
    // lookup below are for v2. The dump is saved and sent back byte for byte,
    // so nothing else depends on the format, but a dump saved under one
    // firmware may not load in another.
    ubxa: {
        label: "UB-Xa",
        bankNames: ["A", "B", "C", "D"],
        pcOffset: 1,          // dial shows 1-128
        pcDelay: 100,         // ms between bank select and program change
        sysex: {
            // Request for the file "Current Program ", taken unchanged from the
            // original Patch Control UBXA device.
            request: function () {
                return [0xf0, 0x00, 0x20, 0x32, 0x00, 0x01, 0x3a, 0x7f, 0x74, 0x07, 0x03, 0x7f,
                    0x42, 0x49, 0x4e, 0x20,                                     // "BIN "
                    0x43, 0x75, 0x72, 0x72, 0x65, 0x6e, 0x74, 0x20,             // "Current "
                    0x50, 0x72, 0x6f, 0x67, 0x72, 0x61, 0x6d, 0x20,             // "Program "
                    0x00, 0x03, 0x7c, 0xf7];
            },
            // After our request any Behringer message starts the dump; without
            // a request (dump started on the synth) only a header packet does.
            starts: function (m, requested) {
                return isBehringer(m) && (requested || m[10] === 0x01);
            },
            accepts: function (m) { return true; },   // packets, EOF, anything in between
            // The UB-Xa closes every dump with F0 7E 00 7B 00 F7 (seen in all
            // dumps in patchsync-debug.log), so there is no need to wait.
            ends: function (m) { return m.length === 6 && m[1] === 0x7e && m[3] === 0x7b; },
            quietMs: 300,         // fallback: complete when nothing arrives for this long
            // The UB-Xa freezes while it sends a dump, so the name is only asked
            // for once the user has stopped stepping through patches.
            nameDelay: 1200,
            replyTimeout: 2500,
            gapMs: 30,            // pause between messages when sending the dump back
            // The old device selected D128 before sending the dump. Set to null
            // to send into whatever program is selected.
            sendSlot: { bank: 3, pc: 127 },
            sendSlotDelay: 2000,
            // Returns a reason when the dump is damaged, null when it looks whole.
            // Data packets must be numbered 0, 1, 2 ... and each must be as long
            // as its length byte says (length byte + 1 data bytes, then the
            // checksum). Some MIDI interfaces lose data in long dumps.
            check: function (msgs) {
                var expected = 0;
                for (var i = 0; i < msgs.length; i++) {
                    var m = msgs[i];
                    if (!isBehringer(m) || m[10] === 0x01) continue;
                    if (m[10] !== 0x02 || m[11] !== expected || m.length !== m[12] + 16) {
                        return "Dump incomplete";
                    }
                    expected++;
                }
                return expected > 0 ? null : "Dump incomplete";
            },
            // Patch name. Ported unchanged from ubxa-parsepatch.js, which is the
            // reference for this: join the payload of every data packet (type
            // 02, length in the byte after the packet number) and read the 16
            // characters from fixed positions in the still 7-bit-packed data.
            // These positions are for firmware v2.
            name: function (msgs) {
                var data = [];
                for (var i = 0; i < msgs.length; i++) {
                    var packet = msgs[i].slice(1, msgs[i].length - 1);   // without F0 and F7
                    if (packet[0] === 0x00 && packet[9] === 0x02) {
                        var length = packet[11];
                        data = data.concat(packet.slice(12, 12 + length));
                    }
                }
                if (data.length < 108) return null;
                var abits = data[96];
                var b0 = (data[88] >> 1) & 1;
                var b1 = (data[88] >> 3) & 1;
                var b2 = (data[88] >> 5) & 1;
                var b7 = (data[104] >> 1) & 1;
                return asciiName([
                    data[91] * 2 + b0, data[90],
                    data[93] * 2 + b1, data[92],
                    data[95] * 2 + b2, data[94],
                    data[98] * 2 + ((abits >> 0) & 1), data[97],
                    data[100] * 2 + ((abits >> 2) & 1), data[99],
                    data[102] * 2 + ((abits >> 4) & 1), data[101],
                    data[105] * 2 + ((abits >> 6) & 1), data[103],
                    data[107] * 2 + b7, data[106]
                ]);
            }
        }
    },

    // Korg Z1. Header F0 42 3g 46, g = global MIDI channel.
    // 10 = current program dump request, 40 = current program dump.
    // The Z1 needs "SysEx Receive" enabled. With "SysEx Transmit" on it also
    // sends the dump by itself after every program change.
    z1: {
        label: "Z1",
        bankNames: ["A", "B", "C", "D"],
        pcOffset: 0,
        pcDelay: 20,
        // The Z1 is played through the Z1 Buddy, which can turn it into an MPE
        // synth. For that it copies the current sound to a working slot, the
        // last program of bank A, and the Z1 plays that. The Buddy reports its
        // mode with short sysex messages of its own (see DEVELOPMENT.md there):
        //   F0 7D 5A 31 01 F7                  ask for the status
        //   F0 7D 5A 31 02 mm F7               go to mode 0 Normal / 1 MPE
        //   F0 7D 5A 31 03 uu F7               set unison 0-3
        //   F0 7D 5A 31 04 ww pt dt F7         set the pots: wonkiness, portamento
        //                                      time, detune (0-99)
        //   F0 7D 5A 31 11 mm uu ww bb pp pt dt F7
        //                                      status: mode (0 Normal, 1 entering
        //                                      MPE, 2 MPE, 3 MPE+), unison,
        //                                      wonkiness, bank and program the MPE
        //                                      sound was copied from, portamento
        //                                      time, detune
        buddy: { tempBank: 0, tempPc: 127 },
        sysex: {
            channel: 0,
            observe: function (m) {
                if (m.length >= 5 && m[1] === 0x42 && m[3] === 0x46 && (m[2] & 0xf0) === 0x30) {
                    this.channel = m[2] & 0x0f;
                }
            },
            request: function () {
                return [0xf0, 0x42, 0x30 | this.channel, 0x46, 0x10, 0x00, 0xf7];
            },
            starts: function (m, requested) {
                return m.length > 8 && m[1] === 0x42 && m[3] === 0x46 && m[4] === 0x40;
            },
            accepts: function (m) { return m[1] === 0x42 && m[3] === 0x46 && m[4] === 0x40; },
            quietMs: 60,
            nameDelay: 700,
            replyTimeout: 2000,
            gapMs: 30,
            sendSlot: null,       // the dump loads into the edit buffer
            sendSlotDelay: 0,
            name: function (msgs) {
                var m = msgs[msgs.length - 1];
                var dec = unpack7(m.slice(6, m.length - 1));
                if (dec.length < 16) return null;
                return asciiName(dec.slice(0, 16));
            }
        }
    },

    // Kurzweil PC3. No published sysex format, so bank and patch only.
    pc3: {
        label: "PC3",
        bankNames: ["A", "B", "C", "D", "E", "F", "G", "H", "I"],
        pcOffset: 0,
        pcDelay: 100,
        sysex: null
    }
};

var synthKey = (jsarguments.length > 1) ? String(jsarguments[1]).toLowerCase() : "ubxa";
var cfg = SYNTHS[synthKey];
if (!cfg) {
    post("patchsync: unknown synth '" + synthKey + "', using ubxa\n");
    cfg = SYNTHS.ubxa;
}

// ---------------------------------------------------------------- state

var bank = 0;            // what we believe the synth has selected
var pc = 0;              // raw program change number 0-127
var synced = false;      // true once bank/pc were sent to or received from the synth
var autoName = 1;
var currentName = "";
var saved = null;        // { bank, pc, name, msgs } stored in the Live set

var rx = [];             // sysex message being received
var inSysex = false;
var collecting = false;  // a dump is arriving
var collected = [];
var awaiting = false;    // a request was sent, nothing has arrived yet
var purpose = null;      // "name" or "store": what to do with the dump
var sending = false;     // the saved dump is being transmitted
var txIndex = 0;

var sendTask = new Task(sendBankAndPatch, this);
var pcTask = new Task(sendProgramChange, this);
var buddyMode = -1;      // Z1 Buddy main mode from its status; -1 = not heard from it
var fetchTask = new Task(fetchName, this);
var changeSeq = 0;       // counts program changes
var dumpSeq = 0;         // changeSeq when the dump now arriving was started
var quietTask = new Task(finishCollect, this);
var replyTask = new Task(replyTimedOut, this);
var txTask = new Task(transmitNext, this);

// ---------------------------------------------------------------- debug log

// While DEBUG is on, every dump received is appended as hex to
// patchsync-debug.log next to the device, and summarised in the Max window.
var DEBUG = true;

function hexOf(bytes) {
    var s = [];
    for (var i = 0; i < bytes.length; i++) s.push((bytes[i] < 16 ? "0" : "") + bytes[i].toString(16));
    return s.join(" ");
}

function logDump(why, msgs, name) {
    var lines = ["--- " + new Date().toString() + " | " + cfg.label + " | " + slotLabel(bank, pc) +
        " | purpose " + why + " | " + msgs.length + " messages | name [" + name + "]"];
    for (var i = 0; i < msgs.length; i++) lines.push(msgs[i].length + ": " + hexOf(msgs[i]));
    post("patchsync: " + lines[0] + "\n");
    try {
        var path = String(this.patcher.filepath);
        var cut = path.lastIndexOf("/");
        if (cut < 0) { post("patchsync: no device folder, log not written\n"); return; }
        var f = new File(path.substring(0, cut + 1) + "patchsync-debug.log", "readwrite", "TEXT");
        if (!f.isopen) f = new File(path.substring(0, cut + 1) + "patchsync-debug.log", "write", "TEXT");
        if (!f.isopen) { post("patchsync: could not open the log file\n"); return; }
        f.position = f.eof;
        for (var j = 0; j < lines.length; j++) f.writeline(lines[j]);
        f.close();
    } catch (e) {
        post("patchsync: log failed: " + e + "\n");
    }
}

// ---------------------------------------------------------------- Z1 Buddy

function inMPE() { return !!cfg.buddy && buddyMode >= 1; }

function isTempSlot(b, p) {
    return !!cfg.buddy && b === cfg.buddy.tempBank && p === cfg.buddy.tempPc;
}

function buddySend(bytes) {
    sendBytes([0xf0, 0x7d, 0x5a, 0x31].concat(bytes, [0xf7]));
}

function buddyAsk() {
    if (cfg.buddy) buddySend([0x01]);
}

// The MPE button on the device. It is a Live parameter, so it is saved with
// the set and comes back when the set is opened: the Buddy is then put in the
// mode the set was saved in. Entering MPE copies the current sound to the
// working slot, so it must wait until the patch of the set has been selected
// on the Z1: while the set is loading, and while a patch change is on its way.
var mpeWanted = false;
var loading = false;
var sendPending = false;     // a bank and program change has not gone out yet
var mpeTask = new Task(mpeGo, this);
var loadedTask = new Task(function () { loading = false; }, this);

function mpe(v) {
    if (!cfg.buddy) return;
    mpeTask.cancel();
    mpeWanted = false;
    if (v) {
        if (inMPE()) return;                   // already there, e.g. set on the Buddy itself
        mpeWanted = true;
        status("Entering MPE...");
        mpeTask.schedule(loading ? 1500 : 0);
    } else {
        if (buddyMode === 0) return;
        buddySend([0x02, 0]);
        status("Leaving MPE...");
    }
}

function mpeGo() {
    if (!mpeWanted) return;
    if (sendPending) { mpeTask.schedule(300); return; }
    mpeWanted = false;
    buddySend([0x02, 1]);
}

// ---- The Buddy's unison and pots, kept in the Live set.
// The Buddy reports them in its status, and they are stored whenever they
// change. When the set is opened they are sent back: the pots once the patch
// has been selected, unison once the Buddy is in the mode the set was saved
// in (unison means something else in MPE mode). A pot set this way holds
// until the pot on the Buddy is moved.
var BUDDY_MAGIC = 2504;
var buddySaved = null;       // { unison, wonk, porta, detune } from the set, not yet sent back
var buddyKept = "";          // what is stored now, to store only changes
var buddyTask = new Task(buddyRestoreGo, this);

function restorebuddy() {
    var a = arrayfromargs(arguments);
    if (!cfg.buddy || a.length < 5 || Math.round(a[0]) !== BUDDY_MAGIC) return;
    buddyKept = a.slice(1, 5).join(" ");
    if (!loading) return;                        // only a set being opened brings state back
    buddySaved = { unison: Math.round(a[1]), wonk: Math.round(a[2]), porta: Math.round(a[3]), detune: Math.round(a[4]) };
    buddyTask.schedule(1200);
}

function buddyRestoreGo() {
    if (!buddySaved) return;
    if (sendPending) { buddyTask.schedule(300); return; }
    buddySend([0x04, buddySaved.wonk & 0x7f, buddySaved.porta & 0x7f, buddySaved.detune & 0x7f]);
    buddyRestoreUnison();
}

// Unison goes back once the Buddy is in the mode the set wants.
function buddyRestoreUnison() {
    if (!buddySaved || buddyMode < 0 || buddyMode === 1) return;
    if (mpeWanted && buddyMode === 0) return;    // MPE is still to be entered
    buddySend([0x03, buddySaved.unison & 0x03]);
    buddySaved = null;
}

function buddyKeep(m) {
    if (m.length < 13 || loading || buddySaved || mpeWanted) return;   // do not store over what is being restored
    var now = [m[6], m[7], m[10], m[11]].join(" ");
    if (now === buddyKept) return;
    buddyKept = now;
    outlet(OUT_BUDDY, [BUDDY_MAGIC, m[6], m[7], m[10], m[11]]);
}

// A status message from the Buddy. Returns true if m was one.
function buddyStatus(m) {
    if (!cfg.buddy || m.length < 11 || m[1] !== 0x7d || m[2] !== 0x5a || m[3] !== 0x31 || m[4] !== 0x11) return false;
    var was = buddyMode;
    buddyMode = m[5];
    buddyRestoreUnison();
    buddyKeep(m);
    // The button follows the Buddy, unless we are about to ask for MPE ourselves.
    if (!(mpeWanted && buddyMode === 0)) outlet(OUT_MPE, "set", buddyMode >= 1 ? 1 : 0);
    if (buddyMode >= 1) {
        // The Z1 now plays the working slot. The dials and the name stay on the
        // sound it was made from, which the Buddy tells us.
        sendTask.cancel();
        pcTask.cancel();
        fetchTask.cancel();
        if (m[8] < cfg.bankNames.length) {
            if (m[8] !== bank || m[9] !== pc) currentName = "";   // not the sound we had the name of
            bank = m[8];
            pc = m[9];
            synced = true;
            showDials();
        }
        showName();
        status(buddyMode === 1 ? "Entering MPE..." : "MPE: " + slotLabel(bank, pc) + " in A " + cfg.buddy.tempPc);
    } else if (was >= 1) {
        showName();
        status("Normal mode");
        scheduleNameFetch();
    }
    return true;
}

function showName() {
    show(OUT_NAME, (inMPE() ? "MPE: " : "") + (currentName || (inMPE() ? "(name not read)" : "")));
}

// ---------------------------------------------------------------- display

function show(out, text) {
    // One symbol with non-breaking spaces, so the message box shows it without
    // quotes and does not turn "01" into 1.
    outlet(out, "set", text === "" ? "-" : String(text).replace(/ /g, " "));
}

function status(text) { show(OUT_STATUS, text); }

function slotLabel(b, p) {
    var names = cfg.bankNames;
    return (b < names.length ? names[b] : "?") + " " + (p + cfg.pcOffset);
}

function savedStatus() {
    if (!cfg.sysex) return "Ready";
    return saved ? "Saved: " + (saved.name || "(no name)") : "Nothing saved";
}

function showDials() {
    if (bank >= 0 && bank < cfg.bankNames.length) outlet(OUT_BANK, "set", bank);
    outlet(OUT_PATCH, "set", pc + cfg.pcOffset);
}

function loadbang() {
    showName();
    status(savedStatus());
    loading = true;
    loadedTask.schedule(3000);
    buddyAsk();
}

// ---------------------------------------------------------------- MIDI out

function sendBytes(bytes) {
    for (var i = 0; i < bytes.length; i++) outlet(OUT_MIDI, bytes[i]);
}

function sendBankAndPatch() {
    sendBytes([0xb0, 0, 0, 0xb0, 32, bank & 0x7f]);
    pcTask.cancel();
    pcTask.schedule(cfg.pcDelay);
}

function sendProgramChange() {
    sendBytes([0xc0, pc & 0x7f]);
    synced = true;
    sendPending = false;
    if (sending) return;
    status("Sent " + slotLabel(bank, pc) + (isTempSlot(bank, pc) ? " (MPE working slot)" : ""));
    scheduleNameFetch();
}

function scheduleSend() {
    sendPending = true;
    sendTask.cancel();
    pcTask.cancel();
    fetchTask.cancel();
    sendTask.schedule(DIAL_DEBOUNCE_MS);
}

// ---------------------------------------------------------------- dials

// In MPE mode the Z1 plays the working slot, and a program change would pull
// it off that. The dial goes back and nothing is sent.
function lockedInMPE() {
    if (!inMPE()) return false;
    showDials();
    status("MPE mode: leave it to change patch");
    return true;
}

function dialbank(v) {
    v = Math.round(v);
    if (sending || (v === bank && synced)) return;
    if (lockedInMPE()) return;
    bank = v;
    scheduleSend();
}

function dialpatch(v) {
    v = Math.round(v) - cfg.pcOffset;
    if (v < 0) v = 0;
    if (v > 127) v = 127;
    if (sending || (v === pc && synced)) return;
    if (lockedInMPE()) return;
    pc = v;
    scheduleSend();
}

function resend() {
    if (sending) return;
    buddyAsk();
    if (lockedInMPE()) return;
    scheduleSend();
}

function auto(v) {
    autoName = v ? 1 : 0;
    if (!autoName) fetchTask.cancel();
}

// ---------------------------------------------------------------- from the synth

// The synth is the authority here: its changes only update the dials and are
// never sent back to it.

function bankmsb(v) { }

function banklsb(v) {
    if (inMPE()) return;                   // belongs to the MPE setup, not to a patch choice
    bank = Math.round(v) & 0x7f;
    if (bank < cfg.bankNames.length) outlet(OUT_BANK, "set", bank);
}

function pgm(v) {
    if (inMPE()) return;
    pc = Math.round(v) & 0x7f;
    synced = true;
    // A pending send would pull the synth back to an older patch.
    sendTask.cancel();
    pcTask.cancel();
    outlet(OUT_PATCH, "set", pc + cfg.pcOffset);
    if (sending) return;
    status("Synth: " + slotLabel(bank, pc) + (isTempSlot(bank, pc) ? " (MPE working slot)" : ""));
    scheduleNameFetch();
}

// ---------------------------------------------------------------- sysex in

function msg_int(v) {
    if (inlet !== 1) return;
    if (v >= 0xf8) return;                 // realtime bytes may appear anywhere
    if (v === 0xf0) { rx = [0xf0]; inSysex = true; return; }
    if (!inSysex) return;
    rx.push(v);
    if (v === 0xf7) {
        inSysex = false;
        var m = rx;
        rx = [];
        onSysex(m);
    } else if (v >= 0x80 || rx.length > 100000) {
        inSysex = false;                   // broken message
        rx = [];
    }
}

// A whole sysex message as one list. Bytes sent one by one from the MIDI
// thread can reach [js] out of order, so the patcher joins them first.
function list() {
    if (inlet !== 1) return;
    var m = [];
    for (var i = 0; i < arguments.length; i++) {
        var v = Math.round(arguments[i]);
        if (v < 0xf8) m.push(v);
    }
    if (m.length < 2 || m[0] !== 0xf0 || m[m.length - 1] !== 0xf7) return;
    onSysex(m);
}

function onSysex(m) {
    var sx = cfg.sysex;
    if (!sx) return;
    if (buddyStatus(m)) return;
    if (sx.observe) sx.observe(m);
    if (sending) return;                   // replies to our own dump
    if (!collecting) {
        if (!sx.starts(m, awaiting)) return;
        collecting = true;
        collected = [];
        if (!awaiting) dumpSeq = changeSeq;    // a dump the synth sent by itself
        awaiting = false;
        replyTask.cancel();
    } else if (!sx.accepts(m)) {
        return;
    }
    collected.push(m);
    quietTask.cancel();
    if (sx.ends && sx.ends(m)) finishCollect();
    else quietTask.schedule(sx.quietMs);
}

function finishCollect() {
    var sx = cfg.sysex;
    var msgs = collected;
    var why = purpose || "name";
    collecting = false;
    collected = [];
    purpose = null;

    var problem = sx.check ? sx.check(msgs) : null;
    var name = problem ? null : sx.name(msgs);
    if (DEBUG) logDump(why + (problem ? " | " + problem : ""), msgs, name);
    if (dumpSeq !== changeSeq) {
        // The program changed while this dump was on its way, so it belongs
        // to a patch that is no longer selected. The name request scheduled
        // by that program change reads the right one.
        if (why === "store") status("Patch changed, try again");
        return;
    }
    if (!problem && name !== null) fetchTask.cancel();   // name is here, no need to ask
    if (problem) {
        // Never keep or show a damaged dump: sending it back could corrupt
        // the sound on the synth.
        currentName = "";
        showName();
        status(problem);
        return;
    }
    if (name !== null) {
        currentName = name;
        showName();
    }
    if (why === "store") {
        saved = { bank: bank, pc: pc, name: name === null ? "" : name, msgs: msgs };
        outlet(OUT_STORE, packSaved(saved));
        status(savedStatus());
    } else if (name === null) {
        status("Could not read name");
    }
}

function replyTimedOut() {
    if (collecting) return;
    awaiting = false;
    purpose = null;
    status("No reply from synth");
}

function request(why) {
    var sx = cfg.sysex;
    if (!sx || sending) return;
    if (why === "store") purpose = "store";
    else if (!purpose) purpose = "name";
    if (collecting || awaiting) return;    // the dump on its way will do
    awaiting = true;
    dumpSeq = changeSeq;
    sendBytes(sx.request());
    replyTask.cancel();
    replyTask.schedule(sx.replyTimeout);
}

function nameDelay() {
    return (cfg.sysex && cfg.sysex.nameDelay) || NAME_FETCH_MS;
}

// Called for every program change. The request goes out only after the
// program has stayed the same for nameDelay(); each new change starts the
// wait again, so stepping through patches sends no requests at all.
function scheduleNameFetch() {
    changeSeq++;
    fetchTask.cancel();
    if (!cfg.sysex || inMPE()) return;
    currentName = "";
    show(OUT_NAME, autoName ? "..." : "");
    if (autoName) fetchTask.schedule(nameDelay());
}

function fetchName() {
    if (inMPE()) return;
    if (collecting || awaiting) {
        // A dump for an earlier patch is still arriving: ask again after it.
        fetchTask.schedule(nameDelay());
        return;
    }
    request("name");
}

function getsysex() {
    if (!cfg.sysex) return;
    if (sending) { status("Busy sending"); return; }
    if (inMPE()) { status("MPE mode: leave it to save"); return; }
    fetchTask.cancel();
    status("Reading...");
    request("store");
}

// ---------------------------------------------------------------- sysex out

function sendsysex() {
    var sx = cfg.sysex;
    if (!sx || sending) return;
    if (inMPE()) { status("MPE mode: leave it to send"); return; }
    if (!saved || !saved.msgs.length) { status("Nothing saved"); return; }
    sendTask.cancel();
    pcTask.cancel();
    fetchTask.cancel();
    sending = true;
    txIndex = 0;
    status("Sending...");
    if (sx.sendSlot) {
        bank = sx.sendSlot.bank;
        pc = sx.sendSlot.pc;
        showDials();
        sendBankAndPatch();
        txTask.schedule(sx.sendSlotDelay);
    } else {
        txTask.schedule(0);
    }
}

function transmitNext() {
    if (!sending || !saved) { sending = false; return; }
    if (txIndex < saved.msgs.length) {
        sendBytes(saved.msgs[txIndex]);
        txIndex++;
    }
    if (txIndex < saved.msgs.length) {
        txTask.schedule(cfg.sysex.gapMs);
    } else {
        sending = false;
        currentName = saved.name;
        showName();
        status("Sent: " + (saved.name || "(no name)"));
    }
}

// ---------------------------------------------------------------- storage

// [pattr] list: magic, bank, pc, byte count, then the dump with three bytes
// packed into each number to keep the list short.
function packSaved(s) {
    var bytes = [];
    for (var i = 0; i < s.msgs.length; i++) bytes = bytes.concat(s.msgs[i]);
    var list = [STORE_MAGIC, s.bank, s.pc, bytes.length];
    for (var j = 0; j < bytes.length; j += 3) {
        list.push((bytes[j] << 16) | ((bytes[j + 1] || 0) << 8) | (bytes[j + 2] || 0));
    }
    return list;
}

function restore() {
    var a = arrayfromargs(arguments);
    if (!cfg.sysex) return;
    if (a.length < 5 || Math.round(a[0]) !== STORE_MAGIC) {
        saved = null;
        status(savedStatus());
        return;
    }
    var count = Math.round(a[3]);
    var bytes = [];
    for (var i = 4; i < a.length; i++) {
        var v = Math.round(a[i]);
        bytes.push((v >> 16) & 0xff, (v >> 8) & 0xff, v & 0xff);
    }
    bytes = bytes.slice(0, count);
    var msgs = [];
    var cur = null;
    for (var j = 0; j < bytes.length; j++) {
        if (bytes[j] === 0xf0) cur = [];
        if (cur) cur.push(bytes[j]);
        if (bytes[j] === 0xf7 && cur) { msgs.push(cur); cur = null; }
    }
    var name = msgs.length ? cfg.sysex.name(msgs) : null;
    saved = { bank: Math.round(a[1]), pc: Math.round(a[2]), name: name === null ? "" : name, msgs: msgs };
    if (!collecting && !awaiting && !sending) status(savedStatus());
}
