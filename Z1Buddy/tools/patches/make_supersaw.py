#!/usr/bin/env python3
"""Build supersaw.syx: a Roland JP-8000 style supersaw for the Korg Z1, made to
be played through the Z1 Buddy in MPE mode with unison 6.

The file is a list of Korg parameter changes (plus two controllers that switch
the insert effects off). Sending it turns whatever sound is selected on the Z1
into the patch; nothing is stored until you write it on the Z1 or enter MPE
mode on the Buddy.

    python3 make_supersaw.py [path to korg_z1_v1.0_midi.txt [ids]]
        ids: a comma-separated list of parameter IDs; then only those are
        written, to supersaw_part.syx, for adding to a sound that is on the
        Z1 already without undoing what was changed there by ear
    ../z1send "Z1 Buddy" supersaw.syx

How it gets close to a supersaw
  A JP-8000 supersaw is seven detuned sawtooth oscillators in one voice.
  One Z1 voice has three: OSC1, OSC2 and the sub oscillator, all set to saw
  here and tuned -8 / +8 / 0 cents. In MPE mode with unison 6 the Buddy plays
  each note on all six timbres and spreads them with its Detune pot, which
  makes 18 saws per note.
"""
import os
import re
import sys

DOC = sys.argv[1] if len(sys.argv) > 1 else os.path.expanduser(
    "~/Documents/SundelinInstruments/docs/korg_z1_v1.0_midi.txt")
O1, O2 = 0x800, 0x1000           # ExID of the OSC1 and OSC2 type-specific parameters
AFTERTOUCH = 19                  # "AfterT" in the Z1's modulation source list 1
LFO1 = 6                         # "LFO1" in the same list
MOD_WHEEL = 20                   # "MWheel"


def doc_parameters():
    """id -> (name, low, high) for the program parameters in the Korg list."""
    text = open(DOC, errors="replace").read().replace("\r", "").split("\n")
    start = next(i for i, l in enumerate(text) if "KORG Z1 - Program Parameters" in l)
    end = next(i for i, l in enumerate(text) if "KORG Z1 - Multi Set Parameters" in l)
    out = {}
    for line in text[start:end]:
        m = re.match(r"^\|\s*(\d+)\|([^|]*)\|[^|]*\|\s*(-?\d+)\s*[~/]\s*\+?(-?\d+)[^|]*\|", line)
        if m:
            out.setdefault(int(m.group(1)), (m.group(2).strip(), int(m.group(3)), int(m.group(4))))
    return out


