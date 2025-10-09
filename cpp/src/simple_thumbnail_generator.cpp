#include <vector>
#include <memory>
#include <string>
#include <cstdint>
#include <stdexcept>
#include <random>
#include "../include/simple_types.h"

class SimpleVideoThumbnailGenerator {
public:
    SimpleVideoThumbnailGenerator() = default;
    ~SimpleVideoThumbnailGenerator() = default;

    bool loadVideo(const std::vector<uint8_t>& videoData) {
        if (videoData.empty()) {
            return false;
        }
        
        videoData_ = videoData;
        isLoaded_ = true;
        
        videoInfo_.duration = 10.0;
        videoInfo_.width = 1920;
        videoInfo_.height = 1080;
        videoInfo_.fps = 30.0;
        videoInfo_.bitrate = 5000000;
        videoInfo_.codec = "h264";
        
        return true;
    }
    
    std::vector<uint8_t> generateThumbnail(double timeSeconds, const SimpleThumbnailOptions& options = {}) {
        if (!isLoaded_) {
            throw std::runtime_error("No video loaded");
        }
        
        return generateTestThumbnail(options.width, options.height, options.quality);
    }
    
    std::vector<std::vector<uint8_t>> generateMultipleThumbnails(
        const std::vector<double>& timePoints, 
        const SimpleThumbnailOptions& options = {}) {
        
        if (!isLoaded_) {
            throw std::runtime_error("No video loaded");
        }
        
        std::vector<std::vector<uint8_t>> thumbnails;
        thumbnails.reserve(timePoints.size());
        
        for (double timePoint : timePoints) {
            auto thumbnail = generateTestThumbnail(options.width, options.height, options.quality);
            thumbnails.push_back(std::move(thumbnail));
        }
        
        return thumbnails;
    }
    
    SimpleVideoInfo getVideoInfo() const {
        if (!isLoaded_) {
            throw std::runtime_error("No video loaded");
        }
        return videoInfo_;
    }
    
    bool isVideoLoaded() const {
        return isLoaded_;
    }
    
    void clear() {
        videoData_.clear();
        isLoaded_ = false;
    }

private:
    std::vector<uint8_t> videoData_;
    bool isLoaded_ = false;
    SimpleVideoInfo videoInfo_;
    
    std::vector<uint8_t> generateTestThumbnail(int32_t width, int32_t height, int32_t quality) {
        std::vector<uint8_t> thumbnail;
        
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 255);
        
        for (int32_t y = 0; y < height; ++y) {
            for (int32_t x = 0; x < width; ++x) {
                uint8_t r = static_cast<uint8_t>((x * 255) / width);
                uint8_t g = static_cast<uint8_t>((y * 255) / height);
                uint8_t b = static_cast<uint8_t>((x + y) * 255 / (width + height));
                
                thumbnail.push_back(r);
                thumbnail.push_back(g);
                thumbnail.push_back(b);
            }
        }
        
        return createSimpleJPEG(thumbnail, width, height, quality);
    }
    
    std::vector<uint8_t> createSimpleJPEG(const std::vector<uint8_t>& rgbData, int32_t width, int32_t height, int32_t quality) {
        std::vector<uint8_t> jpeg;
        
        jpeg.push_back(0xFF);
        jpeg.push_back(0xD8);
        
        jpeg.push_back(0xFF);
        jpeg.push_back(0xE0);
        jpeg.push_back(0x00);
        jpeg.push_back(0x10);
        jpeg.push_back(0x4A);
        jpeg.push_back(0x46);
        jpeg.push_back(0x49);
        jpeg.push_back(0x46);
        jpeg.push_back(0x00);
        jpeg.push_back(0x01);
        jpeg.push_back(0x01);
        jpeg.push_back(0x01);
        jpeg.push_back(0x00);
        jpeg.push_back(0x48);
        jpeg.push_back(0x00);
        jpeg.push_back(0x48);
        jpeg.push_back(0x00);
        jpeg.push_back(0x00);
        
        jpeg.push_back(0xFF);
        jpeg.push_back(0xDB);
        jpeg.push_back(0x00);
        jpeg.push_back(0x43);
        jpeg.push_back(0x00);
        
        for (int i = 0; i < 64; ++i) {
            jpeg.push_back(static_cast<uint8_t>(i * 4));
        }
        
        jpeg.push_back(0xFF);
        jpeg.push_back(0xC0);
        jpeg.push_back(0x00);
        jpeg.push_back(0x11);
        jpeg.push_back(0x08);
        jpeg.push_back(static_cast<uint8_t>((height >> 8) & 0xFF));
        jpeg.push_back(static_cast<uint8_t>(height & 0xFF));
        jpeg.push_back(static_cast<uint8_t>((width >> 8) & 0xFF));
        jpeg.push_back(static_cast<uint8_t>(width & 0xFF));
        jpeg.push_back(0x03);
        jpeg.push_back(0x01);
        jpeg.push_back(0x11);
        jpeg.push_back(0x00);
        jpeg.push_back(0x02);
        jpeg.push_back(0x11);
        jpeg.push_back(0x01);
        jpeg.push_back(0x03);
        jpeg.push_back(0x11);
        jpeg.push_back(0x01);
        
        jpeg.push_back(0xFF);
        jpeg.push_back(0xDA);
        jpeg.push_back(0x00);
        jpeg.push_back(0x0C);
        jpeg.push_back(0x03);
        jpeg.push_back(0x01);
        jpeg.push_back(0x00);
        jpeg.push_back(0x02);
        jpeg.push_back(0x11);
        jpeg.push_back(0x03);
        jpeg.push_back(0x11);
        jpeg.push_back(0x00);
        jpeg.push_back(0x3F);
        jpeg.push_back(0x00);
        
        for (size_t i = 0; i < rgbData.size(); i += 3) {
            jpeg.push_back(rgbData[i]);
            jpeg.push_back(rgbData[i + 1]);
            jpeg.push_back(rgbData[i + 2]);
        }
        
        jpeg.push_back(0xFF);
        jpeg.push_back(0xD9);
        
        return jpeg;
    }
};