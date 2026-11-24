#!/bin/sh
# Runs the firmware's MPE unison, wonkiness and background-queue logic on the
# Mac, with stand-ins for the Teensy parts. The code under test is cut straight
# out of ../../firmware/firmware.ino, so the tests always run what is flashed.
set -e
cd "$(dirname "$0")"
python3 - <<'PY'
s = open('../../firmware/firmware.ino', newline='').read().replace('\r', '')
a = s.index("struct WonkSpec {"); b = s.index("// MPE voice range")
c = s.index("// ============ MPE unison ============"); d = s.index("// ============ Pots ============")
open('block.inc', 'w').write(s[a:b] + s[c:d])
t0 = s.index("struct TxEvent {"); t1 = s.index("};", t0) + 3
q0 = s.index("static constexpr int TX_WATERMARK"); q1 = s.index("void sendcc(")
open('txblock.inc', 'w').write(s[t0:t1] + s[q0:q1])
PY
for t in unison_test wonk_test queue_test txqueue_test; do
  c++ -std=c++17 -o "$t" "$t.cpp"
  echo "== $t"
  # a test that hangs must not hang the script
  ( "./$t" & pid=$!; ( sleep 20; kill $pid 2>/dev/null ) & wait $pid ) | grep -v "^ok" || true
done
rm -f unison_test wonk_test queue_test txqueue_test block.inc txblock.inc
