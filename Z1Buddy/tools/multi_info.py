#!/usr/bin/env python3
"""Show what is in a Korg Z1 Multi setup dump, or build the default one.

    multi_info.py file.syx             print name and the six timbres
    multi_info.py --default out.syx    write a Multi setup for MPE: six timbres on
                                       channels 2-7, all on program A127

Programs are numbered 0-127 here, as on the Buddy and in Patch Sync: A127 is the
last program of bank A (the Z1's own display counts from 1 and calls it A128).
It is the program the Buddy overwrites when it enters MPE mode (storeForMPE()
in the firmware writes to program number 7Fh).

The layout is the "Multi Set Parameters" list of the Korg MIDI implementation:
a 16 character name, then 16 bytes per timbre.
"""
import sys

# A Multi setup read from a Z1 ("Techno Tonic"), used as the starting point for
# --default so that everything this script does not set has a valid value.
BASE = bytes.fromhex("f0423046490000546563686e6f2000546f6e696320200420206e010000110264000030007f6412102040006e01180800113000004130487f64100000006f20010400111400002041006401100800000006010000117f01003c41007f01100b1a40000206000004117f00657f007f2401101900000c02100000111e00657f10427f011018400000066e0019000000001e00005a000c020016000000231e002000000700004e050000000032550000000a231e000000000000000000000022071d2d4d1e0f00000000280a000000000000001510261802017e000000000000000000000000000000000000001121000000014b0000f7")


def unpack(enc):
    out = []
    for i in range(0, len(enc), 8):
        for j in range(1, 8):
            if i + j < len(enc):
                out.append(enc[i + j] | (0x80 if enc[i] & (1 << (j - 1)) else 0))
    return out


def pack(data):
    out = []
    for i in range(0, len(data), 7):
        group = data[i:i + 7]
        out.append(sum(1 << j for j, v in enumerate(group) if v & 0x80))
        out.extend(v & 0x7F for v in group)
    return out


def payload(msg):
    """Data of a current (49h) or stored (4Dh) Multi setup dump."""
    if len(msg) < 8 or msg[0] != 0xF0 or msg[1] != 0x42 or msg[3] != 0x46:
        raise SystemExit("not a Korg Z1 sysex message")
    if msg[4] == 0x49:
        return unpack(list(msg[6:-1]))
    if msg[4] == 0x4D:
        return unpack(list(msg[9:-1]))
    raise SystemExit("not a Multi setup dump (function %02Xh)" % msg[4])


def signed(v):
    return v - 256 if v > 127 else v


def describe(msg):
    d = payload(msg)
    print("name: [%s]" % "".join(chr(c) if 32 <= c < 127 else "." for c in d[:16]))
    for t in range(6):
        b = d[16 + 16 * t:32 + 16 * t]
        ch = b[12] & 0x1F
        print("timbre %d: program %s%03d  channel %-3s  reserve %2d  transpose %+d  detune %+d  "
              "level %s  program change %s" % (
                  t + 1, "B" if b[0] & 0x80 else "A", b[0] & 0x7F,
                  "GLB" if ch > 15 else ch + 1, b[1], signed(b[2]), signed(b[3]),
                  "PROG" if b[5] > 127 else b[5], "on" if b[12] & 0x80 else "off"))
    fx = ["Overdrive", "Compressor", "Parametric EQ", "Wah", "Exciter", "Decimator", "Chorus", "Flanger",
          "Phaser", "Rotary small", "Delay-mono", "Talking Modulator", "Multitap Delay", "Ensemble",
          "Rotary large"]
    master = ["Stereo Delay", "Reverb-Hall", "Reverb-Room"]
    o = FX_AT
    pick = lambda names, v: names[v] if v < len(names) else "?"
    if d[o] == 12:
        print("multitap delay: tap 1 %d ms, tap 2 %d ms, feedback %d, balance %d %%" % (
            round(d[o + 4] * 680 / 251), round(d[o + 6] * 680 / 251), signed(d[o + 7]), d[o + 15]))
    print("effects (shared by all timbres): %s -> %s -> master %s; effect send per timbre: %s" % (
        pick(fx, d[o]), pick(fx, d[o + 23]), pick(master, d[o + 46]),
        " ".join("PROG" if d[16 + 16 * t + 7] > 100 else str(d[16 + 16 * t + 7]) for t in range(6))))


# ---- effects of the default setup
# In a Multi setup the effects are shared by all timbres. After the six timbres
# come: effect 1 type + 22 setting bytes, effect 2 type + 22, master effect type
# + 18. The setting bytes are that effect's parameters in the order of the Korg
# list (checked against setups read from the Z1).
FX_AT = 16 + 6 * 16
TEMPO = 136.0                   # BPM the delay is timed for ("Sandstorm")


def delay_value(ms):
    """Delay time 0-680 ms as the Z1 value 0-251, taken to be an even scale."""
    return max(0, min(251, round(ms / 680.0 * 251)))


def s8(v):
    return v & 0xFF


def set_effects(d):
    beat = 60000.0 / TEMPO
    # Effect 1: multitap delay, the two taps an eighth and a dotted eighth apart
    multitap = [
        0,                          # type: Normal
        0, 0,                       # input level modulation: none
        delay_value(beat / 2),      # tap 1 time: 1/8 note
        85,                         # tap 1 level
        delay_value(beat * 3 / 4),  # tap 2 time: dotted 1/8
        s8(35),                     # feedback
        0, 0,                       # feedback modulation: none
        10,                         # low damp
        35,                         # high damp
        s8(70),                     # spread: wide
        0, 0,                       # spread modulation: none
        22,                         # effect balance, percent
        0, 0,                       # balance modulation: none
    ]
    d[FX_AT] = 12                   # Multitap Delay
    d[FX_AT + 1:FX_AT + 23] = multitap + [0] * (22 - len(multitap))
    # Effect 2 (a flanger in the base setup) and the master effect (a stereo
    # delay) are left as they are but turned fully dry: their effect balance,
    # the 12th setting byte of each, to 0.
    d[FX_AT + 23 + 1 + 11] = 0
    d[FX_AT + 46 + 1 + 11] = 0


def default():
    d = payload(BASE)
    d[:16] = list(b"Z1 Buddy MPE    ")
    for t in range(6):
        o = 16 + 16 * t
        d[o + 0] = 127          # program A127 (counting from 0), the Buddy's MPE slot
        d[o + 1] = 2            # reserve voice
        d[o + 2] = 0            # transpose
        d[o + 3] = 0            # detune
        d[o + 5] = 127          # output level
        d[o + 6] = 128          # panpot: as in the program
        d[o + 7] = 101          # effect send: as in the program (the base had a different one per timbre)
        d[o + 8] = 127          # key zone top
        d[o + 9] = 0            # key zone bottom
        d[o + 10] = 127         # velocity zone top
        d[o + 11] = 1           # velocity zone bottom
        d[o + 12] = t + 1       # MIDI channel 2-7, program change off
    set_effects(d)
    return bytes([0xF0, 0x42, 0x30, 0x46, 0x49, 0x00] + pack(d) + [0xF7])


if __name__ == "__main__":
    if len(sys.argv) == 3 and sys.argv[1] == "--default":
        open(sys.argv[2], "wb").write(default())
        describe(default())
    elif len(sys.argv) == 2:
        describe(open(sys.argv[1], "rb").read())
    else:
        raise SystemExit(__doc__)
