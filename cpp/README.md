# Video Thumbnail Generator - C++ WebAssembly

High-performance video thumbnail generation using C++ and WebAssembly with FFmpeg.

## Features

- **High Performance**: C++ implementation with optimized memory management
- **Multiple Formats**: Support for JPEG, PNG, and WebP output formats
- **Advanced Processing**: Image resizing, cropping, and filtering
- **Memory Efficient**: Custom memory manager with configurable limits
- **WebAssembly Ready**: Full Emscripten bindings for browser integration

## Project Structure

```
cpp/
├── include/                 # Header files
│   ├── video_thumbnail_generator.h
│   ├── ffmpeg_wrapper.h
│   ├── image_processor.h
│   ├── memory_manager.h
│   └── emscripten_bindings.h
├── src/                     # Source files
│   ├── video_thumbnail_generator.cpp
│   ├── ffmpeg_wrapper.cpp
│   ├── image_processor.cpp
│   ├── memory_manager.cpp
│   └── emscripten_bindings.cpp
├── build/                   # Build output
├── example/                 # Web example
│   └── index.html
├── test/                    # Test files
│   └── test.js
├── CMakeLists.txt          # CMake configuration
├── build.sh                # Build script
└── package.json            # Node.js package
```

## Dependencies

- **FFmpeg**: Video processing and decoding
- **Emscripten**: WebAssembly compilation
- **CMake**: Build system

## Building

1. Install Emscripten SDK
2. Build FFmpeg static libraries
3. Run build script:

```bash
cd cpp
./build.sh
```

## Usage

### JavaScript API

```javascript
import wasmModule from './build/video_thumbnail_wasm.js';

const Module = await wasmModule();

// Generate single thumbnail
const thumbnail = Module.generateThumbnail(
    videoData,    // Uint8Array
    timeSeconds,  // number
    width,        // number
    height,       // number
    quality       // number (1-100)
);

// Generate multiple thumbnails
const thumbnails = Module.generateMultipleThumbnails(
    videoData,     // Uint8Array
    timePoints,    // number[]
    width,         // number
    height,        // number
    quality        // number
);

// Get video information
const info = Module.getVideoInfo(videoData);
```

### C++ API

```cpp
#include "video_thumbnail_generator.h"

VideoThumbnailGenerator generator;

// Load video
generator.loadVideo(videoData);

// Generate thumbnail
ThumbnailOptions options;
options.width = 320;
options.height = 240;
options.quality = 85;

auto thumbnail = generator.generateThumbnail(1.0, options);

// Get video info
auto info = generator.getVideoInfo();
```

## Supported Formats

**Input**: MP4, AVI, MOV, MKV, WebM, FLV, WMV, M4V, 3GP, OGV
**Output**: JPEG, PNG, WebP

## Performance

- Optimized memory management
- Multi-threaded processing support
- Efficient image scaling algorithms
- Minimal memory footprint

## Browser Support

- Chrome 57+
- Firefox 52+
- Safari 11+
- Edge 16+

## License

MIT License