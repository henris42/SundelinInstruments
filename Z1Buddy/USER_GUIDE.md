# Z1 Buddy: user guide

The Z1 Buddy is a control panel for the Korg Z1. It gives you a knob and a display
for each parameter of the sound, connects the Z1 to the computer over USB, and lets
you play the Z1 from an MPE controller such as the Osmose.

This guide is for firmware of November 2026. Three features are new in that
version and had not been tried on the instrument when this was written: **unison**,
**unison in MPE mode** and **wonkiness**. They are marked "new" below; expect them
to need adjusting.

## Contents

- [Connecting](#connecting)
- [The panel](#the-panel)
- [Editing a sound](#editing-a-sound)
- [The three pots](#the-three-pots)
- [Unison](#unison)
- [MPE mode](#mpe-mode)
- [Wonkiness](#wonkiness)
- [Using it with Live](#using-it-with-live)
- [Good to know](#good-to-know)

## Connecting

1. **Buddy to Z1:** MIDI out of the Buddy to MIDI in of the Z1, and MIDI out of the
   Z1 to MIDI in of the Buddy. Both directions are needed: the Buddy reads the sound
   from the Z1.
2. **Buddy to computer:** USB. The computer sees a MIDI port called **Z1 Buddy**.
   Everything you send to that port goes to the Z1, and what the Z1 sends comes
   back on it.

On the Z1, in Global mode:

| Setting | Value | Why |
| --- | --- | --- |
| SysEx Receive | Enable | The Buddy edits the sound with system exclusive messages; without this the knobs do nothing |
| SysEx Transmit | On | The Z1 then sends the whole sound after every program change, so the Buddy's displays follow |
| Global MIDI channel | 1 | The Buddy talks to the Z1 on channel 1 only |

When the Buddy starts it asks the Z1 for the current sound and shows the first
oscillator page.

## The panel

- **Three displays**, six parameters each. The top line is the parameter's name,
  the bottom line its value.
- **18 knobs**, one under each parameter on the displays.
- **Page buttons** choose what the knobs control: Oscillator 1, Oscillator 2, Sub
  oscillator, Filter, Effects, Envelopes, LFOs. A light shows the current page
  (Oscillator 1 and 2 light together for the sub oscillator).
- **Mod button:** switches the current page to its modulation settings and back. A
  light shows when it is on.
- **Unison button** with three lights: 2, 3 and 6 voices.
- **Main mode button** with two lights: Normal and MPE.
- **Dump button:** sends the Z1's current sound and arpeggio pattern to the computer.
- **Three pots:** wonkiness, portamento time and detune. When you turn one, its
  name and value appear for a moment at the right end of the last display.

## Editing a sound

1. Select the sound on the Z1 (or from the computer).
2. Press a page button. The displays show that section's parameters.
3. Turn a knob. The Z1 changes at once and the display shows the new value.
4. Press the same page button again for the section's next page. The Filter button,
   for example, steps through filter 1, filter 2 and the common filter settings.
5. Press **Mod** for the modulation settings of the same section.

- The oscillator pages change with the oscillator type. Choose a different type
  (Standard, Comb, VPM, Resonance, Ring, Cross, Sync, Organ, E. Piano, Brass, Reed,
  Plucked, Bowed) and the page shows that model's own parameters.
- Knobs on the Z1 itself still work, and the Buddy's displays follow them.
- **Nothing is stored by the Buddy.** You are editing the Z1's current sound; to
  keep it, write it on the Z1 as usual.
- The effects pages are not complete: some effect parameters are missing.

## The three pots

| Pot | Normal mode | MPE mode |
| --- | --- | --- |
| Wonkiness | Old-synth unevenness, see [Wonkiness](#wonkiness) | The same, plus each voice slightly out of tune |
| Portamento | Portamento time | Portamento time |
| Detune | Detune of the sound's unison, 0–99 cents | Spread of the stacked voices, 0–50 cents |

## Unison

*New.*

Press the **Unison** button to step through off, 2, 3 and 6 voices. The lights show
the setting.

In Normal mode this is the Z1's own unison: every note you play is doubled 2, 3 or
6 times inside the Z1, and the **Detune** pot sets how far the copies are apart. It
is part of the sound, so it is stored with it when you write the sound. When you
load another sound the lights follow that sound's setting.

Unison uses up voices: with 6-voice unison the Z1 plays two notes at a time (three
with the voice expansion).

For what the button does in MPE mode, see below.

## MPE mode

The Z1 does not know MPE. The Buddy makes it work by having the Z1 play six copies
of the same sound, one per note, each on its own MIDI channel. Each note can then
have its own pitch bend, its own filter movement and its own pressure.

### Before you start

- **On the Z1:** nothing to prepare. When you enter MPE mode the Buddy switches the
  Z1 to Multi mode and gives it the setup it needs: six timbres on MIDI channels 2
  to 7, all playing program **A127**. When you leave, it switches the Z1 back to
  Program mode. (*New.* The mode switching has been tried; the setup has not yet
  been loaded into the instrument.)
- **On the controller or in Live:** MPE with six note channels, 2 to 7, and channel
  1 as the global channel. (With unison on, any of channels 2 to 16 will do.)
- **Program A127 is the Buddy's working slot.** It is overwritten every time you
  enter MPE mode, so do not keep a sound there. The Buddy and Patch Sync count
  programs from 0, so A127 is the last program of bank A; the Z1's own display
  counts from 1 and shows it as A128.

### Entering and leaving

1. Select and edit the sound you want to play, in Normal mode.
2. Press the **Main mode** button. The Buddy stores the sound in A127, switches the
   Z1 to Multi mode, sends it the six-voice setup and sets the sound to play one
   note at a time, as each copy only ever plays one. The MPE light comes on. This
   takes a moment; if the Z1 does not answer, press the button again to go back.
3. Play. Up to six notes sound at once.
4. Press the button again to go back to Normal.

### What your gestures do

| Gesture | Result on the Z1 |
| --- | --- |
| Sideways (per-note pitch bend) | Bends that note only |
| Along the key (CC 74) | Sent as controller 7, volume, for that note |
| Pressure | Channel aftertouch for that note |
| Pitch bend on the global channel | Shifts all notes together |
| Other controllers on the global channel | Sent to all six voices |

The CC 74 target is fixed at controller 7 for now. If you play through Live, the MPE
Remap device can send the key position somewhere else before it reaches the Buddy.

A MIDI cable can not carry everything an MPE controller sends for a chord. The
Buddy thins the data: notes always go first and are never lost, pitch comes before
filter and pressure, and each note starts and ends on its exact values. In between,
very fast small movements on many notes at once are smoothed a little.

### Unison in MPE mode

*New.*

In MPE mode the Unison button stacks the six copies instead:

| Unison | Copies per note | Notes at once |
| --- | --- | --- |
| Off | 1 | 6 |
| 2 | 2 | 3 |
| 3 | 3 | 2 |
| 6 | 6 | 1 |

The **Detune** pot spreads the copies of a note apart, up to 50 cents either side.
Bend, key position and pressure go to all copies of that note.

When all the notes are in use and you play another, the oldest one is cut off.
Changing the unison setting silences everything that is sounding.

## Wonkiness

*New.*

The **Wonkiness** pot makes the sound behave like an old analog synth: nothing is
quite in tune or quite where it was set, and it keeps moving a little. At zero it
does nothing. It works in Normal and MPE mode.

Turned up, it does three things:

- **Every note starts slightly off pitch.**
- **Things drift.** The tuning of the oscillators and the two filter cutoffs wander
  slowly around the values of the sound. So do a few parameters that suit each
  oscillator's type: the lip and breath noise of a brass model, where a plucked
  string is plucked and how it is damped, the rosin and bow position of a bowed
  string, the drawbar tuning of an organ, and so on.
- **In MPE mode, each of the six voices is a little out of tune by its own amount,**
  like the voice cards of an old polysynth.

It never moves the effects.

What to know:

- Turn the pot to zero and everything goes back to the sound's own values.
- The drift follows your edits: change the cutoff and it wanders around the new
  value.
- **If you write the sound on the Z1 while the pot is up, the drifted values are
  what gets stored.** Turn it to zero first.
- After a program change the drift pauses for a moment while the Buddy reads the
  new sound.
- The sound's own "random pitch" setting is replaced by the pot while it is up, and
  is zero when you turn it back down.

## Using it with Live

- Choose **Z1 Buddy** as the MIDI output (and input) for the Z1's track.
- **Patch Sync Z1** works through the Buddy: bank and patch dials, the patch name,
  and saving and sending a sound.
- **The Live set remembers the Buddy** (*new*). Patch Sync Z1 stores, with the set,
  whether MPE mode is on, the unison setting and where the three pots stood. Open
  the set and it selects the patch, sets the pots, enters MPE mode if it was on and
  sets unison. A pot set this way keeps that value until you move the pot itself a
  little; then the pot takes over. The sound's edits are not restored this way: for
  that, use Get + Save before saving the set and Send Saved after opening it.
- **Patch Sync Z1 follows MPE mode** (*new*). Its **MPE** button enters and leaves
  MPE mode, and lights when you do it on the Buddy. While MPE mode is on, it shows
  "MPE:" and the name of the sound you came from, keeps the dials on that sound and
  not on the working slot, and does not let you change patch or save: leave MPE
  mode first. In Normal mode it marks A 127 as the MPE working slot when you select
  it.
- When a sound or a single setting is sent to the Z1 from the computer, the
  Buddy's displays follow it.
- For MPE, put **MPE Remap** on the controller's track if you want to change what
  the gestures do before they reach the Buddy.

## Good to know

- **MIDI clock from the Z1 is not passed to the computer.** Start, stop and
  continue are.
- **Pitch bend in Normal mode** may not reach the Z1 from the computer in this
  version; it is being looked into.
- **In Normal mode everything from the computer is sent to the Z1 on channel 1,**
  whatever channel it arrived on.
- **The Dump button** is for saving a sound on the computer: the Z1 sends the sound
  and its arpeggio pattern, and the Buddy passes them on without updating its own
  displays.
- Only Korg Z1 system exclusive messages are passed from the computer to the Z1.
- **If a note hangs:** `tools/buddy panic` on the computer silences everything.
  Before that, `tools/buddy status` shows which notes the Buddy thinks are on,
  which tells where the note-off was lost.
