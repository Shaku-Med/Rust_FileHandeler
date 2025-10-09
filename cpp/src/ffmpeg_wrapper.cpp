#include "ffmpeg_wrapper.h"
#include "memory_manager.h"
#include <stdexcept>
#include <cstring>

class FFmpegWrapper::Impl {
public:
    AVFormatContext* formatContext = nullptr;
    AVCodecContext* codecContext = nullptr;
    const AVCodec* codec = nullptr;
    int videoStreamIndex = -1;
    AVFrame* frame = nullptr;
    AVPacket* packet = nullptr;
    SwsContext* swsContext = nullptr;
    bool initialized = false;
    
    Impl() = default;
    
    ~Impl() {
        cleanup();
    }
    
    void cleanup() {
        if (swsContext) {
            sws_freeContext(swsContext);
            swsContext = nullptr;
        }
        
        if (frame) {
            av_frame_free(&frame);
        }
        
        if (packet) {
            av_packet_free(&packet);
        }
        
        if (codecContext) {
            avcodec_free_context(&codecContext);
        }
        
        if (formatContext) {
            avformat_close_input(&formatContext);
        }
        
        initialized = false;
    }
};

FFmpegWrapper::FFmpegWrapper() : pImpl(std::make_unique<Impl>()) {}

FFmpegWrapper::~FFmpegWrapper() = default;

bool FFmpegWrapper::initialize() {
    if (pImpl->initialized) {
        return true;
    }
    
    av_log_set_level(AV_LOG_ERROR);
    
    pImpl->frame = av_frame_alloc();
    pImpl->packet = av_packet_alloc();
    
    if (!pImpl->frame || !pImpl->packet) {
        return false;
    }
    
    pImpl->initialized = true;
    return true;
}

void FFmpegWrapper::cleanup() {
    pImpl->cleanup();
}

bool FFmpegWrapper::openVideo(const std::vector<uint8_t>& videoData) {
    if (!pImpl->initialized) {
        return false;
    }
    
    cleanup();
    
    AVIOContext* ioContext = avio_alloc_context(
        const_cast<unsigned char*>(videoData.data()),
        static_cast<int>(videoData.size()),
        0,
        nullptr,
        nullptr,
        nullptr,
        nullptr
    );
    
    if (!ioContext) {
        return false;
    }
    
    pImpl->formatContext = avformat_alloc_context();
    if (!pImpl->formatContext) {
        av_free(ioContext);
        return false;
    }
    
    pImpl->formatContext->pb = ioContext;
    
    if (avformat_open_input(&pImpl->formatContext, nullptr, nullptr, nullptr) < 0) {
        av_free(ioContext);
        return false;
    }
    
    if (avformat_find_stream_info(pImpl->formatContext, nullptr) < 0) {
        return false;
    }
    
    pImpl->videoStreamIndex = av_find_best_stream(
        pImpl->formatContext,
        AVMEDIA_TYPE_VIDEO,
        -1,
        -1,
        &pImpl->codec,
        0
    );
    
    if (pImpl->videoStreamIndex < 0 || !pImpl->codec) {
        return false;
    }
    
    pImpl->codecContext = avcodec_alloc_context3(pImpl->codec);
    if (!pImpl->codecContext) {
        return false;
    }
    
    if (avcodec_parameters_to_context(
        pImpl->codecContext,
        pImpl->formatContext->streams[pImpl->videoStreamIndex]->codecpar
    ) < 0) {
        return false;
    }
    
    if (avcodec_open2(pImpl->codecContext, pImpl->codec, nullptr) < 0) {
        return false;
    }
    
    return true;
}

bool FFmpegWrapper::openVideoFromFile(const std::string& filePath) {
    if (!pImpl->initialized) {
        return false;
    }
    
    cleanup();
    
    if (avformat_open_input(&pImpl->formatContext, filePath.c_str(), nullptr, nullptr) < 0) {
        return false;
    }
    
    if (avformat_find_stream_info(pImpl->formatContext, nullptr) < 0) {
        return false;
    }
    
    pImpl->videoStreamIndex = av_find_best_stream(
        pImpl->formatContext,
        AVMEDIA_TYPE_VIDEO,
        -1,
        -1,
        &pImpl->codec,
        0
    );
    
    if (pImpl->videoStreamIndex < 0 || !pImpl->codec) {
        return false;
    }
    
    pImpl->codecContext = avcodec_alloc_context3(pImpl->codec);
    if (!pImpl->codecContext) {
        return false;
    }
    
    if (avcodec_parameters_to_context(
        pImpl->codecContext,
        pImpl->formatContext->streams[pImpl->videoStreamIndex]->codecpar
    ) < 0) {
        return false;
    }
    
    if (avcodec_open2(pImpl->codecContext, pImpl->codec, nullptr) < 0) {
        return false;
    }
    
    return true;
}

