#!/bin/bash

set -e

source "/workspace/cpp/emsdk/emsdk_env.sh"

if [ ! -d "build" ]; then
    mkdir build
fi

cd build

echo "Building with Emscripten..."
cp ../CMakeLists_simple.txt ./CMakeLists.txt
emcmake cmake .. -DCMAKE_BUILD_TYPE=Release
emmake make -j$(nproc)

echo "Build completed successfully!"
echo "Output: build/video_thumbnail_wasm.js"
echo "Output: build/video_thumbnail_wasm.wasm"