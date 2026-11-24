# Patches

Sounds for the Korg Z1, sent through the Z1 Buddy from the command line.

## Supersaw MPE

A Roland JP-8000 style supersaw, made for MPE mode with unison.

| File | What it is |
| --- | --- |
| `make_supersaw.py` | The patch, as readable settings. Run it to rebuild `supersaw.syx`. |
| `supersaw.syx` | 218 parameter changes. Turns the sound selected on the Z1 into the patch. |
| `supersaw_program.syx` | The same sound as one program dump, read back from the Z1 on 24 November 2026. |

Send it (either file works):

```
cd tools
cc -o z1send z1send.c -framework CoreMIDI -framework CoreFoundation   # once
./z1send "Z1 Buddy" patches/supersaw.syx
```

Nothing is stored on the Z1 until you write the sound there, or enter MPE mode on
the Buddy (which copies it to A127).

How many saws you get:

| Buddy unison in MPE mode | Notes at once | Saws per note |
| --- | --- | --- |
| Off | 6 | 3 |
| 2 | 3 | 6 |
| 3 | 2 | 9 |
| 6 | 1 | 18 |

In MPE mode the Buddy's Multi setup adds a multitap delay for all voices, timed
for 136 BPM (see `../multi_info.py`).

A JP-8000 supersaw is 7 saws, so unison 2 or 3 is the closest match.

The mod wheel adds vibrato (LFO 1 on the pitch of all three oscillators). Pitch
bend is 2 semitones up and down, as on a normal keyboard; in MPE mode the Buddy
scales each key's own bend to that range.