def build():
    params = doc_parameters()
    values = {}

    # 1. A clean start: every modulation amount to zero, so nothing of the
    #    sound that was selected before leaks through. These are the
    #    parameters with "Int" in their name and a range around zero.
    for pid, (name, lo, hi) in params.items():
        if pid >= 800:                       # effect settings: left alone
            continue
        if lo < 0 and re.search(r"\bInt\b|Int\.|Intensity|Velocity Control|Veloc", name):
            if 464 <= pid < 800:             # type-specific oscillator parameters: both oscillators
                if pid <= 485:               # standard oscillator only
                    values[pid | O1] = 0
                    values[pid | O2] = 0
            else:
                values[pid] = 0

    def put(pid, value):
        values[pid] = value

    # 2. Common
    for i, ch in enumerate("Supersaw MPE".ljust(16)):
        put(1 + i, ord(ch))
    put(17, 2)        # category: Synth-Lead
    put(19, 0)        # hold off
    put(21, 2)        # voice assign: poly (the Buddy sets mono when it enters MPE)
    put(24, 0)        # unison off: the Buddy does the stacking
    put(25, 0)
    put(30, 3)        # a little random pitch, so the saws never line up
    put(163, 24)      # pitch bend up: 24 semitones, the most the Z1 allows
    put(164, -24)     # pitch bend down
    put(165, 0)       # smooth
    put(166, 0)
    put(169, 0)       # portamento off

    # 3. Three sawtooth oscillators per voice
    for base, ex, fine in ((174, O1, -8), (188, O2, 8)):
        put(base, 0)              # type: Standard
        put(base + 1, 2)          # octave 8'
        put(base + 2, 0)          # semitone
        put(base + 3, fine)       # fine tune, cents
        put(base + 4, 0)          # frequency offset
        put(base + 5, 60)         # pitch slope: centre key
        put(base + 6, 50)         # lower slope 1.00 = normal tracking
        put(base + 7, 50)         # higher slope 1.00
        put(464 | ex, 0)          # wave: saw
        put(465 | ex, 99)         # wave edge: brightest
        put(466 | ex, 99)         # wave level
        put(467 | ex, 0)          # triangle level
        put(468 | ex, 0)          # sine level
        put(470 | ex, 0)          # wave form
        put(475 | ex, 0)          # wave shape input level
        put(483 | ex, 0)          # wave shape balance: unshaped only
    put(202, 2)       # sub oscillator: 8', in unison with the others
    put(203, 0)
    put(204, 0)
    put(205, 0)
    put(206, 60)
    put(207, 50)
    put(208, 50)
    put(215, 0)       # sub oscillator wave: saw

    # 3b. Vibrato from the mod wheel: LFO 1 modulates the pitch of all three
    #     oscillators, by nothing at rest and by VIBRATO at full wheel.
    VIBRATO = 14
    put(107, 0)       # LFO 1 wave: triangle
    put(108, 0)       # key sync off: it runs freely
    put(109, 150)     # frequency, of 0-199 (the scale in Hz is not documented)
    put(114, 0)       # no fade in
    put(117, 0)       # offset
    put(118, 0)       # MIDI sync off
    for base in (182, 196, 209):       # pitch modulation 1 of OSC1, OSC2, sub oscillator
        put(base, LFO1)                # source
        put(base + 1, 0)               # fixed amount: none
        put(base + 2, MOD_WHEEL)       # the amount is controlled by the mod wheel
        put(base + 3, VIBRATO)         # by this much at full wheel

    # 4. Mixer: everything into filter 1, the centre saw a little louder
    put(224, 75); put(227, 0)     # OSC1 to out 1 / out 2
    put(230, 75); put(233, 0)     # OSC2
    put(236, 85); put(239, 0)     # sub oscillator
    put(242, 0); put(245, 0)      # noise
    put(248, 0); put(251, 0)      # feedback
    put(254, 1); put(255, 1); put(256, 1); put(257, 0)

    # 5. Filters in series: a low pass that is nearly open, then a high pass
    #    that thins the bottom the way the JP's supersaw does.
    put(258, 0)       # routing: serial 1
    put(259, 0)       # filter 2 link off
    put(261, 1)       # filter 1: low pass
    put(262, 70)      # input trim
    put(263, 88)      # cutoff
    put(274, 8)       # resonance
    put(270, AFTERTOUCH)          # pressure opens the filter further
    put(271, 25)
    put(288, 2)       # filter 2: high pass
    put(289, 99)
    put(290, 20)      # cutoff

    # 6. Amplifier: instant attack, full sustain, short release
    put(315, 99); put(320, 5)     # amp 1 level, controlled by the amp EG
    put(324, 99); put(329, 5)     # amp 2
    put(334, 0)       # attack time
    put(335, 99)      # attack level
    put(336, 40)      # decay time
    put(337, 92)      # break level
    put(338, 50)      # slope time
    put(339, 90)      # sustain level
    put(340, 22)      # release time
    put(344, 25)      # velocity to level
    put(352, 64)      # pan: centre
    put(355, 115)     # output level
    put(356, 18)      # effect send: a little of whatever the master effect is

    out = bytearray()
    only = set(int(a) for a in sys.argv[2].split(",")) if len(sys.argv) > 2 else None   # for sending a few parameters only
    for pid in sorted(values):
        if only is not None and pid not in only:
            continue
        v = values[pid] & 0x3FFF               # 14-bit two's complement
        out += bytes([0xF0, 0x42, 0x30, 0x46, 0x41, 0x01,
                      pid & 0x7F, (pid >> 7) & 0x7F, v & 0x7F, (v >> 7) & 0x7F, 0xF7])
    if only is None:
        out += bytes([0xB0, 94, 0, 0xB0, 95, 0])   # insert effects 1 and 2 off
    return out, len(values)


if __name__ == "__main__":
    data, count = build()
    name = "supersaw.syx" if len(sys.argv) <= 2 else "supersaw_part.syx"
    path = os.path.join(os.path.dirname(os.path.abspath(__file__)), name)
    open(path, "wb").write(data)
    print("%s: %d parameter changes, %d bytes" % (os.path.basename(path), count, len(data)))