VideoInfo FFmpegWrapper::getVideoInfo() const {
    if (!pImpl->formatContext || !pImpl->codecContext) {
        throw std::runtime_error("Video not loaded");
    }
    
    VideoInfo info;
    info.duration = static_cast<double>(pImpl->formatContext->duration) / AV_TIME_BASE;
    info.width = pImpl->codecContext->width;
    info.height = pImpl->codecContext->height;
    info.fps = static_cast<double>(pImpl->codecContext->framerate.num) / pImpl->codecContext->framerate.den;
    info.bitrate = pImpl->formatContext->bit_rate;
    info.codec = pImpl->codec->name;
    
    return info;
}

FrameData FFmpegWrapper::extractFrameAtTime(double timeSeconds) {
    if (!pImpl->formatContext || !pImpl->codecContext) {
        throw std::runtime_error("Video not loaded");
    }
    
    int64_t timestamp = static_cast<int64_t>(timeSeconds * AV_TIME_BASE);
    return extractFrameAtTimestamp(timestamp);
}

FrameData FFmpegWrapper::extractFrameAtTimestamp(int64_t timestamp) {
    if (!pImpl->formatContext || !pImpl->codecContext) {
        throw std::runtime_error("Video not loaded");
    }
    
    if (av_seek_frame(pImpl->formatContext, -1, timestamp, AVSEEK_FLAG_BACKWARD) < 0) {
        throw std::runtime_error("Failed to seek to timestamp");
    }
    
    avcodec_flush_buffers(pImpl->codecContext);
    
    while (av_read_frame(pImpl->formatContext, pImpl->packet) >= 0) {
        if (pImpl->packet->stream_index == pImpl->videoStreamIndex) {
            int ret = avcodec_send_packet(pImpl->codecContext, pImpl->packet);
            if (ret < 0) {
                av_packet_unref(pImpl->packet);
                continue;
            }
            
            ret = avcodec_receive_frame(pImpl->codecContext, pImpl->frame);
            if (ret == 0) {
                FrameData frameData;
                frameData.width = pImpl->frame->width;
                frameData.height = pImpl->frame->height;
                frameData.format = static_cast<AVPixelFormat>(pImpl->frame->format);
                
                if (!pImpl->swsContext) {
                    pImpl->swsContext = sws_getContext(
                        pImpl->frame->width,
                        pImpl->frame->height,
                        static_cast<AVPixelFormat>(pImpl->frame->format),
                        pImpl->frame->width,
                        pImpl->frame->height,
                        AV_PIX_FMT_RGB24,
                        SWS_BILINEAR,
                        nullptr,
                        nullptr,
                        nullptr
                    );
                }
                
                if (pImpl->swsContext) {
                    AVFrame* rgbFrame = av_frame_alloc();
                    rgbFrame->format = AV_PIX_FMT_RGB24;
                    rgbFrame->width = pImpl->frame->width;
                    rgbFrame->height = pImpl->frame->height;
                    av_frame_get_buffer(rgbFrame, 0);
                    
                    sws_scale(
                        pImpl->swsContext,
                        pImpl->frame->data,
                        pImpl->frame->linesize,
                        0,
                        pImpl->frame->height,
                        rgbFrame->data,
                        rgbFrame->linesize
                    );
                    
                    int dataSize = rgbFrame->linesize[0] * rgbFrame->height;
                    frameData.data.resize(dataSize);
                    std::memcpy(frameData.data.data(), rgbFrame->data[0], dataSize);
                    
                    av_frame_free(&rgbFrame);
                }
                
                av_packet_unref(pImpl->packet);
                return frameData;
            }
        }
        av_packet_unref(pImpl->packet);
    }
    
    throw std::runtime_error("Failed to extract frame");
}

bool FFmpegWrapper::seekToTime(double timeSeconds) {
    if (!pImpl->formatContext) {
        return false;
    }
    
    int64_t timestamp = static_cast<int64_t>(timeSeconds * AV_TIME_BASE);
    return seekToTimestamp(timestamp);
}

bool FFmpegWrapper::seekToTimestamp(int64_t timestamp) {
    if (!pImpl->formatContext) {
        return false;
    }
    
    return av_seek_frame(pImpl->formatContext, -1, timestamp, AVSEEK_FLAG_BACKWARD) >= 0;
}

void FFmpegWrapper::close() {
    cleanup();
}