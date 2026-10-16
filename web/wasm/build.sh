#!/usr/bin/env bash
# Compiles the native SOLAFS/WavIO C++ to WebAssembly with Emscripten, inside the official
# emscripten/emsdk Docker image, so nothing is installed on the host. Needs Docker running.
# Run from web/ as: npm run build:wasm
#
# Output (committed to git, so the web app builds and deploys without Emscripten):
#   web/src/engine/generated/solafs.mjs    ES module loader
#   web/src/engine/generated/solafs.wasm   the engine
#   web/src/engine/generated/solafs.build-hash   hash of the inputs (see scripts/wasm-hash.mjs)
set -euo pipefail

EMSDK_IMAGE="emscripten/emsdk:6.0.12"

WEB="$(cd "$(dirname "$0")/.." && pwd)"
REPO="$(cd "$WEB/.." && pwd)"
OUT="web/src/engine/generated"

mkdir -p "$REPO/$OUT"

docker run --rm -v "$REPO":/src -w /src "$EMSDK_IMAGE" \
  emcc -O3 -std=c++14 \
    -Iweb/wasm/compat \
    -Inative/WAVIO/src -Inative/CIRCULAR_BUFFER/src -Inative/SOLAFS/src \
    web/wasm/solafs_wasm.cpp \
    native/WAVIO/src/WavIO/WavIO.cpp \
    native/WAVIO/src/WavIO/FileIO.cpp \
    native/WAVIO/src/Common/HelpDebug.cpp \
    native/CIRCULAR_BUFFER/src/CircFBuf/CircFBuf.cpp \
    native/SOLAFS/src/SOLAFS/SOLAFS.cpp \
    -Wno-format -Wno-missing-declarations \
    -sMODULARIZE=1 -sEXPORT_ES6=1 -sEXPORT_NAME=createSolafsModule \
    -sENVIRONMENT=web,worker,node \
    -sALLOW_MEMORY_GROWTH=1 \
    -sFILESYSTEM=1 \
    -sEXPORTED_FUNCTIONS=_solafs_file \
    -sEXPORTED_RUNTIME_METHODS=FS,ccall \
    -o "$OUT/solafs.mjs"

node "$WEB/scripts/wasm-hash.mjs" > "$REPO/$OUT/solafs.build-hash"
echo "Built $OUT/solafs.mjs + solafs.wasm ($(wc -c < "$REPO/$OUT/solafs.wasm") bytes)"
