# MQMesh Generator (web)

MQ2Nav's MeshGenerator compiled to WebAssembly so it runs in the browser at
[mqmesh.com/generator](https://mqmesh.com/generator/). Users pick their own EverQuest folder;
the zone files are read locally in the browser and never uploaded.

## How it works

- `src/` is the MeshGenerator source from [brainiac/MQ2Nav](https://github.com/brainiac/MQ2Nav)
  (`meshgen`, `common`, Recast/Detour and EQEmu zone-utilities), patched to build with clang.
- `mq/` holds the MacroQuest headers and imgui fork it depends on.
- `platform/` holds the web-specific parts:
  - `compat.h`: shims for the Windows-only calls (force-included).
  - `gl_renderer.*`: a WebGL2 replacement for the OpenGL 1.x immediate mode drawing.
  - `web_files.*`: folder picker, copying a zone's files into the virtual file system, downloads.
  - `cli_main.cpp`: a headless driver (load zone, build, save) used for testing.
- `web/` is the page shell. Tile builds use threads, so the page must be cross-origin isolated:
  `web/.htaccess` sets the COOP/COEP headers on Apache, and `coi-serviceworker.js` is the fallback.

## Building

Needs [emsdk](https://emscripten.org/docs/getting_started/downloads.html), CMake and Ninja.

```sh
source /path/to/emsdk/emsdk_env.sh
tools/fetch_deps.sh      # third-party sources and protobuf
tools/build_web.sh       # outputs dist/
python3 tools/serve.py dist 8000   # local server with the right headers
```

Native headless build for testing (Linux, clang): `cmake -S . -B build-native -G Ninja && ninja -C build-native`,
then `build-native/meshgen-cli <eq folder> <out folder> <zone>`.

`tools/browser_test.js` is a Playwright test that loads a zone, builds and saves a mesh.

## Licenses

MQ2Nav and its dependencies keep their own licenses (Recast/Detour: zlib; EQEmu zone-utilities: GPL v2;
MacroQuest and Dear ImGui: MIT). See the files under `src/dependencies` and `mq/`.
