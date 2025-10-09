#include "video_thumbnail_generator.h"
#include "ffmpeg_wrapper.h"
#include "image_processor.h"
#include "memory_manager.h"
#include <stdexcept>
#include <algorithm>

class VideoThumbnailGenerator::Impl {
public:
    FFmpegWrapper ffmpegWrapper;
    std::vector<uint8_t> videoData;
    bool isLoaded = false;
    
    Impl() = default;
    
    ~Impl() {
        if (isLoaded) {
            ffmpegWrapper.close();
        }
    }
};

VideoThumbnailGenerator::VideoThumbnailGenerator() 
    : pImpl(std::make_unique<Impl>()) {
    pImpl->ffmpegWrapper.initialize();
}

VideoThumbnailGenerator::~VideoThumbnailGenerator() = default;

bool VideoThumbnailGenerator::loadVideo(const std::vector<uint8_t>& videoData) {
    if (videoData.empty()) {
        return false;
    }
    
    pImpl->videoData = videoData;
    
    if (pImpl->ffmpegWrapper.openVideo(videoData)) {
        pImpl->isLoaded = true;
        return true;
    }
    
    return false;
}

bool VideoThumbnailGenerator::loadVideoFromFile(const std::string& filePath) {
    if (filePath.empty()) {
        return false;
    }
    
    if (pImpl->ffmpegWrapper.openVideoFromFile(filePath)) {
        pImpl->isLoaded = true;
        return true;
    }
    
    return false;
}

std::vector<uint8_t> VideoThumbnailGenerator::generateThumbnail(
    double timeSeconds, 
    const ThumbnailOptions& options) {
    
    if (!pImpl->isLoaded) {
        throw std::runtime_error("No video loaded");
    }
    
    auto frameData = pImpl->ffmpegWrapper.extractFrameAtTime(timeSeconds);
    
    if (frameData.data.empty()) {
        throw std::runtime_error("Failed to extract frame");
    }
    
    auto resizedImage = ImageProcessor::resizeImage(
        frameData.data,
        frameData.width,
        frameData.height,
        options.width,
        options.height,
        options.maintain_aspect_ratio
    );
    
    ImageData imageData{
        resizedImage,
        options.width,
        options.height,
        3
    };
    
    if (options.format == "jpeg") {
        return ImageProcessor::encodeJPEG(imageData, options.quality);
    } else if (options.format == "png") {
        return ImageProcessor::encodePNG(imageData);
    } else if (options.format == "webp") {
        return ImageProcessor::encodeWebP(imageData, options.quality);
    }
    
    throw std::runtime_error("Unsupported format: " + options.format);
}

std::vector<std::vector<uint8_t>> VideoThumbnailGenerator::generateMultipleThumbnails(
    const std::vector<double>& timePoints,
    const ThumbnailOptions& options) {
    
    if (!pImpl->isLoaded) {
        throw std::runtime_error("No video loaded");
    }
    
    std::vector<std::vector<uint8_t>> thumbnails;
    thumbnails.reserve(timePoints.size());
    
    for (double timePoint : timePoints) {
        try {
            auto thumbnail = generateThumbnail(timePoint, options);
            thumbnails.push_back(std::move(thumbnail));
        } catch (const std::exception& e) {
            thumbnails.push_back({});
        }
    }
    
    return thumbnails;
}

VideoInfo VideoThumbnailGenerator::getVideoInfo() const {
    if (!pImpl->isLoaded) {
        throw std::runtime_error("No video loaded");
    }
    
    return pImpl->ffmpegWrapper.getVideoInfo();
}

bool VideoThumbnailGenerator::isVideoLoaded() const {
    return pImpl->isLoaded;
}

void VideoThumbnailGenerator::clear() {
    if (pImpl->isLoaded) {
        pImpl->ffmpegWrapper.close();
        pImpl->isLoaded = false;
    }
    pImpl->videoData.clear();
}