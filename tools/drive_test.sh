#!/bin/bash
# Headless scripted run: tools/drive_test.sh <seconds> "<t>:<key|shot> ..." [tag]
# Keys are X keysyms sent with xdotool (held 0.8 s, the software renderer runs ~12 fps).
SECS=$1; SCRIPT=$2; TAG=${3:-drive}
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD="$ROOT/out/build/linux-amd64-release"; LOGS="$ROOT/logs"; mkdir -p "$LOGS"
export DISPLAY=:99 VK_ICD_FILENAMES=/usr/share/vulkan/icd.d/lvp_icd.json \
       LD_LIBRARY_PATH=/root/rexsdk/linux-amd64/lib SDL_AUDIO_DRIVER=pulseaudio XDG_RUNTIME_DIR=/tmp/xdg
mkdir -p /tmp/xdg && chmod 700 /tmp/xdg
pactl info >/dev/null 2>&1 || pulseaudio -D --exit-idle-time=-1 -n --load="module-null-sink sink_name=null" --load="module-native-protocol-unix" 2>/dev/null
pgrep -x Xvfb >/dev/null || (Xvfb :99 -screen 0 1280x720x24 >/dev/null 2>&1 &); sleep 1
rm -f /dev/shm/xenia_memory_* "$LOGS/${TAG}"_*.png
cd "$BUILD" && rm -rf logs; [ -e assets ] || ln -s "$ROOT/game" assets
timeout "$SECS" ./guardianheroes $GH_ARGS > "$LOGS/$TAG.out" 2>&1 & PID=$!
START=$(date +%s)
for step in $SCRIPT; do
  t=${step%%:*}; k=${step#*:}
  while [ $(( $(date +%s) - START )) -lt "$t" ]; do sleep 0.2; done
  kill -0 $PID 2>/dev/null || break
  if [ "$k" = shot ]; then import -window root "$LOGS/${TAG}_$t.png"
  else xdotool keydown "$k"; sleep 0.8; xdotool keyup "$k"; fi
done
wait $PID; echo "exit $?" >> "$LOGS/$TAG.out"
cat logs/*.log > "$LOGS/$TAG.log" 2>/dev/null
grep -E "\[critical\]|FATAL" "$LOGS/$TAG.log" | head -3; grep FPS "$LOGS/$TAG.log" | tail -1
