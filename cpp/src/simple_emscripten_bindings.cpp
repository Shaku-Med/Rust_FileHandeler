#include <vector>
#include <cstdint>
#include <stdexcept>
#include <string>

#ifdef __EMSCRIPTEN__
#include <emscripten/bind.h>
#include <emscripten/val.h>
#endif

#include "../include/simple_types.h"
#include "simple_thumbnail_generator.cpp"
#include "simple_memory_manager.cpp"

class SimpleEmscriptenBindings {
public:
    static void initialize() {
        SimpleMemoryManager::getInstance().setMaxMemory(256 * 1024 * 1024);
    }
    
    static std::vector<uint8_t> generateThumbnail(
        const std::vector<uint8_t>& videoData,
        double timeSeconds,
        int32_t width,
        int32_t height,
        int32_t quality) {
        
        SimpleVideoThumbnailGenerator generator;
        
        if (!generator.loadVideo(videoData)) {
            throw std::runtime_error("Failed to load video");
        }
        
        SimpleThumbnailOptions options;
        options.width = width;
        options.height = height;
        options.quality = quality;
        
        return generator.generateThumbnail(timeSeconds, options);
    }
    
    static std::vector<std::vector<uint8_t>> generateMultipleThumbnails(
        const std::vector<uint8_t>& videoData,
        const std::vector<double>& timePoints,
        int32_t width,
        int32_t height,
        int32_t quality) {
        
        SimpleVideoThumbnailGenerator generator;
        
        if (!generator.loadVideo(videoData)) {
            throw std::runtime_error("Failed to load video");
        }
        
        SimpleThumbnailOptions options;
        options.width = width;
        options.height = height;
        options.quality = quality;
        
        return generator.generateMultipleThumbnails(timePoints, options);
    }
    
    static SimpleVideoInfo getVideoInfo(const std::vector<uint8_t>& videoData) {
        SimpleVideoThumbnailGenerator generator;
        
        if (!generator.loadVideo(videoData)) {
            throw std::runtime_error("Failed to load video");
        }
        
        return generator.getVideoInfo();
    }
    
    static bool isVideoSupported(const std::vector<uint8_t>& videoData) {
        try {
            SimpleVideoThumbnailGenerator generator;
            return generator.loadVideo(videoData);
        } catch (...) {
            return false;
        }
    }
    
    static std::vector<std::string> getSupportedFormats() {
        return {
            "mp4", "avi", "mov", "mkv", "webm", "flv", "wmv", "m4v", "3gp", "ogv"
        };
    }
    
    static void setMemoryLimit(size_t maxMemory) {
        SimpleMemoryManager::getInstance().setMaxMemory(maxMemory);
    }
    
    static size_t getMemoryUsage() {
        return SimpleMemoryManager::getInstance().getTotalAllocated();
    }
    
    static void clearMemory() {
        SimpleMemoryManager::getInstance().resetStats();
    }
};

#ifdef __EMSCRIPTEN__
emscripten::val generateThumbnailJS(
    emscripten::val videoData,
    double timeSeconds,
    int32_t width,
    int32_t height,
    int32_t quality) {
    
    try {
        std::vector<uint8_t> videoBytes = emscripten::vecFromJSArray<uint8_t>(videoData);
        auto result = SimpleEmscriptenBindings::generateThumbnail(videoBytes, timeSeconds, width, height, quality);
        return emscripten::val(emscripten::typed_memory_view(result.size(), result.data()));
    } catch (const std::exception& e) {
        return emscripten::val::null();
    }
}

emscripten::val generateMultipleThumbnailsJS(
    emscripten::val videoData,
    const std::vector<double>& timePoints,
    int32_t width,
    int32_t height,
    int32_t quality) {
    
    try {
        std::vector<uint8_t> videoBytes = emscripten::vecFromJSArray<uint8_t>(videoData);
        auto results = SimpleEmscriptenBindings::generateMultipleThumbnails(videoBytes, timePoints, width, height, quality);
        
        emscripten::val jsResults = emscripten::val::array();
        for (size_t i = 0; i < results.size(); ++i) {
            jsResults.set(i, emscripten::val(emscripten::typed_memory_view(results[i].size(), results[i].data())));
        }
        return jsResults;
    } catch (const std::exception& e) {
        return emscripten::val::null();
    }
}

emscripten::val getVideoInfoJS(emscripten::val videoData) {
    try {
        std::vector<uint8_t> videoBytes = emscripten::vecFromJSArray<uint8_t>(videoData);
        SimpleVideoInfo info = SimpleEmscriptenBindings::getVideoInfo(videoBytes);
        
        emscripten::val jsInfo = emscripten::val::object();
        jsInfo.set("duration", info.duration);
        jsInfo.set("width", info.width);
        jsInfo.set("height", info.height);
        jsInfo.set("fps", info.fps);
        jsInfo.set("bitrate", info.bitrate);
        jsInfo.set("codec", info.codec);
        
        return jsInfo;
    } catch (const std::exception& e) {
        return emscripten::val::null();
    }
}

EMSCRIPTEN_BINDINGS(video_thumbnail_module) {
    emscripten::function("generateThumbnail", &generateThumbnailJS);
    emscripten::function("generateMultipleThumbnails", &generateMultipleThumbnailsJS);
    emscripten::function("getVideoInfo", &getVideoInfoJS);
    emscripten::function("isVideoSupported", &SimpleEmscriptenBindings::isVideoSupported);
    emscripten::function("getSupportedFormats", &SimpleEmscriptenBindings::getSupportedFormats);
    emscripten::function("setMemoryLimit", &SimpleEmscriptenBindings::setMemoryLimit);
    emscripten::function("getMemoryUsage", &SimpleEmscriptenBindings::getMemoryUsage);
    emscripten::function("clearMemory", &SimpleEmscriptenBindings::clearMemory);
    emscripten::function("initialize", &SimpleEmscriptenBindings::initialize);
    
    emscripten::register_vector<uint8_t>("vector<uint8_t>");
    emscripten::register_vector<double>("vector<double>");
    emscripten::register_vector<std::string>("vector<string>");
}
#endif