#!/usr/bin/env python3
"""Generate "MPE Remap.amxd", a Max for Live MIDI effect that remaps the MPE
dimensions of a controller (bend, slide, pressure) to other MIDI messages.

The mapping logic is in mperemap.js; this script only builds the wiring and
the controls. Run:

    python3 build_mperemap.py

If you edit the device in Max and save it there, do not run this script again
or your changes are overwritten.
"""
import os

from build_devices import HERE, Patch, write_device

WIDTH = 392.0
# key, name used in the parameter names, label on the device
SOURCES = [("bend", "Bend", "Bend"), ("slide", "Slide", "CC74"), ("press", "Press", "Press")]
# Same order as TARGETS in mperemap.js.
TARGET_LABELS = ["Off", "Note Bend", "Note Slide", "Note Press", "Poly AT", "Chan AT", "Pitch Bend", "CC"]
# Everything starts Off: a fresh device sends nothing until a target is chosen.
DEFAULT_TARGET = {"bend": 0, "slide": 0, "press": 0}
DEFAULT_CC = {"bend": 1, "slide": 74, "press": 11}


# Shown in a window of its own by the "?" button. (heading, text) pairs.
HELP = [
    ("MPE Remap", "Sends the three per-note movements of an MPE controller to other MIDI "
     "messages. Made for the Osmose, which has no presets when it is used as a controller: "
     "the mapping lives here instead, per Live set and as presets."),
    ("Before it can work", "1. In Live's MIDI settings, switch MPE on for the controller's input "
     "port. Without it everything arrives on channel 1 and notes cannot be told apart.  "
     "2. Any other Max device on the track must also have MPE enabled (the Patch Sync devices "
     "do), or it flattens everything to one channel.  "
     "3. For the Note Bend, Note Slide and Note Press targets the track's output and the synth "
     "must both be in MPE mode. On the UB-Xa: MIDI/USB menu, MPE Profile."),
    ("The rows", "Bend: sideways movement (per-note pitch bend).  CC74: position along the key.  "
     "Press: pressure (channel aftertouch per note)."),
    ("Target", "Off: nothing is sent.  Note Bend, Note Slide, Note Press: stays per note (MPE).  "
     "Poly AT: polyphonic aftertouch for that key.  Chan AT, Pitch Bend, CC: one value for all "
     "notes on channel 1 - the highest among the held notes, or for Bend the newest held note."),
    ("CC", "The controller used when the target is CC. Pick it by number or by name. The names "
     "come from the Synth menu; controllers the synth does not list show as a plain number."),
    ("Min, Max, Curve", "Min and Max are in percent of the target's range: the value runs from "
     "Min at rest to Max at full travel. Min above Max turns the direction around. Bend moves "
     "both ways around its centre, so only Max applies there, as its depth.  Curve is an "
     "exponent: 0 is straight, above 0 lifts small values, below 0 holds them back."),
    ("Bend range", "How many semitones of bend count as full scale. For Pitch Bend, set it to the "
     "synth's own pitch bend range. For CC or aftertouch, it is the sideways movement that gives "
     "a full swing. 48 passes an MPE bend unchanged."),
    ("Synth", "Only chooses which controller names are shown; it does not change what is sent. "
     "The names are in mperemap-synths.json next to the device and can be edited."),
    ("Presets", "Pick one from the menu to load it. To save: type a name and press Save, or press "
     "Save with the box empty to overwrite the current preset. Delete removes the current "
     "preset. Presets are kept in mperemap-presets.json next to the device, shared by all Live "
     "sets. The controls themselves are also saved with each Live set."),
    ("What it never does", "It never sends a value that was not played, and it never returns a "
     "controller to 0: when a controller is no longer fed, or the last note ends, it stays where "
     "it was. Only aftertouch and pitch bend go back to rest."),
    ("Log", "Writes the last 600 messages in and out, the mapping and the state of the files to "
     "mperemap-debug.log next to the device. Useful when something behaves strangely."),
]


