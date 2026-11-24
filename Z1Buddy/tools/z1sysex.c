// z1sysex - send a sysex request to a MIDI port and save the reply.
//
//   z1sysex <port name> <function hex> <out file> <request bytes in hex...>
//
// Waits up to 3 seconds for a Korg Z1 message (F0 42 3g 46 ...) or a Buddy
// message (F0 7D 5A 31 ...) whose function byte is <function hex> and writes
// it, F0 to F7, to <out file>.
// macOS only (CoreMIDI). Build:
//   cc -o z1sysex z1sysex.c -framework CoreMIDI -framework CoreFoundation
#include <CoreMIDI/CoreMIDI.h>
#include <CoreFoundation/CoreFoundation.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned char msg[65536];
static int len = 0, in_sysex = 0, done = 0, want = 0;
static unsigned char other[16];
static int other_len = 0;

static void port_name(MIDIObjectRef o, char *out, int size) {
  CFStringRef s = NULL;
  out[0] = 0;
  if (MIDIObjectGetStringProperty(o, kMIDIPropertyDisplayName, &s) == noErr && s) {
    CFStringGetCString(s, out, size, kCFStringEncodingUTF8);
    CFRelease(s);
  }
}

static void on_midi(const MIDIPacketList *pl, void *a, void *b) {
  const MIDIPacket *p = &pl->packet[0];
  for (unsigned i = 0; i < pl->numPackets; i++, p = MIDIPacketNext(p)) {
    for (int j = 0; j < p->length && !done; j++) {
      unsigned char c = p->data[j];
      if (c >= 0xf8) continue;                    // realtime
      if (c == 0xf0) { in_sysex = 1; len = 0; }
      if (!in_sysex) continue;
      if (len < (int)sizeof msg) msg[len++] = c;
      if (c != 0xf7) continue;
      in_sysex = 0;
      // a Korg Z1 message (42 3g 46), or one of the Buddy's own (7D 5A 31)
      int korg = msg[1] == 0x42 && msg[3] == 0x46, buddy = msg[1] == 0x7d && msg[2] == 0x5a && msg[3] == 0x31;
      if (len > 5 && (korg || buddy) && msg[4] == want) {
        done = 1;
      } else if (len <= (int)sizeof other) {      // remember a short answer, e.g. an error
        memcpy(other, msg, len);
        other_len = len;
      }
    }
  }
}

int main(int argc, char **argv) {
  if (argc < 5) {
    fprintf(stderr, "usage: z1sysex <port name> <function hex> <out file> <request bytes in hex...>\n");
    return 2;
  }
  char name[256];
  int src = -1, dst = -1;
  for (ItemCount i = 0; i < MIDIGetNumberOfSources(); i++) {
    port_name(MIDIGetSource(i), name, sizeof name);
    if (strstr(name, argv[1])) src = (int)i;
  }
  for (ItemCount i = 0; i < MIDIGetNumberOfDestinations(); i++) {
    port_name(MIDIGetDestination(i), name, sizeof name);
    if (strstr(name, argv[1])) dst = (int)i;
  }
  if (src < 0 || dst < 0) { fprintf(stderr, "MIDI port \"%s\" not found\n", argv[1]); return 1; }
  want = (int)strtol(argv[2], NULL, 16);

  unsigned char req[256];
  int n = 0;
  for (int i = 4; i < argc && n < (int)sizeof req; i++) req[n++] = (unsigned char)strtol(argv[i], NULL, 16);

  MIDIClientRef client;
  MIDIPortRef in, out;
  MIDIClientCreate(CFSTR("z1sysex"), NULL, NULL, &client);
  MIDIInputPortCreate(client, CFSTR("in"), on_midi, NULL, &in);
  MIDIOutputPortCreate(client, CFSTR("out"), &out);
  MIDIPortConnectSource(in, MIDIGetSource(src), NULL);

  unsigned char pbuf[1024];
  MIDIPacketList *pl = (MIDIPacketList *)pbuf;
  MIDIPacketListAdd(pl, sizeof pbuf, MIDIPacketListInit(pl), 0, n, req);
  MIDISend(out, MIDIGetDestination(dst), pl);
  for (int i = 0; i < 30 && !done; i++) CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0.1, false);

  if (!done) {
    if (other_len > 4 && other[4] == 0x26) fprintf(stderr, "the Z1 answered DATA FORMAT ERROR\n");
    else fprintf(stderr, "no reply with function %02X within 3 seconds\n", want);
    return 1;
  }
  FILE *f = fopen(argv[3], "wb");
  if (!f) { perror(argv[3]); return 1; }
  fwrite(msg, 1, len, f);
  fclose(f);
  printf("%d bytes written to %s\n", len, argv[3]);
  return 0;
}
