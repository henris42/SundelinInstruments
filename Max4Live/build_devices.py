#!/usr/bin/env python3
"""Generate the "Patch Sync <synth>.amxd" Max for Live MIDI devices.

All logic lives in patchsync.js; the patcher is only wiring and UI, so the
three devices are built from one description here. Run:

    python3 build_devices.py

An .amxd is a 32-byte header followed by the patcher JSON. If you edit a
device in Max and save it there, do not run this script again for that device
or your changes are overwritten.
"""
import json
import os
import struct

HERE = os.path.dirname(os.path.abspath(__file__))
WIDTH = 160.0
HEIGHT = 169.0

SYNTHS = [
    # key, title, bank names, patch dial min/max, has sysex, send button text,
    # extra attributes for midiparse/midiformat
    ("ubxa", "UB-Xa", ["A", "B", "C", "D"], 1, 128, True, "Push to D128", " @hires 2"),
    ("z1", "Z1", ["A", "B", "C", "D"], 0, 127, True, "Send Saved", " @hires 2"),
    ("pc3", "PC3", ["A", "B", "C", "D", "E", "F", "G", "H", "I"], 0, 127, False, "", " @hires 2"),
]

DIAL_COLORS = {
    "activeneedlecolor": [0.239216, 0.254902, 0.278431, 1.0],
    "dialcolor": [0.32549, 0.345098, 0.372549, 1.0],
    "focusbordercolor": [0.960784, 0.827451, 0.156863, 1.0],
}


class Patch:
    def __init__(self):
        self.boxes = []
        self.lines = []
        self.params = {}
        self.n = 0

    def add(self, maxclass, rect, nin, nout, outtypes=None, pres=None, **attrs):
        self.n += 1
        oid = "obj-%d" % self.n
        box = {
            "id": oid,
            "maxclass": maxclass,
            "numinlets": nin,
            "numoutlets": nout,
            "patching_rect": [float(v) for v in rect],
        }
        if nout:
            box["outlettype"] = outtypes or [""] * nout
        if pres:
            box["presentation"] = 1
            box["presentation_rect"] = [float(v) for v in pres]
        box.update(attrs)
        self.boxes.append({"box": box})
        return oid

    def obj(self, text, x, y, nin, nout, outtypes=None, **attrs):
        width = max(40, 7 * len(text) + 12)
        return self.add("newobj", [x, y, width, 20], nin, nout, outtypes, text=text, **attrs)

    def msg(self, text, x, y, pres=None, **attrs):
        width = max(32, 7 * len(text) + 12)
        return self.add("message", [x, y, width, 20], 2, 1, pres=pres, text=text, **attrs)

    def param(self, oid, longname, shortname):
        self.params[oid] = [longname, shortname, 0]

    def dial(self, x, y, pres, varname, longname, shortname, valueof, annotation):
        valueof = dict(valueof, parameter_longname=longname, parameter_shortname=shortname,
                       parameter_modmode=0)
        oid = self.add(
            "live.dial", [x, y, 45, 50], 1, 2, ["", "float"], pres=pres,
            parameter_enable=1, varname=varname, annotation=annotation, fontsize=10.0,
            saved_attribute_attributes=dict(
                {k: {"expression": ""} for k in DIAL_COLORS}, valueof=valueof),
            **DIAL_COLORS)
        self.param(oid, longname, shortname)
        return oid

    def button(self, x, y, pres, varname, longname, text, toggle=False, annotation=""):
        valueof = {
            "parameter_enum": ["Off", "On"] if toggle else ["val1", "val2"],
            "parameter_longname": longname,
            "parameter_shortname": longname,
            "parameter_mmax": 1,
            "parameter_modmode": 0,
            "parameter_type": 2,
        }
        if toggle:
            valueof["parameter_initial_enable"] = 1
            valueof["parameter_initial"] = [1.0]
        oid = self.add(
            "live.text", [x, y, 48, 15], 1, 2, ["", ""], pres=pres,
            mode=1 if toggle else 0, parameter_enable=1, varname=varname,
            text=text, texton=text, annotation=annotation,
            saved_attribute_attributes={"valueof": valueof})
        self.param(oid, longname, longname)
        return oid

    def wire(self, src, outlet, dst, inlet=0):
        self.lines.append({"patchline": {"source": [src, outlet], "destination": [dst, inlet]}})


