#!/bin/sh
# Fetches third-party sources into third_party/ and builds protobuf (host protoc + wasm lib).
set -e
ROOT=$(cd "$(dirname "$0")/.." && pwd)
TP="$ROOT/third_party"
mkdir -p "$TP" && cd "$TP"
clone() { [ -d "$2" ] || git clone -q --depth 1 -b "$3" "$1" "$2"; }
clone https://github.com/g-truc/glm glm 1.0.1
clone https://github.com/fmtlib/fmt fmt 11.2.0
clone https://github.com/gabime/spdlog spdlog v1.15.3
clone https://github.com/Tencent/rapidjson rapidjson master
clone https://github.com/nothings/stb stb master
clone https://github.com/protocolbuffers/protobuf protobuf v21.12
clone https://github.com/ocornut/imgui imgui-upstream v1.92.5-docking
clone https://github.com/libsdl-org/SDL SDL2 release-2.32.10
clone https://github.com/madler/zlib zlib v1.3.1

cd protobuf
if [ ! -x build-host/protoc ]; then
  cmake -S . -B build-host -G Ninja -DCMAKE_BUILD_TYPE=Release -Dprotobuf_BUILD_TESTS=OFF -Dprotobuf_BUILD_SHARED_LIBS=OFF
  ninja -C build-host
  cmake --install build-host --prefix "$TP/protobuf-host"
fi
if [ ! -f "$TP/protobuf-wasm/lib/libprotobuf.a" ]; then
  emcmake cmake -S . -B build-wasm -G Ninja -DCMAKE_BUILD_TYPE=Release -Dprotobuf_BUILD_TESTS=OFF \
    -Dprotobuf_BUILD_PROTOC_BINARIES=OFF -Dprotobuf_BUILD_SHARED_LIBS=OFF -Dprotobuf_WITH_ZLIB=OFF \
    -DCMAKE_CXX_FLAGS=-pthread -DCMAKE_C_FLAGS=-pthread -DCMAKE_INSTALL_PREFIX="$TP/protobuf-wasm"
  ninja -C build-wasm libprotobuf && ninja -C build-wasm install
fi
"$TP/protobuf-host/bin/protoc" --proto_path="$ROOT/src/common/proto" --cpp_out="$ROOT/src/common/proto" "$ROOT/src/common/proto/NavMeshFile.proto"
