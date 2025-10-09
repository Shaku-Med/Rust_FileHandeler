#pragma once

#include <vector>
#include <cstdint>

struct ImageData {
    std::vector<uint8_t> data;
    int32_t width;
    int32_t height;
    int32_t channels;
};

class ImageProcessor {
public:
    static ImageData resizeImage(
        const std::vector<uint8_t>& inputData,
        int32_t inputWidth,
        int32_t inputHeight,
        int32_t outputWidth,
        int32_t outputHeight,
        bool maintainAspectRatio = true
    );
    
    static ImageData convertFormat(
        const std::vector<uint8_t>& inputData,
        int32_t width,
        int32_t height,
        int32_t inputChannels,
        int32_t outputChannels
    );
    
    static std::vector<uint8_t> encodeJPEG(
        const ImageData& imageData,
        int32_t quality = 85
    );
    
    static std::vector<uint8_t> encodePNG(
        const ImageData& imageData
    );
    
    static std::vector<uint8_t> encodeWebP(
        const ImageData& imageData,
        int32_t quality = 85
    );
    
    static ImageData cropImage(
        const ImageData& inputImage,
        int32_t x,
        int32_t y,
        int32_t cropWidth,
        int32_t cropHeight
    );
    
    static ImageData applyFilters(
        const ImageData& inputImage,
        float brightness = 1.0f,
        float contrast = 1.0f,
        float saturation = 1.0f
    );

private:
    static void calculateAspectRatio(
        int32_t inputWidth,
        int32_t inputHeight,
        int32_t outputWidth,
        int32_t outputHeight,
        bool maintainAspectRatio,
        int32_t& finalWidth,
        int32_t& finalHeight
    );
};