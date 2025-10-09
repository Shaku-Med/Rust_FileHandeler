#pragma once

#include <vector>
#include <memory>
#include <string>
#include <cstdint>

struct VideoInfo {
    double duration;
    int32_t width;
    int32_t height;
    double fps;
    int64_t bitrate;
    std::string codec;
};

struct ThumbnailOptions {
    int32_t width = 320;
    int32_t height = 240;
    int32_t quality = 85;
    bool maintain_aspect_ratio = true;
    std::string format = "jpeg";
};

class VideoThumbnailGenerator {
public:
    VideoThumbnailGenerator();
    ~VideoThumbnailGenerator();

    bool loadVideo(const std::vector<uint8_t>& videoData);
    bool loadVideoFromFile(const std::string& filePath);
    
    std::vector<uint8_t> generateThumbnail(double timeSeconds, const ThumbnailOptions& options = {});
    std::vector<std::vector<uint8_t>> generateMultipleThumbnails(
        const std::vector<double>& timePoints, 
        const ThumbnailOptions& options = {}
    );
    
    VideoInfo getVideoInfo() const;
    bool isVideoLoaded() const;
    
    void clear();

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};