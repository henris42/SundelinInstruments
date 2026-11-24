# Z1 Buddy: development notes

How the Z1 Buddy firmware is built and how it works inside. For how to use the
Buddy, see the [user guide](USER_GUIDE.md).

The Z1 Buddy is a hardware front end for the Korg Z1: a Teensy 4.1 with three LCDs,
18 encoders and a USB MIDI port. It sits between the computer and the Z1 and is an
editor for the Z1's program parameters, a USB–MIDI bridge, and an MPE extension that
plays the Z1 from an MPE controller.

This document describes the firmware in `firmware/` as of November 2026. It was
written by reading the code, not by running it on the bench, so anything about what
the hardware *does* is inferred from the source. Points I could not settle from the
code are collected under [Open questions](#open-questions).

## Contents

- [Folder layout](#folder-layout)
- [Building and flashing](#building-and-flashing)
- [Hardware, as the code sees it](#hardware-as-the-code-sees-it)
- [Code map](#code-map)
- [The MIDI bridge](#the-midi-bridge)
- [The MPE extension](#the-mpe-extension)
- [Unison](#unison)
- [Wonkiness](#wonkiness)
- [The editor](#the-editor)
- [Open questions](#open-questions)

## Folder layout

| Path | What it is |
| --- | --- |
| `firmware/` | Current source. This is what is flashed. |
| `firmware_old_2025/` | An older, diverging copy (char buffers instead of `std::string`). Kept for reference. |
| `firmware.zip` | Snapshot from September 2025. |
| `build_z1buddy/firmware.ino.hex` | The latest image built from `firmware/` (24 November 2026, with unison; not yet flashed when this was written). |
| `*.prt.*`, `*.stl` | Enclosure CAD and print files. |

`firmware/` came from `~/share/firmware` (February 2026) plus the changes listed
under [Sysex bridge](#sysex-bridge). `~/share/firmware` does not have those changes.

## Building and flashing

The board is a Teensy 4.1 with USB type "Serial + MIDI". One script does the work:

```
tools/buddy build      # compile; the image goes to build_z1buddy/firmware.ino.hex
tools/buddy flash      # compile, then upload to the Z1 Buddy
tools/buddy monitor    # show the debug output with times, reconnecting after a restart
tools/buddy go         # flash, then monitor
tools/buddy port       # show which connected Teensy is the Z1 Buddy
tools/buddy status     # ask the running Buddy for its debug status
tools/buddy panic      # all notes off
tools/buddy bend 60    # MPE mode: key bend depth, percent of true pitch
```

- **The Buddy is found by its USB serial number** (13124960), never by its USB
  location. Several Teensys are usually connected and their locations change: on
  23 November the Buddy was at `usb:14420000`; a day later another Teensy 4.1 sat
  there and the Buddy was at `usb:14500000`. Do not upload with a location typed
  from memory.
- **The monitor** reads the Buddy's USB serial port, puts the time in front of every
  line and appends everything to `build_z1buddy/monitor.log`. It keeps running
  while the Buddy restarts, so it can stay open in a second terminal during
  `tools/buddy flash`. Stop it with Ctrl-C.
- **CPU speed** is set to 720 MHz, a mild overclock (`speed=720` in `tools/buddy`).
- **Optimize: "Faster" (`opt=o2std`)**, the Teensy's normal setting. Until the
  refactor of 24 November 2026 only "Smallest Code" fitted: the editor's use of
  `std::string` and `std::stringstream` made the program overflow the fast RAM.
- **Core version: 1.62.0** (GCC 15, still built as C++17: `constexpr` yes,
  `consteval` no). Core 1.59.0 (GCC 11) builds it as well.

  | Build | Code in flash | Code in fast RAM |
  | --- | --- | --- |
  | Before the refactor, core 1.59, Smallest Code (the only one that fitted) | 307 KB | 212 KB |
  | After, core 1.59, Smallest Code | 88 KB | 68 KB |
  | After, core 1.62, Faster (the default now) | 123 KB | 101 KB |
- **Library.** `teensy4_i2c` by Richard Gemmell, from GitHub; it is not in the
  Arduino library manager. `Bounce` and `LiquidCrystal` come with the Teensy core.
- **USB name.** `name.c` sets the names the computer shows ("SundelinInstruments",
  "Z1 Buddy"). Live identifies the port by this name.
- Settings (serial number, board options, path of `arduino-cli`) are at the top of
  `tools/buddy` and can be overridden from the environment.

What the script runs, should it be needed by hand:

```
~/bin/arduino-cli compile -b teensy:avr:teensy41:usb=serialmidi,opt=osstd --build-path build_z1buddy/work firmware
~/bin/arduino-cli upload  -b teensy:avr:teensy41:usb=serialmidi,opt=osstd -p <port from "tools/buddy port"> --protocol teensy --input-dir build_z1buddy/work
```

## Hardware, as the code sees it

| Part | Connection |
| --- | --- |
| Z1 MIDI in/out | `Serial1` at 31250 baud, 32-byte extra buffers each way |
| 3 LCDs, 40×2 | `LiquidCrystal`, shared data pins 36, 37, 32, 9 and RS 6; enable pins 10, 12, 11 |
| 18 encoders with push buttons | Three MCP23017 boards on I2C (0x23, 0x21, 0x25), interrupt pins 39, 38, 27 |
| 16 LEDs | MCP23017 at 0x20, both ports as outputs |
| Page buttons | Four lines on pins 7, 8, 28, 29, read as a 4-bit code |
| Buttons S10, S11 | Pins 34, 35 |
| 3 pots | A6, A7, A8, scaled to 0–99 |
| I2C | 1 MHz |

Each LCD shows six parameters: name on the top line, value below. Display position
0–17 maps to encoder 0–17. Three encoders on the second board (6, 7, 8) have their
pins swapped in code, marked `//flipped`.

### Buttons

What the controls mean to a player is in the [user guide](USER_GUIDE.md); this is
how the code reads them.

| Control | Action |
| --- | --- |
| Page code 9 / 8 / 10 | Oscillator 1 / Oscillator 2 / Sub oscillator |
| Page code 7 / 14 / 13 / 12 | Filter / Effects / Envelopes / LFOs |
| Page code 6 | Next unison setting: off, 2, 3, 6 voices (see [Unison](#unison)) |
| Page code 11 | Main mode: Normal ↔ MPE |
| S10 | Dump mode: request the current program, then the arpeggio pattern, and pass both to the computer without parsing them |
| S11 | Toggle the "Mod" variant of the current page |

Pressing a page button again steps through that section's sub-pages.

### LEDs

| LED | Meaning |
| --- | --- |
| 0, 1 | Main mode: Normal, MPE (LED 2 is free since MPE+ was parked) |
| 3, 4, 5 | Oscillator 1, Oscillator 2 (both lit: Sub oscillator), Filter |
| 6, 7, 8 | Unison 2, 3, 6 |
| 9, 10, 12 | Effects, Envelopes, LFOs |
| 11 | Mod page active |

### Pots

| Pot | Action |
| --- | --- |
| A6 | Wonkiness, 0–99 (see [Wonkiness](#wonkiness)) |

A pot that moves, or is set from the computer, is shown for 1.5 s in the last slot
of the last display (position 17), which pages rarely use: its name (Wonk, Porta,
Detune) and value. Only that slot is written; afterwards it gets back what the page
had there (`potShow()`, `potShowTick()`).
| A7 | Sends CC 5 (portamento time) on channel 1 |
| A8 | Unison detune, 0–99 cents (program parameter 27) |

## Code map

| File | Lines | Contents |
| --- | --- | --- |
| `firmware.ino` | 2,100 | Setup and main loop, MIDI bridge in both directions, sysex handling, the output queue and MPE scheduler, MPE unison, wonkiness, pots, Buddy sysex, button and encoder scanning |
| `z1_params.h` | 1,270 | Every parameter the editor knows, as one constant table, and the lists of names |
| `Z1.cpp`, `Z1.h` | 3,500 | `Z1Edit`: the display pages, Korg sysex send and parse, main-mode and unison switching |
| `Editable.cpp`, `.h` | 280 | `EditableValue`: the current value of one parameter and how it is shown; `EditableSet`: all of them, found by key |
| `SynthBuddy.cpp`, `.h` | 290 | `SynthBuddy`: LCD layout, LEDs, encoder decoding (state table), MCP23017 access |
| `mpe_multi.h` | — | The Multi setup for MPE mode, generated by `tools/pull_multi.sh` |
| `name.c` | 30 | USB manufacturer and product strings |
| `EventManager.h`, `._cpp` | — | Not compiled (the include is commented out) |

`Z1Edit` and `SynthBuddy` are singletons (`getInstance()`). All parameters are
reached through the global `editables`, by the Z1's parameter ID:
`editables[263]->set(60)`.

The main loop has no blocking waits apart from serial writes:

1. Service the three encoder boards when their interrupt flag is set.
2. Check S10, S11 and the page buttons (150 ms debounce).
3. Read the pots and send on change.
4. `processDIN()`: parse what the Z1 sent.
5. `processUSB()`: parse what the computer sent, then run the MPE scheduler.

Debug output goes to the USB serial port at 115200 baud (`USE_DEBUG 1`).

## The MIDI bridge

### Z1 to computer

`processDIN()` is a byte parser with running status. Every channel message is
forwarded to USB with its channel unchanged.

- **Clock is not forwarded.** The line for `0xF8` is commented out. Start, Continue,
  Stop and Active Sensing are forwarded.
- **Program changes are remembered** for 200 ms so the same program change coming
  back from the computer is not sent to the Z1 again (anti-loop).
- **Sysex** is collected up to 1,024 bytes and handed to `onSysEx()` without the
  `F0` and `F7`. Longer messages are discarded.

### Computer to Z1

What happens depends on the main mode.

**Normal mode.** Every message is forced to channel 1.

- Notes and program changes go to the Z1 at once.
- Bank select (CC 0 and CC 32) goes at once, so it arrives before the program
  change that follows it.
- Other controllers, poly pressure and channel pressure go through the
  [scheduler](#the-scheduler).
- Pitch bend goes through the [scheduler](#the-scheduler) on channel 1. (Before
  24 November 2026 it was never sent in this mode: a bend on channel 1 was filed
  as the MPE global offset.)

**MPE modes.** Channels are kept. See [The MPE extension](#the-mpe-extension).

Real-time messages are forwarded at once in both modes.

### Sysex bridge

Added on 23 November 2026 so that Patch Sync in Live can read and write Z1
programs through the Buddy:

- **Computer to Z1:** Korg Z1 sysex (`F0 42 3g 46 …`) is written through to the
  Z1. USB delivers long messages in 290-byte chunks; the first chunk decides for
  the whole message. Other manufacturers' sysex is not forwarded.
- **Z1 to computer:** every sysex message from the Z1 is also sent to USB. Before,
  this only happened in dump mode.
- Sending a full 666-byte dump to the Z1 blocks the main loop for about 0.2 s.
- **The Buddy reads what it forwards** (since 24 November 2026). A parameter
  change or a program dump sent from the computer to the Z1 is also applied to the
  editor, so the displays, and everything that works from the editor's values such
  as the wonkiness drift, stay in step. The chunks are collected into one message
  first and handed to `onSysEx()` without `F0`/`F7`, exactly like the Z1's own
  messages. Only program parameters (group 1) are taken into the editor.

### Buddy sysex

Short messages between the computer and the Buddy itself, added on 24 November
2026 so that software can switch the Buddy's mode and follow it. They are never
passed on to the Z1. They use manufacturer ID `7Dh`, which MIDI sets aside for
private use, followed by "Z1" (`5Ah 31h`):

```
F0 7D 5A 31 <command> <data...> F7
```

| Direction | Command | Meaning |
| --- | --- | --- |
| To the Buddy | `01` | Send the status |
| To the Buddy | `02 mm` | Go to main mode `mm`: 0 Normal, 1 MPE |
| To the Buddy | `03 uu` | Set unison `uu`: 0 off, 1 = 2 voices, 2 = 3, 3 = 6 (the program's unison in Normal mode, the stacking in MPE mode) |
| To the Buddy | `04 ww pt dt` | Set the three pots: wonkiness, portamento time, detune, 0–99 each; `7F` leaves one as it is |
| To the Buddy | `05` | Send the debug status |
| To the Buddy | `06` | Panic: note-off for every note the Buddy has on for the Z1, then All Notes Off on all channels |
| To the Buddy | `07 pp` | Depth of a key's own bend in MPE mode, percent of true pitch, 0–127. 100 after power-up. |
| From the Buddy | `11 mm uu ww bb pp pt dt` | Status |
| From the Buddy | `12 …` | Debug status, see below |

The status bytes:

| Byte | Meaning |
| --- | --- |
| `mm` | 0 Normal, 1 entering MPE (waiting for the Z1), 2 MPE |
| `uu` | Unison shown on the LEDs, 0–3 |
| `ww` | Wonkiness, 0–99: the value in effect, from the pot or as set |
| `pt dt` | Portamento time and detune, 0–99 (the last two bytes of the message) |
| `bb pp` | Bank (0 = A, 1 = B, `7F` = not known yet) and program (0–127) last selected in Normal mode. In MPE mode this is the sound that was copied to the working slot A127. |

- The status is sent when asked for and whenever mode, unison, bank or program
  changes. A change of a pot alone is sent after 0.3 s, so turning one does not
  send a stream of messages.
- **Pots set from the computer.** A pot can not be moved by software, so the Buddy
  keeps a value in effect for each pot (`potValue`). It follows the pot, but command
  `04` can set it. A value set this way holds until the pot is moved at least 3
  steps (`POT_TAKEOVER`) from where it stood at that moment; smaller movement and
  noise are ignored, then the pot takes over again.
- Bank and program are taken from the bank select (CC 32) and program change
  messages that pass through the Buddy in Normal mode, in either direction. Until
  a bank select has passed, the bank is reported as not known.
- Entering MPE from the computer runs the same steps as the button, so the status
  goes 0 → 1 → 2 as the Z1 answers.

### Debug status

For finding where a note or a message went. `tools/buddy status` asks for it and
prints it in words; `tools/buddy panic` sends the panic message.

```
mode Normal, unison off, wonkiness 16, portamento 0, detune 0, last patch A 12, up 12 min 34 s
waiting now:     0 events, 0 sysex bytes, 2 background changes
since last report: most events waiting 7, longest wait 4 ms, longest loop 12 ms
notes from the computer: 1523 on, 1522 off      sent to the Z1: 1523 on, 1522 off
expression messages sent: 48211      sysex messages sent: 9120 (0 closed unfinished)
on, as received from the computer: D4 (62) on channel 3
on, as sent to the Z1:             none
```

The two lists at the end are what settles a hanging note:

| The hanging note is… | The note-off was lost… |
| --- | --- |
| still in the first list | before the Buddy: the computer never sent it (Live, a device in the chain, the controller) |
| in the second list only | in the Buddy |
| in neither list | after the Buddy: it was written to the cable and the Z1 did not take it |

The second list is counted at the moment a note message is written to the port,
not when it is queued. "Longest wait" is the longest any note or other event sat
in the output queue, and with "longest loop" it is reset by each report. The byte
layout is at `buddySendDebug()` in `firmware.ino`.

Code: the block marked `Buddy sysex` in `firmware.ino`; `Z1Edit::gotoMainMode()`,
`enterMPE()`, `leaveMPE()` and `setUnisonLevel()`.

The Z1 Patch Sync device in Live uses these messages to make the state of the Z1
and the Buddy part of a Live set:

- It has an MPE button, which is a Live parameter and so saved with the set.
- It stores unison and the three pots in a hidden parameter whenever the Buddy
  reports a change.
- When a set is opened it sends them back in order: bank and patch, then the pots,
  then MPE mode if the set had it on, then unison once the Buddy is in that mode.
- In MPE mode it shows "MPE: name" for the sound the working slot was made from,
  ignores the program changes that belong to the MPE setup, and refuses patch
  changes and saves.

The sound itself is only brought back if it was stored in the set with Get + Save
and is sent with Send Saved; that step is still by hand.

### What the Buddy does with Z1 sysex

`onSysEx()` decides by message length (without `F0`/`F7`):

| Length | Taken as | Action |
| --- | --- | --- |
| 664 | Current program dump | `parseZ1SysExPreset()`: unpack 7-bit data, update all parameters and the displays |
| 9 | Parameter change | `parseZ1SysExParam()`: update that one parameter |
| 4 | Status reply | If it is "write completed" (`21h`) while entering MPE, move to the next step |
| 298 | Arpeggio pattern dump | While entering MPE, finishes the switch; in dump mode, ends it |

At start-up the Buddy requests the current program (`10h`), so the displays match
the Z1.

## The MPE extension

The Z1 is not an MPE synth. The Buddy makes it behave like one by sending each MPE
note channel to its own MIDI channel on the Z1, and by thinning the per-note data so
it fits a DIN cable.

### Channel layout

- Channel 1 is the MPE global channel.
- Channels 2 to `MPE_MAX_CHANNEL` (7) are note channels: six voices.
- Notes are forwarded on their own channel, immediately.

The Z1 has no idea what MPE is. The design is a multi-timbral performance on the
Z1 with six timbres, all playing the same program, one per note channel. That
shared program is A127 (see below).

### The Multi setup

The Buddy carries that Multi setup with it, as one sysex message ("current multi
setup data dump", function `49h`) compiled into the firmware from
`firmware/mpe_multi.h`, and sends it to the Z1 when MPE mode is entered.

The header is made by an external tool, not by hand:

```
tools/pull_multi.sh --default   # build it: six timbres, channels 2-7, program A127
tools/pull_multi.sh             # or read the Multi setup now selected on the Z1
```

Both write `tools/mpe_multi.syx` and turn it into the header with `xxd -i`, which
gives the array `mpe_multi_syx` and its length `mpe_multi_syx_len`. Rebuild and
flash afterwards.

| Tool | What it does |
| --- | --- |
| `tools/pull_multi.sh` | The whole step: get the setup, print what is in it, write the header |
| `tools/z1sysex.c` | Sends a sysex request to a MIDI port and saves the reply (macOS, CoreMIDI); built on first use |
| `tools/multi_info.py` | Prints the name and the six timbres of a Multi setup dump; with `--default`, builds the MPE setup |
| `tools/check_params.py` | Compares the parameter table with the Korg list |

The default setup is built from a Multi setup read from the Z1 ("Techno Tonic"),
so every field the tool does not set has a valid value. It sets, per timbre:
program A127, MIDI channel 2–7, reserve voice 2, no transpose or detune, level 127,
full key and velocity zones, program change off, pan and effect send as in the
program. The controller filters keep the values of the base setup.

Effects in a Multi setup are shared by all six timbres. The default sets effect 1
to a **multitap delay** timed for 136 BPM (taps at an eighth and a dotted eighth,
221 and 331 ms, 22 % wet), and turns effect 2 and the master effect fully dry.
The tempo and the delay settings are at the top of `set_effects()` in
`multi_info.py`. Two things are assumed there: that the delay time value 0–251 is
an even scale over 0–680 ms, and that the setting bytes of an effect are its
parameters in the order of the Korg list (this fits the setups read from the Z1).

For anything finer, set the effects up by ear on the Z1 in Multi mode and run
`tools/pull_multi.sh` without `--default`: the header then carries exactly that.

The header in the repository today is the built default. It has not been loaded
into a Z1 yet.

### Entering MPE

The main-mode button switches between Normal and MPE. Entering MPE is a short
conversation with the Z1, tracked in `mainState`:

| State | What happens |
| --- | --- |
| `NORMAL` | Button pressed (or Buddy sysex `02 01`): send "program write request" (`11h`) to store the current sound in program **A127**. LED 1 on. |
| `MPEPRE` | Wait for "write completed" (`21h`), then tell the Z1 to go to Multi mode. |
| `MPEPRE2` | Wait for the 298-byte arpeggio pattern dump, which the Z1 sends whenever its mode has changed. Then send the Multi setup from `mpe_multi.h`. |
| `MPEPRE3` | Wait for "data load completed" (`23h`) or "data load error" (`24h`). Either way, send parameter change ID 21 = 0 (voice assign mode: mono) and go on. The error is printed on the serial monitor. |
| `MPE` | MPE routing active. |

Pressing the main mode button while the Buddy is waiting for the Z1 gives up and
goes back to Normal.

Leaving MPE sets the timbre detunes back to 0, tells the Z1 to go back to Program
mode, and, when the Z1's arpeggio dump shows it has done so, asks for the current
program so the displays are right again.

### Switching the Z1's mode

The Korg list has no message for this, but the Z1 sends one itself when the mode is
changed on its panel, and obeys it when it is sent back (captured and tried on 24
November 2026):

| Message | Effect |
| --- | --- |
| `F0 42 30 46 4E 12 01 F7` | Multi mode |
| `F0 42 30 46 4E 01 00 F7` | Program mode |

After either, the Z1 sends its arpeggio settings as NRPNs and the current arpeggio
pattern dump (`6Bh`, 298 bytes). What the two data bytes mean beyond these two
cases is not known. Code: `Z1Edit::setZ1Mode()`.

So the sound being edited is copied to A127, the program all six timbres play,
and its voice assign mode is set to mono (program parameter 21, value 0 =
MONO_MULTI), since each timbre carries one note.

Program A127 is overwritten every time MPE is entered. This is by design: A127 is
the Buddy's working slot for MPE.

MPE+ was a third mode meant for Osmose-specific behaviour. It is parked: the
state still exists in the code but nothing leads to it.

### What each message does in MPE mode

| From the controller | On channel 1 (global) | On a note channel |
| --- | --- | --- |
| Note on/off | Forwarded at once | Forwarded at once |
| Pitch bend | Stored as a global offset, not sent | Added to the global offset, clamped, queued |
| CC 74 | Renumbered, then copied to channels 2–7 and queued | Renumbered and queued |
| Other CC | Copied to channels 2–7 and queued | Queued |
| Channel pressure | Queued on channel 1 | Queued |
| Poly pressure | Queued | Queued |
| Program change | Forwarded at once (with the anti-loop check) | Forwarded at once |

**CC 74 renumbering.** The Z1 has no fixed "timbre" controller, so CC 74 is sent as
the controller number held in the Buddy setting `C74To` (key 10001, default 7,
which is Volume on the Z1). This is the Buddy's own equivalent of the MPE remapper
in Live.

**Per-note pitch bend** arrives in MPE scale, where full travel means 48
semitones (`MPE_BEND_SEMITONES`). The Z1 plays a bend in the scale of the sound,
whose range is program parameters 163 (up) and 164 (down), usually 2 semitones.
The Buddy multiplies the bend by 48 ÷ range (`mpeScaleBend()`), so a key bent a
semitone sounds a semitone higher whatever the sound's range, and a small range
gets the Z1's full bend resolution. What does not fit the range is held at its
end. The range is read from the program dump (bytes 145 and 146) and follows
parameter changes; a sound whose range is 0 or reversed is left unscaled.

How much of that true pitch a key gets is `mpeBendPercent`, 100 by default. It
can be changed while playing with `tools/buddy bend <percent>` (Buddy sysex 07)
to find what feels right on a given controller; the value is not kept over a
power cycle, so the one that works should become the default in the code.

**Global pitch bend.** A bend on channel 1 is added, unscaled, to every note
channel's bend, and all six are sent again at once: a pitch wheel moves every
sounding note by the sound's own range.

### The scheduler

A DIN cable carries about 3,125 bytes per second, roughly 1,000 three-byte
messages. An MPE controller playing a chord sends far more than that. Forwarding
everything would queue up and make the Z1 lag behind the hands, so
`drainUSBCoalescedToDIN()` sends only what matters most, every pass of the main
loop.

**Coalescing.** Incoming values are not queued. For each stream the Buddy keeps
only the latest value and a "dirty" flag:

- pitch bend per channel
- channel pressure per channel
- each controller per channel
- poly pressure per note per channel

A hundred pressure values arriving between two passes cost one slot, and the newest
wins.

**Byte budget.** Each pass may send as many bytes as the cable could have carried
since the last pass (elapsed ms × 3.125), limited by what the [output queue](#the-output-queue)
lets into the Teensy's send buffer. This is what keeps the link from backing up.

**Notes first.** For 2 ms after any note on or off the scheduler sends nothing, so
note messages are never stuck behind expression data.

**Deadband.** A stream is worth sending when it has moved far enough from the value
last sent:

| Stream | Base deadband | Refresh after | Minimum gap between sends of this type |
| --- | --- | --- | --- |
| Pitch bend (14-bit) | 64 | 40 ms | 2 ms |
| Controller | 4 | 60 ms | 5 ms |
| Poly pressure | 5 | 120 ms | 6 ms |
| Channel pressure | 5 | 60 ms | 6 ms |

- The deadband shrinks when the link has spare capacity and as the last send gets
  older, down to 1. With a quiet link everything goes through at full resolution.
- A value of 0 is always sent if the last one was not 0, so releases land exactly.
- A stream that differs by less than the deadband is still sent once its refresh
  time has passed, so nothing stays slightly wrong for long.

**Priority.** Candidates get a score: type weight × (error ÷ deadband) × an age
boost. Weights are pitch bend 4, controller 1.0, poly pressure 0.9, channel
pressure 0.8, so pitch wins clearly when there is a choice. The best 32 are kept, sorted.

**Fairness between voices.** The byte budget is split between channels in
proportion to their total error, with a small guaranteed share for every channel
that has anything to send. One busy voice can not starve the others.

Then the candidates are sent in score order while both the total budget and their
channel's share allow it.

All the numbers are `#define`s at the top of `firmware.ino`.

### The output queue

Everything the Buddy sends to the Z1 shares one DIN cable, 3,125 bytes a second.
Since 24 November 2026 nothing writes to the port directly: it all goes through
`txPump()` in `firmware.ino`, which decides on every pass of the main loop what the
cable carries next.

| Priority | Traffic | How it is treated |
| --- | --- | --- |
| Always | MIDI clock, start, stop (realtime) | Straight to the port. One byte, may go anywhere, even inside a sysex message. |
| 1 | Notes, program change, bank select, pedal | A queue in order of arrival. Never dropped, never thinned. `txEvent()` |
| 2 | Per-note bend, controllers, pressure | The scheduler below: newest value of each stream only, pitch bend first, thinned to what fits. |
| 3 | Sysex: editor changes, mode changes, dumps | A queue in order. Never dropped; each message in one piece. Goes when expression leaves the cable free, and at least every 20 ms. `txSysEx()` |
| 4 | Wonkiness drift, timbre detune | Newest value per parameter only; goes when nothing else waits, and at least every 100 ms. `bgSend()` |

Points that matter:

- **The Teensy's own send buffer is kept nearly empty** (16 bytes, 5 ms;
  `TX_WATERMARK`). What waits, waits in the queues above, where a note can still
  go in front of it. Without this a note would queue behind up to 96 bytes of older
  data, 30 ms.
- **Start and end values are exact.** Just before a note-on or a note-off goes out,
  every value still waiting for that channel (bend, pressure, controllers) is sent
  as it is, ahead of the note (`txFlushExpression()`). So a note starts at the right
  pitch and filter position and ends on its final values. Values that change in
  the release are thinned like the rest; the scheduler always delivers the last
  one, and a return to 0 at once.
- **Pitch comes first** among the expression: its weight in the scheduler is 4,
  against 1 for controllers and 0.8–0.9 for pressure.
- **The one thing a note can not overtake is a sysex message that has started.**
  A parameter change is 11 bytes, 3.5 ms. The Multi setup sent on entering MPE is
  245 bytes, 78 ms, once.
- **A sysex message from the computer arrives in pieces.** If its end never comes,
  the message is closed after 0.3 s so the notes are not held up.
- **The queues grow as needed** (`std::deque`, `std::vector`); nothing is dropped
  or made to wait for want of space.

History: before this, the write routine for notes returned early when the
Teensy's buffer was full, and a note-off arriving behind a burst of wonkiness
messages was lost (a stuck note).

`bgFlush()` moves the background queue into the sysex queue at once, for the
moments where order matters: before the Z1 is told to change mode, and when unison
is switched.

`tools/hosttest/run.sh` runs this queue against a simulated cable, together with
the unison and wonkiness logic. In that test a note arriving while 300 expression
messages wait is on the cable 5 ms later, and ten editor changes get through in
0.4 s while expression takes the rest.

### MPE hooks

`onMPE_NoteOn`, `onMPE_NoteOff`, `onMPE_PitchBend`, `onMPE_CC`,
`onMPE_ChannelPressure` and `onMPE_PolyPressure` are called for every incoming
message. They are stubs today (the note hooks print to the serial monitor) and are
the place for voice allocation or per-note logic.

## Unison

Added on 24 November 2026. It compiles, and the MPE stacking logic passes a test on
the Mac with stand-ins for the hardware, but nothing here has been tried on the Z1.

The unison button and its three LEDs (2, 3, 6 voices) mean different things in the
two modes.

### Normal mode: the program's own unison

Unison is part of the Z1 program, so it is switched with program parameter changes
like any other edit:

| Parameter | Meaning | Sent by |
| --- | --- | --- |
| 24 | Unison type: 0 off, 1 = 2 voices, 2 = 3, 3 = 6 | Unison button |
| 25 | Unison switch: 0 off, 1 on | Unison button (on unless the type is off) |
| 27 | Unison detune, 0–99 cents | Pot A8 |

- The button steps off → 2 → 3 → 6 → off (`Z1Edit::setUnison()`).
- When a program dump arrives, the Buddy reads byte 21 of the program (type in
  bits 0–1, switch in bit 2) and sets its state and LEDs to match
  (`unisonFromPatch()`), so it follows the sound that is actually loaded.
- Unison mode (parameter 26, fixed or dynamic) has no control.

### MPE mode: stacking timbres

In MPE mode each of the six timbres is one mono voice, so unison is done by the
Buddy: one note is sent to 2, 3 or 6 timbres at once, and the timbres are detuned
against each other in the Z1's Multi setup.

| Button setting | Timbres per note | Notes at once | Output channels per note |
| --- | --- | --- | --- |
| Off | 1 | 6 | Same as the input channel (unchanged behaviour) |
| 2 | 2 | 3 | 2–3, 4–5, 6–7 |
| 3 | 3 | 2 | 2–4, 5–7 |
| 6 | 6 | 1 | 2–7 |

**Groups.** The six output channels are cut into groups of that size. An input
channel (one MPE note) is given a group when its first message arrives, so the
bend, CC 74 and pressure sent just before a note-on already go to the right
timbres. It keeps the group after the note ends, for release tails, until another
note needs it.

**Any input channel works.** With unison on, the controller may use any of
channels 2–16; they are mapped onto the groups. With unison off, the input channel
is used as it is, so the controller must stay on 2–7.

**Stealing.** A new note takes a group with no held note, the one unused longest.
If every group is busy, the oldest note is cut off and its group reused. The
late note-off of the cut note is ignored.

**Expression.** Per-note bend, controllers and pressure are copied to every timbre
of the group and go through the [scheduler](#the-scheduler) as before. Messages on
channel 1 behave as without unison. Notes are written without the scheduler and
wait for room in the buffer, so a stacked note-on or note-off is never dropped.

**Detune.** Pot A8 sets the spread, 0–50 cents. Within a group the timbres are
spread evenly from −spread to +spread, sent as the Detune of each timbre: Multi
parameter 21 for timbre 1 and 27 higher for each next timbre (group 3 of the Korg
parameter change).

| Timbres per note | Detune of the timbres, for a spread of d |
| --- | --- |
| 2 | −d, +d |
| 3 | −d, 0, +d |
| 6 | −d, −0.6d, −0.2d, +0.2d, +0.6d, +d |

**Changing the setting** releases all held notes, sends All Notes Off on channels
2–7 and sends the six detunes again. Leaving MPE mode does the same and sets the
detunes back to 0.

The Z1 program's own unison is not touched in MPE mode.

Code: the block marked `MPE unison` in `firmware.ino` (`mpeGroupFor()`,
`mpeUnisonNoteOn()`, `mpeSendDetune()`, `mpeUnisonChanged()`), and
`Z1Edit::nextUnisonMode()`.

To be checked on the hardware:

- That the Z1 applies a Multi Detune parameter change to a sounding Multi setup.
- That negative values are accepted in the form used here (14-bit two's
  complement, as for the program parameters).
- Six parameter changes are 66 bytes, about 20 ms of DIN. Turning the pot sends
  them for every step of the spread.

## Wonkiness

Added on 24 November 2026. It compiles and its logic passes a test on the Mac with
stand-ins for the hardware; it has not been tried on the Z1.

Pot A6 makes the current sound behave like an old analog synth: nothing is quite in
tune or quite where it was set, and it keeps moving a little. It works in Normal
and MPE mode. At 0 it does nothing and everything it moved goes back.

Everything is sent as Korg parameter changes (sysex function `41h`), the numbered
parameters the editor uses. The Z1's NRPNs are not used: the Korg list has NRPNs
for the arpeggiator only.

There are three layers:

| Layer | What it does | How |
| --- | --- | --- |
| Per note | Each note starts slightly off pitch | Program parameter 30, Random Pitch Intensity; the Z1 does the scattering |
| Per voice (MPE mode) | Each of the six timbres is permanently a little off, by its own amount | Multi Detune of each timbre, added to the unison spread; up to ±12 cents |
| Drift | A set of parameters wanders slowly around the patch's values | Program parameter changes, three parameters every 150 ms |

### What drifts

Always, for every patch:

| Parameter | ID | Drift at full pot |
| --- | --- | --- |
| OSC1 fine tune | 177 | ±6 cents |
| OSC2 fine tune | 191 | ±6 cents |
| Sub oscillator fine tune | 204 | ±4 cents |
| Filter 1 cutoff | 263 | ±8 steps |
| Filter 2 cutoff | 290 | ±8 steps |

Then, for each of the two oscillators, the parameters that suit its type (the
type is read from parameter 174 for OSC1 and 188 for OSC2):

| Oscillator type | Drifting parameters (ID, drift at full pot) |
| --- | --- |
| Standard | Wave edge (465, ±5), wave form (470, ±4) |
| Comb filter | Feedback (492, ±3), high damp (497, ±5) |
| VPM | Modulator level (520, ±4), modulator fine frequency (514, ±3) |
| Resonance | Fine frequency of the four bands (535, 541, 547, 553, ±4) |
| Ring mod | Modulation depth (559, ±4), wave edge (565, ±4) |
| Cross mod | Modulation depth (568, ±3), wave edge (573, ±4) |
| Sync | Wave edge (576, ±5) |
| Organ | Fine tune of the three drawbars (579, 586, 593, ±3) |
| E. piano | Hammer force (602, ±5), overtone level (608, ±4), pickup position (611, ±3) |
| Brass | Lip character (626, ±5), noise level (634, ±4), strength (648, ±4) |
| Reed | Noise level (668, ±5), wave shape (691, ±4) |
| Plucked string | String position (702, ±5), damping (708, ±4), attack noise (700, ±5), pickup position (719, ±4) |
| Bowed string | Rosin (736, ±5), string position (737, ±4), damping (740, ±3) |

The choice of parameters and depths is a first guess, to be tuned by ear. It is one
table, `WONK_BY_TYPE` in `firmware.ino`, with up to four parameters per type;
`WONK_COMMON` holds the ones for every patch. Only oscillator and filter parameters
are used, because the editor knows those completely; effect parameters are not all
defined in the firmware.

### How the drift works

- Each parameter has its own random walk, pulled gently back to the centre. The pot
  scales how far it may go.
- The centre is the patch's own value, taken from the editor. Edit a parameter on
  the Buddy or on the Z1 and the drift follows the new value.
- Three parameters are moved every 150 ms, in turn, and only when the rounded value
  changes. In the test this came to about 150 bytes per second, 5 % of the DIN link.
- If an oscillator type is changed in the editor, the list is rebuilt for the new
  type.

### Keeping the patch's own values safe

The Buddy learns the patch's values from program dumps, so a dump must never
contain drifted values, and nothing may be sent while the editor's values are
stale. Three rules take care of that:

- **Program change** (from the Z1 or the computer): the drift stops at once. If no
  dump has arrived after 400 ms, the Buddy asks the Z1 for one.
- **Dump request** (from the computer, or button S10): every drifted parameter is
  first put back to the patch value, then the drift stops.
- **Dump arrives:** the list is rebuilt for the new patch and the drift starts again.

Limits to know:

- Random Pitch Intensity is not in the editor, so the Buddy treats the patch's own
  value as 0. With the pot up it is set by the pot; back at 0 it is set to 0.
- A sound stored on the Z1 while the pot is up is stored with the drifted values.
- The per-voice offsets in MPE mode are rolled once at power-up.
- Whether program parameter changes reach all six timbres in MPE mode has to be
  tried; see [Open questions](#open-questions).

## The editor

### The parameter table

What the editor knows about the Z1 is one constant table, `Z1_PARAMS` in
`z1_params.h`: 1,057 parameters, one line each.

```
{ key, name on the display, lowest, highest, start value, type, list of names }
{ 263, "Cutoff", 0, 99, 0, editableType::Integer, nullptr },
```

- **Key:** the Korg parameter ID, with the ExID of the type-dependent blocks in
  the bits above it: `0x0800 +` OSC1, `0x1000 +` OSC2, `0x1800 +` effect 1,
  `0x2000 +` effect 2, `0x2800 +` master effect. Key 10001 (`C74To`) is the
  Buddy's own.
- **Lists:** a parameter chosen from names (`TextEnum`) points to its list, such as
  `z1OscTypes` or `z1ModSrc1`; the lists are constants in the same file.
- **Checked by the compiler:** a `static_assert` refuses a table that is not sorted
  by key, has a key twice, or has a list parameter reaching outside its list.
- **Lookup** is a binary search (`z1ParamIndex()`), usable at compile time too.

The table replaced 784 hand-written `new EditableValue(...)` lines, with loops
around many of them. It was not retyped: those lines were run on the Mac and what
they built was written out, and the result compared again with the original. One
definition appeared twice in the old code (plucked string, harmonics position), an
exact repeat.

`tools/check_params.py` compares the table with the Korg parameter list. It
currently reports 78 differences, all there before the refactor and left as they
were: 63 effect modulation sources that use the 49-entry source list where Korg
gives the 46-entry one, and 15 list parameters in the oscillators and filters
whose range is not the Korg one (filter type 0–5 for 1–5, input select 0–5 for
0–4, wave form LFO 0–3 for 6–9, and so on). Some may be deliberate, with the code
adding an offset; each needs a look.

### Values and display

`EditableSet::init()` makes one `EditableValue` per table line, on the heap. An
`EditableValue` holds only what changes: the current value, where it is shown,
whether it is enabled, and optionally a formatter and a change callback.

A page function such as `showFilt()` clears the displays and places up to 18
parameters on the encoders with `editables[id]->show(position)`.

- **Turning an encoder** changes the value, redraws it and calls the default
  change handler, which sends a Korg parameter change (`41h`, group 1 = program)
  for any key below 10000. Note that the master effect keys (`0x2800 +`, from
  10240) are above that, so master effect edits are not sent.
- **Changes made on the Z1** come back as 9-byte parameter changes and update the
  same `EditableValue`, so the Buddy follows the Z1's own knobs.
- **A program change on the Z1** needs a fresh dump to refresh everything. With
  "SysEx Transmit" on, the Z1 sends one by itself.
- **Oscillator and effect pages** depend on the selected type: there is a page
  function per oscillator model (Standard, Comb, VPM, …, Bowed) and per effect.
- **Mod pages** (S11) show the modulation routing for the same section.
- **Formatters** (`z1ScaleFunc`, `z1ExpFunc`) write a value as text where the plain
  number is not what should be shown, a frequency for example. They and the
  centring of text in a display field use `snprintf` into small buffers; there are
  no strings or string streams left in the firmware. `tools/hosttest` compares
  their output with the old code's, kept there as a reference.

## Open questions

Things the code does not explain, or that look unintended. Each needs a check on
the hardware or an answer from the author.

1. **Does the Z1 take the Multi setup?** The Buddy now switches the Z1 to Multi
   mode itself and then sends the setup. The mode switch was tried and works; the
   setup itself has not been loaded into a Z1 yet.
2. **Eight Multi slots answer "data format error"** when read one at a time through
   the Buddy (the 2nd, 6th, 10th and 15th of each bank); the other 24 read fine and
   hold what look like factory setups. Cause unknown.
3. **Leaving MPE.** Going back to Normal changes the Buddy's state only. Nothing
   restores the Z1's voice assign mode or leaves the Multi setup.
4. **Dump mode** (S10) ends when the arpeggio dump the Buddy asks for arrives. If
   that 298-byte message is lost, dump mode stays on and incoming programs are no
   longer parsed until restart.
5. **Hard-coded Korg channel.** All requests use global channel 1 (`30h`).
6. **Do program parameter changes reach the MPE timbres?** In MPE mode the six
   timbres play A127. The drift and the editor both send program parameter changes;
   whether the Z1 applies them to what the timbres are playing is not known here.
7. **The CC 74 target can not be changed.** The setting `C74To` (key 10001) exists
   but is not placed on any display page, so CC 74 always goes to controller 7.

Settled:

- **A127 is overwritten on MPE entry** by design.
- **MPE+** is parked. Since 24 November 2026 the main mode has two positions only.
- **Parameter 21 = 0** is correct: parameter 21 is Voice Assign Mode, 0 = MONO_MULTI
  (Korg program parameter list).
- **Unison button and pot A8** had swapped arguments; fixed with the unison work.
- **Pitch bend in Normal mode** was never sent to the Z1; fixed.
- **Parameter table audit (24 November 2026).** All 784 parameter definitions in
  `Z1.cpp` were checked by script: each one's key against the slot it is stored in,
  and its range against the Korg program parameter list. Five wrong keys were fixed
  (EG release level 39, EG decay-time mod intensity 47, OSC1 fine tune 177, noise
  filter cutoff mod intensity 220, reed pressure mod 2 intensity 662: each sent its
  neighbour's parameter, or in one case a number that does not exist). Seven ranges
  were corrected (fine tune of OSC1 and OSC2 to ±50, the four harmonics mod
  intensities of the resonance oscillator to ±15, talking modulator resonance to
  0–99). The script does not check labels, nor the byte positions used when a
  program dump is read.
