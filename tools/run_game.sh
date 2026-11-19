#!/bin/bash
# Headless test run (no GPU needed): Xvfb + Mesa lavapipe (software Vulkan), PulseAudio null sink
# (the SDK's SDL3 has no dummy/disk audio driver).
# Takes a screenshot every 15 s and summarises errors from the game log.
#   tools/run_game.sh <seconds> <tag>
# Outputs go to logs/<tag>.out, logs/<tag>.log and logs/<tag>_<t>.png
SECS=${1:-120}; TAG=${2:-run}
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD="$ROOT/out/build/linux-amd64-release"
LOGS="$ROOT/logs"; mkdir -p "$LOGS"
export DISPLAY=:99 VK_ICD_FILENAMES=/usr/share/vulkan/icd.d/lvp_icd.json \
       LD_LIBRARY_PATH=/root/rexsdk/linux-amd64/lib SDL_AUDIO_DRIVER=pulseaudio XDG_RUNTIME_DIR=/tmp/xdg
mkdir -p /tmp/xdg && chmod 700 /tmp/xdg
pactl info >/dev/null 2>&1 || pulseaudio -D --exit-idle-time=-1 -n --load="module-null-sink sink_name=null" --load="module-native-protocol-unix" 2>/dev/null
pgrep -x Xvfb >/dev/null || (Xvfb :99 -screen 0 1280x720x24 >/dev/null 2>&1 &); sleep 1
rm -f /dev/shm/xenia_memory_* 2>/dev/null
cd "$BUILD" && rm -rf logs
[ -e assets ] || ln -s "$ROOT/game" assets
timeout "$SECS" ./guardianheroes > "$LOGS/$TAG.out" 2>&1 &
PID=$!
for t in $(seq ${SHOT_EVERY:-15} ${SHOT_EVERY:-15} "$SECS"); do
  sleep 15; kill -0 $PID 2>/dev/null || break
  import -window root "$LOGS/${TAG}_$t.png" 2>/dev/null
done
wait $PID; echo "exit $?" >> "$LOGS/$TAG.out"
cat logs/*.log > "$LOGS/$TAG.log" 2>/dev/null
grep -E "\[(critical|error)\]" "$LOGS/$TAG.log" | cut -c1-220 | sed 's/^\[[^]]*\] //' | sort | uniq -c | sort -rn | head -15
tail -3 "$LOGS/$TAG.out"; ls "$LOGS/${TAG}"_*.png 2>/dev/null
