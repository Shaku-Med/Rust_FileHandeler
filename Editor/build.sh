#!/bin/bash

# Video Editor Build Script
echo "🎬 Building Video Editor..."
echo ""

# Check if Emscripten is available
if ! command -v emcc &> /dev/null; then
    echo "❌ Emscripten (emcc) not found!"
    echo "Please activate Emscripten first:"
    echo "  source ~/emsdk/emsdk_env.sh"
    exit 1
fi

echo "✓ Emscripten found"
echo ""

# Create build directory
mkdir -p build

# Compile C code to WASM
echo "📦 Compiling editor.c to WebAssembly..."
emcc -O3 src/editor.c \
  -o build/editor.js \
  -s WASM=1 \
  -s ALLOW_MEMORY_GROWTH=1 \
  -s EXPORTED_FUNCTIONS='["_init_project","_add_video_clip","_add_text_layer","_add_effect","_export_to_hls","_get_clip_count","_get_text_count","_get_effect_count","_clear_project","_malloc","_free"]' \
  -s EXPORTED_RUNTIME_METHODS='["ccall","cwrap","UTF8ToString","stringToUTF8"]' \
  -s MODULARIZE=1 \
  -s EXPORT_NAME="createEditorModule"

if [ $? -eq 0 ]; then
    echo "✓ Compilation successful!"
    echo ""
    echo "Generated files:"
    ls -lh build/editor.js build/editor.wasm
    echo ""
    echo "🎉 Build complete!"
    echo ""
    echo "To run the editor:"
    echo "  cd .."
    echo "  python3 ../server.py"
    echo "  Then open: http://localhost:3000/Editor/index.html"
else
    echo "❌ Compilation failed!"
    exit 1
fi