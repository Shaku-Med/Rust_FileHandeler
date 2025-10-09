#pragma once

#include <vector>
#include <memory>
#include <cstdint>

extern "C" {
    #include <libavformat/avformat.h>
    #include <libavcodec/avcodec.h>
    #include <libavutil/avutil.h>
    #include <libswscale/swscale.h>
}

struct FrameData {
    std::vector<uint8_t> data;
    int32_t width;
    int32_t height;
    AVPixelFormat format;
};

class FFmpegWrapper {
public:
    FFmpegWrapper();
    ~FFmpegWrapper();

    bool initialize();
    void cleanup();
    
    bool openVideo(const std::vector<uint8_t>& videoData);
    bool openVideoFromFile(const std::string& filePath);
    
    VideoInfo getVideoInfo() const;
    FrameData extractFrameAtTime(double timeSeconds);
    FrameData extractFrameAtTimestamp(int64_t timestamp);
    
    bool seekToTime(double timeSeconds);
    bool seekToTimestamp(int64_t timestamp);
    
    void close();

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};