def help_patcher():
    """The help text as a subpatcher: one comment box per heading and paragraph."""
    boxes = []
    width, y, n = 520, 12, 0
    for heading, text in HELP:
        for content, bold in ((heading, True), (text, False)):
            n += 1
            lines = 1 if bold else max(1, -(-len(content) // 84))      # about 84 characters a line
            height = 20 if bold else 6 + 14 * lines
            boxes.append({"box": {
                "id": "obj-%d" % n, "maxclass": "comment", "numinlets": 1, "numoutlets": 0,
                "patching_rect": [12.0, float(y), float(width), float(height)],
                "fontsize": 13.0 if bold else 11.0, "fontface": 1 if bold else 0,
                "linecount": lines, "text": content}})
            y += height + (2 if bold else 10)
    # [pcontrol] needs an inlet to reach this patcher; it carries nothing.
    boxes.append({"box": {
        "id": "obj-%d" % (n + 1), "maxclass": "inlet", "numinlets": 0, "numoutlets": 1,
        "outlettype": [""], "comment": "open / close from pcontrol",
        "patching_rect": [float(width - 20), float(y), 25.0, 25.0], "hidden": 1}})
    y += 30
    return {
        "fileversion": 1,
        "appversion": {"major": 8, "minor": 6, "revision": 4, "architecture": "x64", "modernui": 1},
        "classnamespace": "box",
        "rect": [120.0, 120.0, float(width + 40), float(min(y + 20, 820))],
        "bglocked": 1, "openinpresentation": 0, "default_fontsize": 11.0, "default_fontface": 0,
        "default_fontname": "Arial", "gridonopen": 1, "gridsize": [8.0, 8.0],
        "toolbarvisible": 0, "statusbarvisible": 0, "enablehscroll": 0, "enablevscroll": 1,
        "boxes": boxes, "lines": [],
    }


def comment(p, text, pres, x=1100, y=30):
    return p.add("live.comment", [x, y, 150, 18], 1, 0, pres=pres, text=text, fontsize=9.5)


def menu(p, x, y, pres, varname, longname, shortname, items, initial, annotation):
    oid = p.add(
        "live.menu", [x, y, 80, 15], 1, 3, ["", "", "float"], pres=pres,
        parameter_enable=1, varname=varname, annotation=annotation,
        saved_attribute_attributes={"valueof": {
            "parameter_longname": longname, "parameter_shortname": shortname,
            "parameter_type": 2, "parameter_enum": items, "parameter_modmode": 0,
            "parameter_initial_enable": 1, "parameter_initial": [float(initial)]}})
    p.param(oid, longname, shortname)
    return oid


def numbox(p, x, y, pres, varname, longname, shortname, lo, hi, initial, annotation,
           is_float=False, unitstyle=0):
    oid = p.add(
        "live.numbox", [x, y, 40, 15], 1, 2, ["", "float"], pres=pres,
        parameter_enable=1, varname=varname, annotation=annotation,
        saved_attribute_attributes={"valueof": {
            "parameter_longname": longname, "parameter_shortname": shortname,
            "parameter_type": 0 if is_float else 1, "parameter_unitstyle": unitstyle,
            "parameter_mmin": float(lo), "parameter_mmax": float(hi), "parameter_modmode": 0,
            "parameter_initial_enable": 1, "parameter_initial": [initial]}})
    p.param(oid, longname, shortname)
    return oid


def build():
    p = Patch()

    # ---- MIDI. Live hands an MPE device one note per channel. Bend, slide
    # (CC 74) and pressure go to the js; everything else passes straight
    # through, with its channel.
    midiin = p.obj("midiin", 30, 30, 1, 1, ["int"])
    parse = p.obj("midiparse @hires 2", 30, 70, 1, 8, ["", "", "", "int", "int", "", "int", ""])
    fmt = p.obj("midiformat @hires 2", 30, 470, 7, 2, ["int", ""])
    midiout = p.obj("midiout", 30, 520, 1, 0)
    p.wire(midiin, 0, parse)
    p.wire(fmt, 0, midiout)
    # Sysex passes untouched, so a Patch Sync device on the same track still
    # reaches the synth and hears its replies. [midiparse] drops it.
    sysexin = p.obj("sysexin", 200, 30, 1, 1, ["int"])
    p.wire(sysexin, 0, midiout)

    # Notes pass here unless the js says it sends them itself (gate closed).
    note_gate = p.obj("gate 1 1", 30, 300, 2, 1)
    p.wire(parse, 0, note_gate, 1)
    p.wire(note_gate, 0, fmt, 0)
    p.wire(parse, 1, fmt, 1)                 # poly pressure
    slide_route = p.obj("route 74", 150, 110, 2, 2)
    p.wire(parse, 2, slide_route)
    p.wire(slide_route, 1, fmt, 2)           # other controllers
    p.wire(parse, 3, fmt, 3)                 # program change
    p.wire(parse, 6, fmt, 6)                 # channel

    js = p.obj("js mperemap.js", 400, 300, 1, 8,
               saved_object_attributes={"filename": "mperemap.js", "parameter_enable": 0})
    # The js sends complete messages to a [midiout] of its own, so its bytes
    # can not get mixed with those of the direct path.
    js_midiout = p.obj("midiout", 400, 520, 1, 0)
    p.wire(js, 0, js_midiout)
    p.wire(js, 1, note_gate, 0)

    # The js gets the raw bytes and puts the messages together itself:
    # [midiparse] loses the channel of bend, pressure and controllers on its
    # midievent outlet. They go through [deferlow], which keeps their order;
    # [js] on its own defers what comes from the MIDI thread and can turn a
    # burst around.
    to_js = p.obj("deferlow", 400, 240, 1, 1)
    p.wire(midiin, 0, to_js)
    p.wire(to_js, 0, js)

    # ---- controls: one row per MPE dimension
    for text, x, w in (("Target", 36, 78), ("CC", 118, 150), ("Min", 272, 36), ("Max", 310, 36), ("Curve", 348, 40)):
        comment(p, text, [x, 1, w, 14])
    # Controller names for the chosen synth. Each menu lists all 128
    # controllers, so its item number is the controller number.
    cc_menus = p.obj("route " + " ".join(src for src, _, _ in SOURCES), 1300, 300, 1, len(SOURCES) + 1)
    p.wire(js, 6, cc_menus)
    keys = []          # names the js uses to set the controls, in order
    controls = []
    for row, (src, label, shown) in enumerate(SOURCES):
        y = 16 + 18 * row
        px = 800
        py = 30 + 90 * row
        comment(p, shown, [2, y, 34, 15], x=px - 60, y=py)
        fields = [
            ("target", menu(p, px, py, [36, y, 78, 15], src + "target", label + " Target",
                            label + " To", TARGET_LABELS, DEFAULT_TARGET[src],
                            "Where the %s of each note is sent." % label.lower())),
            ("cc", numbox(p, px + 100, py, [118, y, 30, 15], src + "cc", label + " CC", label + " CC",
                          0, 127, DEFAULT_CC[src],
                          "Controller number, used when the target is CC.")),
            ("min", numbox(p, px + 200, py, [272, y, 36, 15], src + "min", label + " Min",
                           label + " Min", 0, 100, 0,
                           "Output at rest, in percent of the target's range. Above Max it turns "
                           "the direction around. Not used for Bend, which is two-sided.",
                           unitstyle=5)),
            ("max", numbox(p, px + 250, py, [310, y, 36, 15], src + "max", label + " Max",
                           label + " Max", 0, 100, 100,
                           "Output at full travel, in percent of the target's range. For Bend: its depth.",
                           unitstyle=5)),
            ("curve", numbox(p, px + 300, py, [348, y, 40, 15], src + "curve", label + " Curve",
                             label + " Crv", -100, 100, 0,
                             "Response curve (exponent). 0 is straight; +100 lifts small values (x^0.25), -100 holds them back (x^4).")),
        ]
        cc_name = p.add("umenu", [px + 100, py - 26, 120, 18], 1, 3, ["int", "", ""],
                        pres=[150, y - 1, 118, 17], items=[], parameter_enable=0, fontsize=9.0,
                        varname=src + "ccname")
        p.wire(cc_menus, row, cc_name)
        p.wire(cc_name, 0, fields[1][1])     # picking a name sets the number
        for field, oid in fields:
            pre = p.obj("prepend p %s %s" % (src, field), px + 100 * FIELD_X[field], py + 30, 1, 1)
            p.wire(oid, 0, pre)
            p.wire(pre, 0, js)
            keys.append(src + field)
            controls.append(oid)

    comment(p, "Bend range (st)", [2, 72, 114, 15], x=740, y=320)
    bend_range = numbox(
        p, 900, 320, [118, 72, 40, 15], "bendrange", "Bend Range", "Bend Rng", 0.5, 96, 48.0,
        "Semitones of bend that count as full scale: the pitch bend range of the synth for "
        "Pitch Bend, the sideways movement for a full swing for CC or aftertouch. "
        "48 leaves an MPE bend unchanged.", is_float=True, unitstyle=1)
    pre = p.obj("prepend p range", 900, 350, 1, 1)
    p.wire(bend_range, 0, pre)
    p.wire(pre, 0, js)
    keys.append("range")
    controls.append(bend_range)

    # The js sets the controls when a preset is loaded.
    setter = p.obj("route " + " ".join(keys), 560, 360, 1, len(keys) + 1)
    p.wire(js, 2, setter)
    for i, oid in enumerate(controls):
        p.wire(setter, i, oid)

    # When the device has loaded, make every control report its restored value.
    thisdev = p.obj("live.thisdevice", 1100, 80, 1, 3, ["bang", "int", "int"])
    for oid in controls:
        p.wire(thisdev, 0, oid)

    # ---- synth: decides which controller names the menus show
    comment(p, "Synth", [200, 72, 40, 15], x=1300, y=390)
    synth = p.add("umenu", [1300, 420, 110, 20], 1, 3, ["int", "", ""], pres=[242, 71, 146, 17],
                  items=[], parameter_enable=0, fontsize=9.0, varname="synthmenu")
    p.wire(js, 7, synth)
    pre = p.obj("prepend synth", 1300, 450, 1, 1)
    p.wire(synth, 1, pre)
    p.wire(pre, 0, js)

    # ---- presets
    presets = p.add("umenu", [560, 420, 130, 20], 1, 3, ["int", "", ""], pres=[2, 92, 170, 18],
                    items=[], parameter_enable=0, fontsize=10.0, varname="presetmenu")
    p.wire(js, 3, presets)
    pre = p.obj("prepend preset", 560, 450, 1, 1)
    p.wire(presets, 1, pre)
    p.wire(pre, 0, js)

    name = p.add("textedit", [720, 420, 130, 20], 1, 4, ["", "int", "", ""], pres=[176, 92, 212, 18],
                 keymode=1, lines=1, fontsize=10.0, varname="presetname_edit")
    name_route = p.obj("route text", 720, 450, 1, 2)
    pre_save = p.obj("prepend save", 720, 480, 1, 1)
    p.wire(name, 0, name_route)
    p.wire(name_route, 0, pre_save)
    p.wire(pre_save, 0, js)
    comment(p, "Type a name, then Save", [2, 113, 170, 15], x=720, y=390)

    save = p.button(880, 390, [176, 113, 56, 15], "savebtn", "Save Preset", "Save",
                    annotation="Save the mapping under the name typed above, "
                               "or under the current preset if the box is empty.")
    save_bang = p.obj("t b", 880, 420, 1, 1, ["bang"])
    p.wire(save, 0, save_bang)
    p.wire(save_bang, 0, name)

    delete = p.button(960, 390, [236, 113, 56, 15], "delbtn", "Delete Preset", "Delete",
                      annotation="Delete the current preset from the preset file.")
    m_del = p.msg("del", 960, 420)
    p.wire(delete, 0, m_del)
    p.wire(m_del, 0, js)

    log = p.button(1040, 390, [298, 113, 56, 15], "logbtn", "Write Log", "Log",
                   annotation="Append the last MIDI events in and out to mperemap-debug.log.")
    m_log = p.msg("log", 1040, 420)
    p.wire(log, 0, m_log)
    p.wire(m_log, 0, js)

    # ---- help: "?" opens the text above in a window
    help_btn = p.button(1120, 390, [358, 113, 30, 15], "helpbtn", "Help", "?",
                        annotation="Open the help window.")
    m_open = p.msg("open", 1120, 420)
    pcontrol = p.obj("pcontrol", 1120, 450, 1, 1)
    help_sub = p.obj("p Help", 1120, 480, 1, 0, patcher=help_patcher())
    p.wire(help_btn, 0, m_open)
    p.wire(m_open, 0, pcontrol)
    p.wire(pcontrol, 0, help_sub)

    # The preset name is kept in a hidden parameter, so the menu shows it
    # again when the Live set is reopened.
    store = p.obj(
        "pattr presetname", 400, 400, 1, 3, varname="presetname",
        saved_object_attributes={"parameter_enable": 1, "parameter_mappable": 0},
        saved_attribute_attributes={"valueof": {
            "parameter_invisible": 1, "parameter_longname": "Preset Name",
            "parameter_shortname": "Preset Name", "parameter_modmode": 0,
            "parameter_type": 3}})
    p.param(store, "Preset Name", "Preset Name")
    pre_restore = p.obj("prepend restore", 400, 440, 1, 1)
    p.wire(js, 5, store)
    p.wire(store, 0, pre_restore)
    p.wire(pre_restore, 0, js)

    status = p.msg("-", 560, 500, pres=[2, 134, 386, 20], ignoreclick=1)
    p.wire(js, 4, status)

    write_device(
        p, os.path.join(HERE, "MPE Remap.amxd"), "mperemap.js",
        ["Bend Target", "Slide Target", "Press Target", "Bend Range",
         "Bend Max", "Slide Max", "Press Max", "Press Curve"],
        width=WIDTH, is_mpe=1)


FIELD_X = {"target": 0, "cc": 1, "min": 2, "max": 2.5, "curve": 3}

if __name__ == "__main__":
    build()