def build(key, title, banks, pmin, pmax, has_sysex, send_text, midi_attrs, out_dir=None):
    p = Patch()

    # ---- MIDI thru. Everything passes except bank select and program change,
    # which go to the js so they are never echoed back to the synth.
    midiin = p.obj("midiin", 30, 30, 1, 1, ["int"])
    parse = p.obj("midiparse" + midi_attrs, 30, 70, 1, 8, ["", "", "", "int", "int", "", "int", ""])
    route = p.obj("route 0 32", 150, 110, 3, 3)
    fmt = p.obj("midiformat" + midi_attrs, 30, 470, 7, 2, ["int", ""])
    midiout = p.obj("midiout", 30, 520, 1, 0)
    p.wire(midiin, 0, parse)
    for outlet in (0, 1, 4, 5, 6):
        p.wire(parse, outlet, fmt, outlet)
    p.wire(parse, 2, route)
    p.wire(route, 2, fmt, 2)
    p.wire(fmt, 0, midiout)

    js = p.obj("js patchsync.js %s" % key, 300, 300, 2, 8,
               saved_object_attributes={"filename": "patchsync.js", "parameter_enable": 0})
    p.wire(js, 0, midiout)

    pre_msb = p.obj("prepend bankmsb", 150, 150, 1, 1)
    pre_lsb = p.obj("prepend banklsb", 270, 150, 1, 1)
    pre_pgm = p.obj("prepend pgm", 390, 150, 1, 1)
    p.wire(route, 0, pre_msb)
    p.wire(route, 1, pre_lsb)
    p.wire(parse, 3, pre_pgm)
    # [js] defers what it receives from the MIDI thread, and a burst deferred
    # that way can arrive out of order. [deferlow] queues in order, and each
    # sysex message is joined into one list first, as the old device did.
    defer_midi = p.obj("deferlow", 270, 200, 1, 1)
    for src in (pre_msb, pre_lsb, pre_pgm):
        p.wire(src, 0, defer_midi)
    p.wire(defer_midi, 0, js)

    sysexin = p.obj("sysexin", 600, 130, 1, 1, ["int"])
    split = p.obj("t i i", 600, 160, 1, 2, ["int", "int"])
    endmark = p.obj("sel 247", 600, 190, 2, 2, ["bang", ""])
    group = p.obj("zl.group 8192 @zlmaxsize 8192", 660, 220, 2, 2)
    defer_sysex = p.obj("deferlow", 660, 250, 1, 1)
    p.wire(sysexin, 0, split)
    p.wire(split, 1, group)          # right outlet first: add the byte
    p.wire(split, 0, endmark)        # then F7 sends the whole message on
    p.wire(endmark, 0, group)
    p.wire(group, 0, defer_sysex)
    p.wire(defer_sysex, 0, js, 1)

    # ---- dials
    bank = p.dial(800, 30, [2, 4, 52, 50], "bankdial", "Bank Number", "Bank", {
        "parameter_enum": banks, "parameter_mmax": len(banks) - 1,
        "parameter_type": 2, "parameter_unitstyle": 9, "parameter_mapping_index": 1,
    }, "Bank selected on the synth.")
    patch = p.dial(920, 30, [54, 4, 52, 50], "patchdial", "Patch Number", "Patch", {
        "parameter_mmin": float(pmin), "parameter_mmax": float(pmax),
        "parameter_type": 1, "parameter_unitstyle": 0, "parameter_mapping_index": 2,
    }, "Patch selected on the synth.")
    pre_bank = p.obj("prepend dialbank", 800, 100, 1, 1)
    pre_patch = p.obj("prepend dialpatch", 920, 100, 1, 1)
    p.wire(bank, 0, pre_bank)
    p.wire(patch, 0, pre_patch)
    p.wire(pre_bank, 0, js)
    p.wire(pre_patch, 0, js)
    p.wire(js, 1, bank)
    p.wire(js, 2, patch)

    # When the device has loaded, make both dials report their restored values.
    thisdev = p.obj("live.thisdevice", 1040, 30, 1, 3, ["bang", "int", "int"])
    p.wire(thisdev, 0, bank)
    p.wire(thisdev, 0, patch)

    # ---- buttons
    resend = p.button(480, 190, [110, 8, 46, 15], "resendbtn", "Resend", "Resend",
                      annotation="Send the bank and patch to the synth again.")
    m_resend = p.msg("resend", 480, 215)
    p.wire(resend, 0, m_resend)
    p.wire(m_resend, 0, js)

    if key == "z1":
        # The Z1 is played through the Z1 Buddy, which can run it as an MPE
        # synth. This switches the Buddy's mode and follows it.
        # A Live parameter, so the mode is saved with the set. The js waits
        # with entering MPE until the set's patch has been selected.
        mpe = p.button(1040, 190, [110, 46, 46, 15], "mpebtn", "MPE Mode", "MPE", toggle=True,
                       annotation="Z1 Buddy: enter or leave MPE mode. Saved with the Live set; "
                                  "follows the mode set on the Buddy.")
        for box in p.boxes:
            if box["box"]["id"] == mpe:
                box["box"]["saved_attribute_attributes"]["valueof"]["parameter_initial"] = [0.0]
        # The Buddy's unison and pots are kept in a hidden parameter, like
        # the saved dump, so they come back with the set.
        bstore = p.obj(
            "pattr buddystore", 1040, 300, 1, 3, varname="buddystore",
            saved_object_attributes={"parameter_enable": 1, "parameter_mappable": 0},
            saved_attribute_attributes={"valueof": {
                "parameter_invisible": 1, "parameter_longname": "Buddy State",
                "parameter_shortname": "Buddy State", "parameter_modmode": 0,
                "parameter_type": 3}})
        p.param(bstore, "Buddy State", "Buddy State")
        pre_bstore = p.obj("prepend restorebuddy", 1040, 340, 1, 1)
        p.wire(js, 7, bstore)
        p.wire(bstore, 0, pre_bstore)
        p.wire(pre_bstore, 0, js)

        pre_mpe = p.obj("prepend mpe", 1040, 215, 1, 1)
        p.wire(mpe, 0, pre_mpe)
        p.wire(pre_mpe, 0, js)
        p.wire(js, 6, mpe)

    status_y = 62
    if has_sysex:
        auto = p.button(560, 190, [110, 30, 46, 15], "autobtn", "Auto Name", "Auto", toggle=True,
                        annotation="Read the patch name from the synth after every program change.")
        pre_auto = p.obj("prepend auto", 560, 215, 1, 1)
        p.wire(auto, 0, pre_auto)
        p.wire(pre_auto, 0, js)

        get = p.button(660, 190, [4, 108, 74, 15], "getbtn", "Get Sysex", "Get + Save",
                       annotation="Read the current sound from the synth and save it in the Live set.")
        m_get = p.msg("getsysex", 660, 215)
        p.wire(get, 0, m_get)
        p.wire(m_get, 0, js)

        send = p.button(760, 190, [82, 108, 74, 15], "sendbtn", "Send Sysex", send_text,
                        annotation="Send the sound saved in the Live set to the synth.")
        m_send = p.msg("sendsysex", 760, 215)
        p.wire(send, 0, m_send)
        p.wire(m_send, 0, js)

        # The dump is kept in a hidden blob parameter, so it is saved with the set.
        store = p.obj(
            "pattr sysexstore", 420, 400, 1, 3, varname="sysexstore",
            saved_object_attributes={"parameter_enable": 1, "parameter_mappable": 0},
            saved_attribute_attributes={"valueof": {
                "parameter_invisible": 1, "parameter_longname": "Sysex Store",
                "parameter_shortname": "Sysex Store", "parameter_modmode": 0,
                "parameter_type": 3}})
        p.param(store, "Sysex Store", "Sysex Store")
        pre_restore = p.obj("prepend restore", 420, 440, 1, 1)
        p.wire(js, 3, store)
        p.wire(store, 0, pre_restore)
        p.wire(pre_restore, 0, js)

        name = p.msg("-", 520, 400, pres=[4, 62, 152, 20], ignoreclick=1)
        p.wire(js, 4, name)
        status_y = 84

    status = p.msg("-", 620, 400, pres=[4, status_y, 152, 20], ignoreclick=1)
    p.wire(js, 5, status)

    p.add("live.comment", [1040, 100, 150, 30], 1, 0, pres=[0, 134, WIDTH, 30],
          text="%s\nPatch Sync" % title, textjustification=1, fontsize=10.0, linecount=2,
          presentation_linecount=2)

    # MPE is enabled so that per-note bend, slide and pressure pass through
    # when this device shares a track with an MPE controller. A Max device
    # without it gets, and passes on, everything flattened to one channel.
    write_device(p, os.path.join(out_dir or HERE, "Patch Sync %s.amxd" % title.replace("-", "").upper()),
                 "patchsync.js", ["Bank Number", "Patch Number"], is_mpe=1)


