#!/usr/bin/env python3
"""Check the parameter table in firmware/z1_params.h against the Korg list.

    tools/check_params.py [path to korg_z1_v1.0_midi.txt]

Reports every parameter whose range differs from the "Program Parameters"
list of the Korg Z1 MIDI implementation, and every key that list does not
have. The Korg document is not part of this repository (it is Korg's); by
default it is looked for in ../docs/ of the checkout.
"""
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
DOC = sys.argv[1] if len(sys.argv) > 1 else os.path.join(HERE, "..", "..", "docs", "korg_z1_v1.0_midi.txt")
BLOCKS = {0: "", 1: "OSC1 ", 2: "OSC2 ", 3: "FX1 ", 4: "FX2 ", 5: "master FX "}


def korg():
    text = open(DOC, errors="replace").read().replace("\r", "").split("\n")
    start = next(i for i, l in enumerate(text) if "KORG Z1 - Program Parameters" in l)
    end = next(i for i, l in enumerate(text) if "KORG Z1 - Multi Set Parameters" in l)
    out = {}
    for line in text[start:end]:
        m = re.match(r"^\|\s*(\d+)\|([^|]*)\|([^|]*)\|\s*(-?\d+)\s*([~/])\s*\+?(-?\d+)[^|]*\|", line)
        if m:
            # "-1/0~99" style ranges: take the outer values
            more = re.findall(r"-?\d+", line.split("|")[4])
            lo, hi = min(int(v) for v in more), max(int(v) for v in more)
            out.setdefault(int(m.group(1)), (m.group(2).strip(), lo, hi))
    return out


def table():
    src = open(os.path.join(HERE, "..", "firmware", "z1_params.h"), newline="").read()
    rows = []
    for m in re.finditer(r'\{\s*(?:(0x[0-9A-Fa-f]+)\s*\+\s*)?(\d+),\s*"([^"]*)",\s*(-?\d+),\s*(-?\d+),\s*(-?\d+),\s*editableType::(\w+)', src):
        base = int(m.group(1), 16) if m.group(1) else 0
        rows.append((base + int(m.group(2)), m.group(3), int(m.group(4)), int(m.group(5)), m.group(7)))
    return rows


def main():
    doc = korg()
    rows = table()
    problems = 0
    for key, name, lo, hi, kind in rows:
        if 10000 <= key < 0x2800:
            continue                      # the Buddy's own settings
        pid, block = key & 0x7FF, key >> 11
        where = "%s%d (%s)" % (BLOCKS.get(block, "?"), pid, name)
        if pid not in doc:
            print("not in the Korg list: %s" % where)
            problems += 1
            continue
        kname, klo, khi = doc[pid]
        if kind == "TextEnum":
            if lo < klo or hi > khi:
                print("list outside the Korg range: %s is %d..%d, Korg '%s' %d..%d" % (where, lo, hi, kname, klo, khi))
                problems += 1
        elif (lo, hi) != (klo, khi):
            print("range differs: %s is %d..%d, Korg '%s' %d..%d" % (where, lo, hi, kname, klo, khi))
            problems += 1
    print("%d parameters checked, %d to look at" % (len(rows), problems))
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
