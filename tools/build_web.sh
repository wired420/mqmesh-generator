#!/bin/sh
# Builds the WebAssembly page into dist/. Needs emsdk activated (source emsdk_env.sh).
set -e
ROOT=$(cd "$(dirname "$0")/.." && pwd)
# Emscripten's own port downloads can be blocked by proxies; use the local SDL2 clone.
export EMCC_LOCAL_PORTS="sdl2=$ROOT/third_party/SDL2"
emcmake cmake -S "$ROOT" -B "$ROOT/build-web" -G Ninja -DCMAKE_BUILD_TYPE=Release
ninja -C "$ROOT/build-web" meshgen
"$ROOT/tools/package.sh"
