#!/bin/bash

set -e

if [ ! -d "build" ]; then
    mkdir build
fi

cd build

if [ ! -d "libs" ]; then
    mkdir libs
    echo "Please place FFmpeg static libraries in the libs/ directory:"
    echo "- libavformat.a"
    echo "- libavcodec.a" 
    echo "- libavutil.a"
    echo "- libswscale.a"
    echo ""
    echo "You can build them using:"
    echo "emconfigure ./configure --enable-static --disable-shared --disable-doc --disable-ffmpeg --disable-ffplay --disable-ffprobe"
    echo "emmake make"
    exit 1
fi

emcmake cmake .. -DCMAKE_BUILD_TYPE=Release
emmake make -j$(nproc)

echo "Build completed successfully!"
echo "Output: build/video_thumbnail_wasm.js"
echo "Output: build/video_thumbnail_wasm.wasm"