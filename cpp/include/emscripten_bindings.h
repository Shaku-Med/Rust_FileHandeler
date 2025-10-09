#pragma once

#ifdef __EMSCRIPTEN__
#include <emscripten/bind.h>
#include <emscripten/val.h>
#endif

#include <vector>
#include <cstdint>

class EmscriptenBindings {
public:
    static void initialize();
    
    static std::vector<uint8_t> generateThumbnail(
        const std::vector<uint8_t>& videoData,
        double timeSeconds,
        int32_t width,
        int32_t height,
        int32_t quality
    );
    
    static std::vector<std::vector<uint8_t>> generateMultipleThumbnails(
        const std::vector<uint8_t>& videoData,
        const std::vector<double>& timePoints,
        int32_t width,
        int32_t height,
        int32_t quality
    );
    
    static VideoInfo getVideoInfo(const std::vector<uint8_t>& videoData);
    
    static bool isVideoSupported(const std::vector<uint8_t>& videoData);
    
    static std::vector<std::string> getSupportedFormats();
    
    static void setMemoryLimit(size_t maxMemory);
    static size_t getMemoryUsage();
    static void clearMemory();

#ifdef __EMSCRIPTEN__
    static emscripten::val generateThumbnailJS(
        emscripten::val videoData,
        double timeSeconds,
        int32_t width,
        int32_t height,
        int32_t quality
    );
    
    static emscripten::val generateMultipleThumbnailsJS(
        emscripten::val videoData,
        const std::vector<double>& timePoints,
        int32_t width,
        int32_t height,
        int32_t quality
    );
    
    static emscripten::val getVideoInfoJS(emscripten::val videoData);
#endif
};