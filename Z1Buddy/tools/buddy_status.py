#!/usr/bin/env python3
"""Print the Z1 Buddy's debug status (Buddy sysex 12h) in words.

    buddy_status.py status.syx

The layout is described at buddySendDebug() in firmware/firmware.ino.
Normally run through "tools/buddy status".
"""
import sys

NOTES = ["C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"]


def note_name(n):
    return "%s%d" % (NOTES[n % 12], n // 12 - 1)


def main(path):
    m = open(path, "rb").read()
    if len(m) < 12 or m[:5] != bytes([0xF0, 0x7D, 0x5A, 0x31, 0x12]):
        raise SystemExit("not a Buddy debug status")
    d = list(m[5:-1])
    pos = [0]

    def take(n=1):
        v = 0
        for i in range(n):
            v |= d[pos[0] + i] << (7 * i)
        pos[0] += n
        return v

    version = take()
    if version != 1:
        raise SystemExit("unknown status format %d" % version)
    mode, unison, wonk, porta, detune, bank, prog = (take() for _ in range(7))
    uptime = take(3)
    ev, sx, bg, insx = take(2), take(2), take(), take()
    max_ev, max_wait, max_loop = take(2), take(2), take(2)
    on_in, off_in, on_out, off_out = take(3), take(3), take(3), take(3)
    expr, sysex, stalls = take(3), take(3), take(2)
    ni, no = take(), take()
    held_in = [(take() + 1, take()) for _ in range(ni)]
    held_out = [(take() + 1, take()) for _ in range(no)]

    print("mode %s, unison %s, wonkiness %d, portamento %d, detune %d, last patch %s %d, up %d min %02d s" % (
        ["Normal", "entering MPE", "MPE"][mode] if mode < 3 else mode, ["off", "2", "3", "6"][unison & 3],
        wonk, porta, detune, "?" if bank == 0x7F else "AB"[bank & 1], prog, uptime // 60, uptime % 60))
    print("waiting now:     %d events, %d sysex bytes, %d background changes%s" % (
        ev, sx, bg, ", a sysex message is going out" if insx else ""))
    print("since last report: most events waiting %d, longest wait %d ms, longest loop %d ms" % (max_ev, max_wait, max_loop))
    print("notes from the computer: %d on, %d off      sent to the Z1: %d on, %d off" % (on_in, off_in, on_out, off_out))
    print("expression messages sent: %d      sysex messages sent: %d (%d closed unfinished)" % (expr, sysex, stalls))

    def show(title, held):
        if held:
            print("%s %s" % (title, ", ".join("%s (%d) on channel %d" % (note_name(n), n, ch) for ch, n in held)))
        else:
            print("%s none" % title)

    show("on, as received from the computer:", held_in)
    show("on, as sent to the Z1:            ", held_out)
    if not held_in and not held_out:
        print("=> the Buddy has no note on. A note that still sounds got its note-off from the Buddy; the Z1 did not take it.")
    elif held_in:
        print("=> the computer has not sent a note-off for the notes in the first list (if no key is held now, look upstream: Live, its devices, the controller).")
    else:
        print("=> the Buddy holds notes for the Z1 that the computer no longer does: a fault in the Buddy.")


if __name__ == "__main__":
    if len(sys.argv) != 2:
        raise SystemExit(__doc__)
    main(sys.argv[1])
