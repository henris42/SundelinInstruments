#!/bin/sh
# Read the Multi setup that is selected on the Z1 and turn it into
# firmware/mpe_multi.h, the setup the Buddy loads when it enters MPE mode.
#
#   tools/pull_multi.sh            the Multi setup currently selected on the Z1
#   tools/pull_multi.sh --default  no Z1 needed: six timbres, channels 2-7, A127
#
# The Z1 must be connected through the Buddy (MIDI port "Z1 Buddy") and have
# "SysEx Receive" enabled. Rebuild and flash the firmware afterwards.
set -e
cd "$(dirname "$0")"
PORT="${Z1_PORT:-Z1 Buddy}"

if [ "$1" = "--default" ]; then
  python3 multi_info.py --default mpe_multi.syx
else
  [ -x ./z1sysex ] || cc -o z1sysex z1sysex.c -framework CoreMIDI -framework CoreFoundation
  # 19h = current multi setup data dump request; the reply is function 49h
  ./z1sysex "$PORT" 49 mpe_multi.syx f0 42 30 46 19 00 f7
  python3 multi_info.py mpe_multi.syx
fi

# xxd names the array after the file: mpe_multi_syx and mpe_multi_syx_len
xxd -i mpe_multi.syx > ../firmware/mpe_multi.h
echo "wrote firmware/mpe_multi.h"
