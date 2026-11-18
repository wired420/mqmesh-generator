#!/bin/sh
# Copies the web build into dist/
set -e
ROOT=$(cd "$(dirname "$0")/.." && pwd)
mkdir -p "$ROOT/dist"
cp "$ROOT/web/index.html" "$ROOT/web/coi-serviceworker.js" "$ROOT/web/.htaccess" "$ROOT/dist/"
cp "$ROOT/build-web/meshgen.js" "$ROOT/build-web/meshgen.wasm" "$ROOT/dist/"
