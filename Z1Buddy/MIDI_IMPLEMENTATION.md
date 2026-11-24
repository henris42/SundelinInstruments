# Z1 Buddy: MIDI implementation

What the Z1 Buddy does with every MIDI message, as seen from the computer or
controller connected to its USB port. This is the definition; where the firmware
differs from it, the firmware is wrong.

Firmware of November 2026. Parts marked *new* have been built and tested off the
device but not yet confirmed on the instrument.

## Contents

- [Connections](#connections)
- [Modes](#modes)
- [Normal mode](#normal-mode)
- [MPE mode](#mpe-mode)
- [Pitch bend of a key](#pitch-bend-of-a-key)
- [What is thinned and what is not](#what-is-thinned-and-what-is-not)
- [From the Z1 to the computer](#from-the-z1-to-the-computer)
- [System exclusive](#system-exclusive)
- [Buddy messages](#buddy-messages)

## Connections

| Port | Direction | Carries |
| --- | --- | --- |
| USB, MIDI port "Z1 Buddy" | Computer ↔ Buddy | Everything in this document |
| DIN out | Buddy → Z1 | What the Buddy sends on to the Z1, 31.25 kbaud |
| DIN in | Z1 → Buddy | What the Z1 sends; passed on to USB |

The Z1 must be set to global MIDI channel 1 and have "SysEx Receive" enabled.

## Modes

| Mode | Use | The Z1 is in |
| --- | --- | --- |
| Normal | An ordinary keyboard or sequencer | Program mode |
| MPE | An MPE controller, one channel per note | Multi mode, six timbres on channels 2–7, all playing program A127 |

The mode is changed with the button on the Buddy or with a
[Buddy message](#buddy-messages). Entering MPE mode stores the current sound in
program A127 (the last program of bank A, counting from 0), switches the Z1 to
Multi mode and loads the six-timbre setup; leaving it switches the Z1 back to
Program mode.

## Normal mode

Everything from the computer is sent to the Z1 on **channel 1**, whatever channel
it arrived on.

| Message | Sent to the Z1 | How |
| --- | --- | --- |
| Note on, note off | Yes | At once, never dropped |
| Program change | Yes | At once |
| Bank select (CC 0, CC 32) | Yes | At once, so it precedes the program change |
| Pitch bend | Yes, as it is | Thinned |
| Other controllers | Yes | Thinned |
| Channel pressure, poly pressure | Yes | Thinned |
| Clock, start, continue, stop | Yes | At once |
| Song position, song select, tune request, time code | Yes | At once |
| System exclusive | Korg Z1 messages only | See [System exclusive](#system-exclusive) |

## MPE mode

The Buddy is an MPE receiver with one zone:

| | |
| --- | --- |
| Master channel | 1 |
| Note channels | 2–7 with unison off (six notes). With unison on, any of 2–16; see below. |
| Per-note pitch bend | See [Pitch bend of a key](#pitch-bend-of-a-key) |
| Per-note timbre | CC 74 |
| Per-note pressure | Channel pressure |

| Message | On the master channel (1) | On a note channel |
| --- | --- | --- |
| Note on, note off | Sent on channel 1 | Sent on that channel, at once, never dropped |
| Pitch bend | Added to the bend of all six voices, as it is: a pitch wheel moves everything by the sound's own bend range | That note's bend, scaled; thinned |
| CC 74 | Renumbered, then to all six voices | Renumbered, to that voice; thinned |
| Other controllers | To all six voices | To that voice; thinned |
| Channel pressure | Sent on channel 1 | Channel pressure for that voice; thinned |
| Poly pressure | Sent on channel 1 | Sent on that channel; thinned |
| Program change | Sent | Sent |
| RPN 0 (CC 101, 100, then 6 and 38) | Passed on as controllers | Sets the Buddy's key bend range; not passed on |

**CC 74** is sent to the Z1 as controller 7 (volume). The Z1 has no fixed timbre
controller; the number is a setting in the firmware (`C74To`) that has no control
on the panel yet.

**Unison** (*new*). With the Buddy's unison set to 2, 3 or 6, each note is played
by that many voices, detuned against each other, and 3, 2 or 1 notes can sound.
The voices of a note are a fixed group of channels (2–3, 4–5, 6–7; or 2–4, 5–7; or
2–7). A note channel from the controller is given a group at its first message,
whichever of channels 2–16 it is, and everything on that channel goes to all
voices of the group. When all groups are in use, the oldest note is ended to make
room.

## Pitch bend of a key

*New.*

MIDI has no way for two devices to agree on a bend range: each side has a setting,
and they have to be set the same. The Buddy's side is this:

| | |
| --- | --- |
| Range of the incoming bend | A setting of the Buddy: what a full bend of a key means, in semitones and cents. **48 semitones** on a new Buddy, the value MPE prescribes. Set the controller's own pitch bend range to the same value. |
| Changing it | Buddy message `08` (`tools/buddy bendrange`), or the standard MIDI way: RPN 0 on a note channel (CC 101 = 0, CC 100 = 0, CC 6 = semitones, CC 38 = cents). Kept when the Buddy is switched off. |
| Result on the Z1 | True pitch: a key bent by a semitone sounds a semitone higher. |
| Limit | The pitch bend range of the sound on the Z1 (its "PB Intensity", normally 2 semitones). A bend beyond that stays at the limit. |
| Depth | 100 % of true pitch, adjustable with Buddy message `07`; also kept. |

The Z1 plays a pitch bend in the scale of the sound: a full bend is the sound's
bend range. So the Buddy multiplies a key's bend by

    range of the incoming bend ÷ range of the sound × depth

For the standard case that is 48 ÷ 2 = 24.

**Resolution.** Pitch bend is a 14-bit value, 8,192 steps each way.

| Range set on controller and Buddy | One step is |
| --- | --- |
| 48 semitones (standard MPE) | 0.59 cent |
| 81 semitones 92 cents | exactly 1 cent |
| 12 semitones | 0.15 cent |
| 2 semitones | 0.024 cent |

What the Buddy sends on to the Z1 is finer than any of these needs: with a sound
whose bend range is 2 semitones, one step to the Z1 is 0.024 cent. So the choice of
range decides the resolution, and it is made on the controller: a controller whose
keys only ever bend a semitone or so gets the finest pitch from a narrow range.
The Osmose does not send its range; it has to be set by hand on both sides. When
it is played through Live, what counts is the range Live uses for its MPE output,
48 unless changed.

How finely the Z1 itself follows a pitch bend is not stated in Korg's documents.
They say only that it sends and receives pitch bend as the full 14 bits, and that
its tuning parameters are set in steps of 1 cent.

## What is thinned and what is not

A DIN cable carries 3,125 bytes a second, far less than an MPE controller sends
for a chord. The Buddy decides what the cable carries next, in this order:

| Order | Messages | Guarantee |
| --- | --- | --- |
| Always | Clock, start, continue, stop | Passed at once, untouched |
| 1 | Notes, program change, bank select | Every one is sent, in order |
| 2 | Pitch bend, controllers, pressure | The newest value of each always arrives; values in between may be skipped. Pitch bend goes before the others. |
| 3 | System exclusive | Every message is sent, in order and in one piece; it waits for a gap, at most 20 ms at a time |
| 4 | The Buddy's own background changes | When nothing else is waiting |

For the controllers in row 2:

- **The values a note starts with and ends with are exact.** Whatever is still
  waiting for a channel is sent just before that channel's note-on and note-off.
- A return to 0 is sent at once.
- A value that has stopped changing is sent within 40 ms (pitch bend), 60 ms
  (controllers, channel pressure) or 120 ms (poly pressure), however small the
  last change was.
- With a quiet cable nothing is skipped.

A note can be delayed by a system exclusive message that is already on the cable:
3.5 ms for a parameter change, 78 ms for the Multi setup sent once on entering MPE
mode.

## From the Z1 to the computer

Everything the Z1 sends is passed to USB on its own channel, with two exceptions:

- MIDI clock from the Z1 is not passed on. Start, continue, stop and active
  sensing are.
- A system exclusive message longer than 1,024 bytes is dropped. Single program
  and Multi setup dumps fit; bank dumps do not.

A program change from the Z1 that comes straight back from the computer within
0.2 s is not sent to the Z1 again.

## System exclusive

| From the computer | Treatment |
| --- | --- |
| Korg Z1 messages (`F0 42 3g 46 …`) | Passed to the Z1. The Buddy reads them as well: a parameter change or a program dump updates its displays. |
| Buddy messages (`F0 7D 5A 31 …`) | Acted on by the Buddy; never passed to the Z1 |
| Anything else | Dropped |

Useful Korg messages, all for global channel 1 (`g` = 0):

| Message | Meaning |
| --- | --- |
| `F0 42 30 46 10 00 F7` | Request the current program; answer `40 …`, 666 bytes |
| `F0 42 30 46 19 00 F7` | Request the current Multi setup; answer `49 …`, 245 bytes |
| `F0 42 30 46 41 gg pl pm vl vm F7` | Parameter change: group (1 = program, 3 = Multi), parameter ID and value, low 7 bits first |
| `F0 42 30 46 4E 12 01 F7` | Switch the Z1 to Multi mode (not in Korg's list; found by capture) |
| `F0 42 30 46 4E 01 00 F7` | Switch the Z1 to Program mode |

## Buddy messages

`F0 7D 5A 31 <command> <data…> F7`. `7D` is the manufacturer ID that MIDI sets
aside for private use; `5A 31` is "Z1".

| Direction | Command | Meaning |
| --- | --- | --- |
| To the Buddy | `01` | Send the status |
| To the Buddy | `02 mm` | Go to mode `mm`: 0 Normal, 1 MPE |
| To the Buddy | `03 uu` | Unison: 0 off, 1 = 2 voices, 2 = 3, 3 = 6 |
| To the Buddy | `04 ww pt dt` | Set the three pots: wonkiness, portamento time, detune, 0–99; `7F` leaves one as it is. Holds until the pot itself is moved. |
| To the Buddy | `05` | Send the debug status |
| To the Buddy | `06` | Panic: all notes off |
| To the Buddy | `07 pp` | Depth of a key's bend in MPE mode, percent of true pitch, 0–127. Kept when switched off. |
| To the Buddy | `08 ss cc` | Range of the incoming key bend: `ss` semitones (1–96) and `cc` cents (0–99). Kept when switched off; 48 00 on a new Buddy. |
| From the Buddy | `11 mm uu ww bb pp pt dt` | Status; sent when asked for and when it changes |
| From the Buddy | `12 …` | Debug status |

Status bytes:

| Byte | Meaning |
| --- | --- |
| `mm` | 0 Normal, 1 entering MPE, 2 MPE |
| `uu` | Unison, 0–3 |
| `ww` | Wonkiness, 0–99 |
| `bb pp` | Bank (0 = A, 1 = B, `7F` = not known) and program last selected in Normal mode: in MPE mode, the sound that was copied to A127 |
| `pt dt` | Portamento time and detune, 0–99 |

The debug status holds queue lengths, the longest wait of a note, counters, and
the notes that are on, both as received and as sent to the Z1. Its layout is in
the firmware source at `buddySendDebug()`; `tools/buddy status` prints it in
words.
