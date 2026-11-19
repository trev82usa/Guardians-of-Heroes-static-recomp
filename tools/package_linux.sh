#!/bin/bash
# Packages the Linux build as GuardianHeroes-linux-x64.tar.gz.
#   tools/package_linux.sh [output dir] [--with-game]
# --with-game copies game/ (your own extracted files) into assets/ for a ready-to-run private copy.
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD="$ROOT/out/build/linux-amd64-release"
SDK_LIB="${REXSDK_PREFIX:-/root/rexsdk/linux-amd64}/lib"
OUT="${1:-$ROOT/out}"
STAGE="$(mktemp -d)"; PKG="$STAGE/GuardianHeroes"
mkdir -p "$PKG/assets" "$PKG/tools"
cp "$BUILD/guardianheroes" "$PKG/"
# Build for game version 0.0.0.2, if it was built (variants/v2). Either program starts the one
# that matches the game files in assets/.
V2="$ROOT/variants/v2/out/build/linux-amd64-release/guardianheroes-v2"
if [ -f "$V2" ]; then cp "$V2" "$PKG/"; patchelf --set-rpath '$ORIGIN' "$PKG/guardianheroes-v2"; fi
cp "$BUILD"/librexgpu-xenos.so "$SDK_LIB/librexruntime.so" "$PKG/"
for f in "$PKG"/*.so; do patchelf --set-rpath '$ORIGIN' "$f"; done
cp "$ROOT/packaging/README-linux.txt" "$PKG/README.txt"
cp "$ROOT/guardianheroes.toml" "$PKG/guardianheroes.toml"
cp "$ROOT/tools/stfs_extract.py" "$PKG/tools/"
cp "$ROOT/tools/install_game.py" "$PKG/install_game.py"; chmod +x "$PKG/install_game.py"
if [ "${2:-}" = "--with-game" ]; then
  cp -r "$ROOT/game/." "$PKG/assets/"
else
  echo "Run install_game.py (next to the game) to fill this folder from your game package (see README.txt)." \
    > "$PKG/assets/PUT_GAME_FILES_HERE.txt"
fi
(cd "$PKG" && ldd ./guardianheroes ./*.so | grep "not found") && { echo "missing libraries"; exit 1; }
tar -C "$STAGE" -czf "$OUT/GuardianHeroes-linux-x64.tar.gz" GuardianHeroes
rm -rf "$STAGE"
ls -la "$OUT/GuardianHeroes-linux-x64.tar.gz"
