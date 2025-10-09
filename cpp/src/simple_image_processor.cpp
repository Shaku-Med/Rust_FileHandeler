#include <vector>
#include <cstdint>
#include <algorithm>
#include <cmath>

struct SimpleImageData {
    std::vector<uint8_t> data;
    int32_t width;
    int32_t height;
    int32_t channels;
};

class SimpleImageProcessor {
public:
    static SimpleImageData resizeImage(
        const std::vector<uint8_t>& inputData,
        int32_t inputWidth,
        int32_t inputHeight,
        int32_t outputWidth,
        int32_t outputHeight,
        bool maintainAspectRatio = true) {
        
        int32_t finalWidth, finalHeight;
        calculateAspectRatio(inputWidth, inputHeight, outputWidth, outputHeight, 
                            maintainAspectRatio, finalWidth, finalHeight);
        
        SimpleImageData result;
        result.width = finalWidth;
        result.height = finalHeight;
        result.channels = 3;
        result.data.resize(finalWidth * finalHeight * 3);
        
        float xRatio = static_cast<float>(inputWidth) / finalWidth;
        float yRatio = static_cast<float>(inputHeight) / finalHeight;
        
        for (int32_t y = 0; y < finalHeight; ++y) {
            for (int32_t x = 0; x < finalWidth; ++x) {
                int32_t srcX = static_cast<int32_t>(x * xRatio);
                int32_t srcY = static_cast<int32_t>(y * yRatio);
                
                srcX = std::min(srcX, inputWidth - 1);
                srcY = std::min(srcY, inputHeight - 1);
                
                int32_t srcIndex = (srcY * inputWidth + srcX) * 3;
                int32_t dstIndex = (y * finalWidth + x) * 3;
                
                result.data[dstIndex] = inputData[srcIndex];
                result.data[dstIndex + 1] = inputData[srcIndex + 1];
                result.data[dstIndex + 2] = inputData[srcIndex + 2];
            }
        }
        
        return result;
    }
    
    static std::vector<uint8_t> encodeJPEG(
        const SimpleImageData& imageData,
        int32_t quality = 85) {
        
        quality = std::clamp(quality, 1, 100);
        
        std::vector<uint8_t> result;
        result.reserve(imageData.width * imageData.height * 3 / 10);
        
        result.push_back(0xFF);
        result.push_back(0xD8);
        
        result.push_back(0xFF);
        result.push_back(0xE0);
        result.push_back(0x00);
        result.push_back(0x10);
        result.push_back(0x4A);
        result.push_back(0x46);
        result.push_back(0x49);
        result.push_back(0x46);
        result.push_back(0x00);
        result.push_back(0x01);
        result.push_back(0x01);
        result.push_back(0x01);
        result.push_back(0x00);
        result.push_back(0x48);
        result.push_back(0x00);
        result.push_back(0x48);
        result.push_back(0x00);
        result.push_back(0x00);
        
        result.push_back(0xFF);
        result.push_back(0xDB);
        result.push_back(0x00);
        result.push_back(0x43);
        result.push_back(0x00);
        
        for (int i = 0; i < 64; ++i) {
            result.push_back(static_cast<uint8_t>(i * 4));
        }
        
        result.push_back(0xFF);
        result.push_back(0xC0);
        result.push_back(0x00);
        result.push_back(0x11);
        result.push_back(0x08);
        result.push_back(static_cast<uint8_t>((imageData.height >> 8) & 0xFF));
        result.push_back(static_cast<uint8_t>(imageData.height & 0xFF));
        result.push_back(static_cast<uint8_t>((imageData.width >> 8) & 0xFF));
        result.push_back(static_cast<uint8_t>(imageData.width & 0xFF));
        result.push_back(0x03);
        result.push_back(0x01);
        result.push_back(0x11);
        result.push_back(0x00);
        result.push_back(0x02);
        result.push_back(0x11);
        result.push_back(0x01);
        result.push_back(0x03);
        result.push_back(0x11);
        result.push_back(0x01);
        
        result.push_back(0xFF);
        result.push_back(0xDA);
        result.push_back(0x00);
        result.push_back(0x0C);
        result.push_back(0x03);
        result.push_back(0x01);
        result.push_back(0x00);
        result.push_back(0x02);
        result.push_back(0x11);
        result.push_back(0x03);
        result.push_back(0x11);
        result.push_back(0x00);
        result.push_back(0x3F);
        result.push_back(0x00);
        
        for (size_t i = 0; i < imageData.data.size(); i += 3) {
            result.push_back(imageData.data[i]);
            result.push_back(imageData.data[i + 1]);
            result.push_back(imageData.data[i + 2]);
        }
        
        result.push_back(0xFF);
        result.push_back(0xD9);
        
        return result;
    }

private:
    static void calculateAspectRatio(
        int32_t inputWidth,
        int32_t inputHeight,
        int32_t outputWidth,
        int32_t outputHeight,
        bool maintainAspectRatio,
        int32_t& finalWidth,
        int32_t& finalHeight) {
        
        if (!maintainAspectRatio) {
            finalWidth = outputWidth;
            finalHeight = outputHeight;
            return;
        }
        
        float inputAspect = static_cast<float>(inputWidth) / inputHeight;
        float outputAspect = static_cast<float>(outputWidth) / outputHeight;
        
        if (inputAspect > outputAspect) {
            finalWidth = outputWidth;
            finalHeight = static_cast<int32_t>(outputWidth / inputAspect);
        } else {
            finalHeight = outputHeight;
            finalWidth = static_cast<int32_t>(outputHeight * inputAspect);
        }
    }
};