def write_device(p, path, js_name, bank_params, width=WIDTH, height=HEIGHT, is_mpe=0):
    """Write the patch p as a Max for Live MIDI effect. bank_params are the
    parameters shown on a control surface; is_mpe=1 makes Live hand the device
    MPE (one note per channel)."""
    params = dict(p.params)
    params["parameterbanks"] = {"0": {"index": 0, "name": "", "parameters": (
        list(bank_params) + ["-"] * 8)[:8]}}
    params["inherited_shortname"] = 1

    patcher = {
        "fileversion": 1,
        "appversion": {"major": 8, "minor": 6, "revision": 4, "architecture": "x64", "modernui": 1},
        "classnamespace": "box",
        "rect": [60.0, 100.0, 1240.0, 640.0],
        "openrect": [0.0, 0.0, width, height],
        "bglocked": 0,
        "openinpresentation": 1,
        "default_fontsize": 10.0,
        "default_fontface": 0,
        "default_fontname": "Arial Bold",
        "gridonopen": 1,
        "gridsize": [8.0, 8.0],
        "gridsnaponopen": 1,
        "objectsnaponopen": 1,
        "statusbarvisible": 2,
        "toolbarvisible": 1,
        "boxanimatetime": 500,
        "enablehscroll": 1,
        "enablevscroll": 1,
        "devicewidth": width,
        "description": "",
        "digest": "",
        "tags": "",
        "style": "",
        "subpatcher_template": "",
        "assistshowspatchername": 0,
        "boxes": p.boxes,
        "lines": p.lines,
        "parameters": params,
        "dependency_cache": [{"name": js_name, "bootpath": HERE.replace(
            os.path.expanduser("~"), "~"), "type": "TEXT", "implicit": 1}],
        "latency": 0,
        "is_mpe": is_mpe,
        "minimum_live_version": "",
        "minimum_max_version": "",
        "platform_compatibility": 0,
        "project": {
            "version": 1, "creationdate": 3590052786, "modificationdate": 3590052786,
            "viewrect": [25.0, 106.0, 300.0, 500.0], "autoorganize": 1, "hideprojectwindow": 1,
            "showdependencies": 1, "autolocalize": 0,
            "contents": {"patchers": {}, "code": {}}, "layout": {}, "searchpath": {},
            "detailsvisible": 0, "amxdtype": 1835887981, "readonly": 0, "devpathtype": 0,
            "devpath": ".", "sortmode": 0, "viewmode": 0, "includepackages": 0},
        "autosave": 0,
    }

    payload = json.dumps({"patcher": patcher}, indent="\t").encode("utf-8") + b"\n\x00"
    # 'ampf' chunk, device type 'mmmm' (MIDI effect), 'meta' chunk, 'ptch' chunk
    header = (b"ampf" + struct.pack("<I", 4) + b"mmmm" + b"meta" + struct.pack("<II", 4, 0)
              + b"ptch" + struct.pack("<I", len(payload)))
    with open(path, "wb") as f:
        f.write(header + payload)
    print("wrote", path, len(header) + len(payload), "bytes,", len(p.boxes), "objects")


if __name__ == "__main__":
    for synth in SYNTHS:
        build(*synth)
