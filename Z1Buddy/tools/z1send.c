// z1send - send a .syx file to a MIDI port, one message at a time, and count
// the Z1's answers.
//
//   z1send <port name> <file.syx> [gap in ms, default 12]
//
// Meant for files of Korg Z1 parameter changes: the Z1 answers each with
// "data load completed" (23h) or "data load error" (24h).
// macOS only (CoreMIDI). Build:
//   cc -o z1send z1send.c -framework CoreMIDI -framework CoreFoundation
#include <CoreMIDI/CoreMIDI.h>
#include <CoreFoundation/CoreFoundation.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int ok = 0, bad = 0, in_sysex = 0, pos = 0;
static unsigned char cur[8];

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
    for (int j = 0; j < p->length; j++) {
      unsigned char c = p->data[j];
      if (c >= 0xf8) continue;
      if (c == 0xf0) { in_sysex = 1; pos = 0; }
      if (!in_sysex) continue;
      if (pos < 8) cur[pos] = c;
      pos++;
      if (c != 0xf7) continue;
      in_sysex = 0;
      if (pos == 6 && cur[1] == 0x42 && cur[3] == 0x46) {
        if (cur[4] == 0x23) ok++;
        else if (cur[4] == 0x24 || cur[4] == 0x26) bad++;
      }
    }
  }
}

int main(int argc, char **argv) {
  if (argc < 3) { fprintf(stderr, "usage: z1send <port name> <file.syx> [gap in ms]\n"); return 2; }
  double gap = argc > 3 ? atof(argv[3]) / 1000.0 : 0.012;
  char name[256];
  int src = -1, dst = -1;
  for (ItemCount i = 0; i < MIDIGetNumberOfSources(); i++) { port_name(MIDIGetSource(i), name, sizeof name); if (strstr(name, argv[1])) src = (int)i; }
  for (ItemCount i = 0; i < MIDIGetNumberOfDestinations(); i++) { port_name(MIDIGetDestination(i), name, sizeof name); if (strstr(name, argv[1])) dst = (int)i; }
  if (src < 0 || dst < 0) { fprintf(stderr, "MIDI port \"%s\" not found\n", argv[1]); return 1; }
  FILE *f = fopen(argv[2], "rb");
  if (!f) { perror(argv[2]); return 1; }
  static unsigned char data[1 << 20];
  int n = (int)fread(data, 1, sizeof data, f);
  fclose(f);

  MIDIClientRef client;
  MIDIPortRef in, out;
  MIDIClientCreate(CFSTR("z1send"), NULL, NULL, &client);
  MIDIInputPortCreate(client, CFSTR("in"), on_midi, NULL, &in);
  MIDIOutputPortCreate(client, CFSTR("out"), &out);
  MIDIPortConnectSource(in, MIDIGetSource(src), NULL);

  int sent = 0, start = -1;
  for (int i = 0; i < n; i++) {
    int status = data[i] >= 0x80;
    if (data[i] == 0xf0) start = i;
    else if (start < 0 && status) start = i;        // a channel message: status + data bytes
    int end = -1;
    if (start >= 0 && data[start] == 0xf0) { if (data[i] == 0xf7) end = i; }
    else if (start >= 0 && (i + 1 == n || data[i + 1] >= 0x80)) end = i;
    if (end < 0) continue;
    unsigned char pbuf[2048];
    MIDIPacketList *pl = (MIDIPacketList *)pbuf;
    MIDIPacketListAdd(pl, sizeof pbuf, MIDIPacketListInit(pl), 0, end - start + 1, data + start);
    MIDISend(out, MIDIGetDestination(dst), pl);
    sent++;
    start = -1;
    CFRunLoopRunInMode(kCFRunLoopDefaultMode, gap, false);
  }
  CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0.5, false);
  if (ok == 0 && bad == 0) printf("%d messages sent (the Z1 sent no confirmations back)\n", sent);
  else printf("%d messages sent; the Z1 confirmed %d and refused %d\n", sent, ok, bad);
  return bad ? 1 : 0;
}
