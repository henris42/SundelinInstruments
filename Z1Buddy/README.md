# Z1 Buddy

A hardware front end for the Korg Z1: a box with three displays, 18 knobs and a USB
port that sits between the computer and the Z1. It gives the Z1 a knob-per-function
editor, connects it to the computer over USB, and lets it be played from an MPE
controller.

| Document | For | Contents |
| --- | --- | --- |
| [USER_GUIDE.md](USER_GUIDE.md) | Anyone playing the Z1 with the Buddy | Connecting it, the controls, editing sounds, unison, MPE mode, wonkiness |
| [DEVELOPMENT.md](DEVELOPMENT.md) | Anyone changing the firmware | Building and flashing, hardware pins, code map, MIDI bridge, MPE scheduler, open questions |

Quick start for firmware work: `tools/buddy go` builds, flashes and opens the debug
monitor.

Folders: `firmware/` is the current source, `build_z1buddy/` the latest built image, `tools/` the command-line scripts,
and the `.prt` and `.stl` files are the enclosure.
