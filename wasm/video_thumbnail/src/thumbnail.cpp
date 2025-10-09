#include "video_thumbnail/thumbnail.hpp"
#include "ffmpeg_memory_io.hpp"
#include <stdexcept>
#include <string>
#include <vector>
#include <memory>
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
}
#include <turbojpeg.h>

namespace vthumb {
static inline void ensure(int cond, const char* msg) {
    if (!cond) throw std::runtime_error(msg);
}

static inline AVCodecContext* open_decoder(AVStream* stream) {
    const AVCodec* dec = avcodec_find_decoder(stream->codecpar->codec_id);
    ensure(dec != nullptr, "decoder not found");
    AVCodecContext* ctx = avcodec_alloc_context3(dec);
    ensure(ctx != nullptr, "codec ctx alloc failed");
    ensure(avcodec_parameters_to_context(ctx, stream->codecpar) >= 0, "params to ctx failed");
    AVDictionary* opts = nullptr;
    ensure(avcodec_open2(ctx, dec, &opts) >= 0, "open decoder failed");
    return ctx;
}

static inline ImageBuffer encode_jpeg_rgb8(uint8_t* rgb, int width, int height, int quality) {
    tjhandle tj = tjInitCompress();
    ensure(tj != nullptr, "tjInitCompress failed");
    unsigned long jpegSize = 0;
    unsigned char* jpegBuf = nullptr;
    int subsamp = TJSAMP_444;
    int flags = TJFLAG_FASTDCT;
    int rc = tjCompress2(tj, rgb, width, 0, height, TJPF_RGB, &jpegBuf, &jpegSize, subsamp, quality, flags);
    ensure(rc == 0, "jpeg compress failed");
    ImageBuffer out;
    out.data.assign(jpegBuf, jpegBuf + jpegSize);
    out.width = width;
    out.height = height;
    out.channels = 3;
    out.mime = "image/jpeg";
    tjFree(jpegBuf);
    tjDestroy(tj);
    return out;
}

ImageBuffer extract_thumbnail_from_bytes(const uint8_t* bytes, size_t length, const DecodeOptions& options) {
    AVFormatContext* fmt = nullptr;
    AVIOContext* io_ctx = nullptr;
    uint8_t* io_buf = nullptr;
    ensure(open_memory_io(&fmt, bytes, length, &io_ctx, &io_buf) == 0, "open io failed");
    ensure(avformat_open_input(&fmt, nullptr, nullptr, nullptr) >= 0, "open input failed");
    ensure(avformat_find_stream_info(fmt, nullptr) >= 0, "find stream info failed");
    int vindex = av_find_best_stream(fmt, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    ensure(vindex >= 0, "no video stream");
    AVStream* vs = fmt->streams[vindex];
    AVCodecContext* dec = open_decoder(vs);

    double seekTs = options.timestampSeconds;
    if (seekTs > 0) {
        int64_t ts = static_cast<int64_t>(seekTs / av_q2d(vs->time_base));
        av_seek_frame(fmt, vindex, ts, options.exactSeek ? AVSEEK_FLAG_BACKWARD : 0);
        avcodec_flush_buffers(dec);
    }

    int targetW = options.targetWidth > 0 ? options.targetWidth : dec->width;
    int targetH = options.targetHeight > 0 ? options.targetHeight : dec->height;
    if (options.preserveAspect) {
        if (options.targetWidth > 0 && options.targetHeight > 0) {
            double ar = static_cast<double>(dec->width) / static_cast<double>(dec->height);
            int w = options.targetWidth;
            int h = static_cast<int>(w / ar);
            if (h > options.targetHeight) {
                h = options.targetHeight;
                w = static_cast<int>(h * ar);
            }
            targetW = w;
            targetH = h;
        }
    }
    if (!options.allowUpscale) {
        if (targetW > dec->width) targetW = dec->width;
        if (targetH > dec->height) targetH = dec->height;
    }

    SwsContext* sws = sws_getContext(dec->width, dec->height, dec->pix_fmt, targetW, targetH, AV_PIX_FMT_RGB24, SWS_BICUBIC, nullptr, nullptr, nullptr);
    ensure(sws != nullptr, "sws context failed");

    std::unique_ptr<uint8_t, void(*)(void*)> rgbBuf(static_cast<uint8_t*>(av_malloc(targetW * targetH * 3)), av_free);
    ensure(rgbBuf != nullptr, "rgb alloc failed");

    AVFrame* frame = av_frame_alloc();
    AVFrame* rgb = av_frame_alloc();
    ensure(frame && rgb, "frame alloc failed");
    int rgb_linesize[4] = { targetW * 3, 0, 0, 0 };
    uint8_t* rgb_data[4] = { rgbBuf.get(), nullptr, nullptr, nullptr };

    AVPacket* pkt = av_packet_alloc();
    ensure(pkt != nullptr, "packet alloc failed");

    ImageBuffer result;
    bool got = false;

    while (av_read_frame(fmt, pkt) >= 0) {
        if (pkt->stream_index != vindex) { av_packet_unref(pkt); continue; }
        ensure(avcodec_send_packet(dec, pkt) >= 0, "send pkt failed");
        av_packet_unref(pkt);
        while (true) {
            int r = avcodec_receive_frame(dec, frame);
            if (r == AVERROR(EAGAIN) || r == AVERROR_EOF) break;
            ensure(r >= 0, "receive frame failed");
            const uint8_t* src_slices[4] = { frame->data[0], frame->data[1], frame->data[2], frame->data[3] };
            int src_linesize[4] = { frame->linesize[0], frame->linesize[1], frame->linesize[2], frame->linesize[3] };
            sws_scale(sws, src_slices, src_linesize, 0, dec->height, rgb_data, rgb_linesize);
            result = encode_jpeg_rgb8(rgbBuf.get(), targetW, targetH, 85);
            got = true;
            break;
        }
        if (got) break;
    }

    if (!got) {
        avcodec_send_packet(dec, nullptr);
        while (!got) {
            int r = avcodec_receive_frame(dec, frame);
            if (r == AVERROR_EOF) break;
            if (r == AVERROR(EAGAIN)) continue;
            ensure(r >= 0, "drain receive failed");
            const uint8_t* src_slices[4] = { frame->data[0], frame->data[1], frame->data[2], frame->data[3] };
            int src_linesize[4] = { frame->linesize[0], frame->linesize[1], frame->linesize[2], frame->linesize[3] };
            sws_scale(sws, src_slices, src_linesize, 0, dec->height, rgb_data, rgb_linesize);
            result = encode_jpeg_rgb8(rgbBuf.get(), targetW, targetH, 85);
            got = true;
        }
    }

    sws_freeContext(sws);
    av_frame_free(&frame);
    av_frame_free(&rgb);
    av_packet_free(&pkt);
    avcodec_free_context(&dec);
    close_memory_io(fmt, io_ctx);

    ensure(got, "no frame decoded");
    return result;
}
}
