#pragma once

#include <vector>
#include <cstdint>
#include <string>

struct SimpleVideoInfo {
    double duration;
    int32_t width;
    int32_t height;
    double fps;
    int64_t bitrate;
    std::string codec;
};

struct SimpleThumbnailOptions {
    int32_t width = 320;
    int32_t height = 240;
    int32_t quality = 85;
    bool maintain_aspect_ratio = true;
    std::string format = "jpeg";
};