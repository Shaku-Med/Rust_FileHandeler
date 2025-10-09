#include "emscripten_bindings.h"
#include "video_thumbnail_generator.h"
#include "memory_manager.h"
#include <stdexcept>

#ifdef __EMSCRIPTEN__
#include <emscripten/bind.h>
#include <emscripten/val.h>
#endif

void EmscriptenBindings::initialize() {
    MemoryManager::getInstance().setMaxMemory(256 * 1024 * 1024);
}

std::vector<uint8_t> EmscriptenBindings::generateThumbnail(
    const std::vector<uint8_t>& videoData,
    double timeSeconds,
    int32_t width,
    int32_t height,
    int32_t quality) {
    
    VideoThumbnailGenerator generator;
    
    if (!generator.loadVideo(videoData)) {
        throw std::runtime_error("Failed to load video");
    }
    
    ThumbnailOptions options;
    options.width = width;
    options.height = height;
    options.quality = quality;
    
    return generator.generateThumbnail(timeSeconds, options);
}

std::vector<std::vector<uint8_t>> EmscriptenBindings::generateMultipleThumbnails(
    const std::vector<uint8_t>& videoData,
    const std::vector<double>& timePoints,
    int32_t width,
    int32_t height,
    int32_t quality) {
    
    VideoThumbnailGenerator generator;
    
    if (!generator.loadVideo(videoData)) {
        throw std::runtime_error("Failed to load video");
    }
    
    ThumbnailOptions options;
    options.width = width;
    options.height = height;
    options.quality = quality;
    
    return generator.generateMultipleThumbnails(timePoints, options);
}

VideoInfo EmscriptenBindings::getVideoInfo(const std::vector<uint8_t>& videoData) {
    VideoThumbnailGenerator generator;
    
    if (!generator.loadVideo(videoData)) {
        throw std::runtime_error("Failed to load video");
    }
    
    return generator.getVideoInfo();
}

bool EmscriptenBindings::isVideoSupported(const std::vector<uint8_t>& videoData) {
    try {
        VideoThumbnailGenerator generator;
        return generator.loadVideo(videoData);
    } catch (...) {
        return false;
    }
}

std::vector<std::string> EmscriptenBindings::getSupportedFormats() {
    return {
        "mp4", "avi", "mov", "mkv", "webm", "flv", "wmv", "m4v", "3gp", "ogv"
    };
}

void EmscriptenBindings::setMemoryLimit(size_t maxMemory) {
    MemoryManager::getInstance().setMaxMemory(maxMemory);
}

size_t EmscriptenBindings::getMemoryUsage() {
    return MemoryManager::getInstance().getTotalAllocated();
}

void EmscriptenBindings::clearMemory() {
    MemoryManager::getInstance().resetStats();
}

#ifdef __EMSCRIPTEN__
emscripten::val EmscriptenBindings::generateThumbnailJS(
    emscripten::val videoData,
    double timeSeconds,
    int32_t width,
    int32_t height,
    int32_t quality) {
    
    try {
        std::vector<uint8_t> videoBytes = emscripten::vecFromJSArray<uint8_t>(videoData);
        auto result = generateThumbnail(videoBytes, timeSeconds, width, height, quality);
        return emscripten::val(emscripten::typed_memory_view(result.size(), result.data()));
    } catch (const std::exception& e) {
        return emscripten::val::null();
    }
}

emscripten::val EmscriptenBindings::generateMultipleThumbnailsJS(
    emscripten::val videoData,
    const std::vector<double>& timePoints,
    int32_t width,
    int32_t height,
    int32_t quality) {
    
    try {
        std::vector<uint8_t> videoBytes = emscripten::vecFromJSArray<uint8_t>(videoData);
        auto results = generateMultipleThumbnails(videoBytes, timePoints, width, height, quality);
        
        emscripten::val jsResults = emscripten::val::array();
        for (size_t i = 0; i < results.size(); ++i) {
            jsResults.set(i, emscripten::val(emscripten::typed_memory_view(results[i].size(), results[i].data())));
        }
        return jsResults;
    } catch (const std::exception& e) {
        return emscripten::val::null();
    }
}

emscripten::val EmscriptenBindings::getVideoInfoJS(emscripten::val videoData) {
    try {
        std::vector<uint8_t> videoBytes = emscripten::vecFromJSArray<uint8_t>(videoData);
        VideoInfo info = getVideoInfo(videoBytes);
        
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
    emscripten::function("generateThumbnail", &EmscriptenBindings::generateThumbnailJS);
    emscripten::function("generateMultipleThumbnails", &EmscriptenBindings::generateMultipleThumbnailsJS);
    emscripten::function("getVideoInfo", &EmscriptenBindings::getVideoInfoJS);
    emscripten::function("isVideoSupported", &EmscriptenBindings::isVideoSupported);
    emscripten::function("getSupportedFormats", &EmscriptenBindings::getSupportedFormats);
    emscripten::function("setMemoryLimit", &EmscriptenBindings::setMemoryLimit);
    emscripten::function("getMemoryUsage", &EmscriptenBindings::getMemoryUsage);
    emscripten::function("clearMemory", &EmscriptenBindings::clearMemory);
    emscripten::function("initialize", &EmscriptenBindings::initialize);
    
    emscripten::register_vector<uint8_t>("vector<uint8_t>");
    emscripten::register_vector<double>("vector<double>");
    emscripten::register_vector<std::string>("vector<string>");
}
#endif