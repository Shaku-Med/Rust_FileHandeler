#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace vthumb {
struct ImageBuffer {
    std::vector<uint8_t> data;
    int width{0};
    int height{0};
    int channels{3};
    std::string mime;
};

struct DecodeOptions {
    double timestampSeconds{0.0};
    int targetWidth{0};
    int targetHeight{0};
    bool exactSeek{false};
    bool preserveAspect{true};
    bool allowUpscale{false};
    std::string pixelFormat;
};

ImageBuffer extract_thumbnail_from_bytes(const uint8_t* bytes, size_t length, const DecodeOptions& options);
}
