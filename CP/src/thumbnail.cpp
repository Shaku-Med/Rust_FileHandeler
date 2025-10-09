#include <cstdint>
#include <vector>
#include <string>
#include <stdexcept>
#include <memory>
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libavutil/avutil.h>
#include <libavutil/mem.h>
#include <libavutil/opt.h>
#include <libavutil/pixdesc.h>
#include <libswscale/swscale.h>
}
#include "cp/thumbnail.hpp"

namespace {
struct AvCloser {
  void operator()(AVFormatContext* ctx) const { if (ctx) avformat_close_input(&ctx); }
  void operator()(AVCodecContext* ctx) const { if (ctx) avcodec_free_context(&ctx); }
  void operator()(AVFrame* f) const { if (f) av_frame_free(&f); }
  void operator()(AVPacket* p) const { if (p) av_packet_free(&p); }
  void operator()(SwsContext* s) const { if (s) sws_freeContext(s); }
  void operator()(AVIOContext* s) const { if (s) { av_freep(&s->buffer); avio_context_free(&s); } }
};

struct BufferReader {
  const std::uint8_t* data;
  std::size_t size;
  std::size_t pos;
};

int read_packet(void* opaque, std::uint8_t* buf, int buf_size) {
  BufferReader* r = static_cast<BufferReader*>(opaque);
  if (r->pos >= r->size) return AVERROR_EOF;
  std::size_t remaining = r->size - r->pos;
  std::size_t n = remaining < static_cast<std::size_t>(buf_size) ? remaining : static_cast<std::size_t>(buf_size);
  std::memcpy(buf, r->data + r->pos, n);
  r->pos += n;
  return static_cast<int>(n);
}

int64_t seek_packet(void* opaque, int64_t offset, int whence) {
  BufferReader* r = static_cast<BufferReader*>(opaque);
  if (whence == AVSEEK_SIZE) return static_cast<int64_t>(r->size);
  int64_t newpos = -1;
  if (whence == SEEK_SET) newpos = offset;
  else if (whence == SEEK_CUR) newpos = static_cast<int64_t>(r->pos) + offset;
  else if (whence == SEEK_END) newpos = static_cast<int64_t>(r->size) + offset;
  if (newpos < 0 || static_cast<std::size_t>(newpos) > r->size) return AVERROR(EINVAL);
  r->pos = static_cast<std::size_t>(newpos);
  return newpos;
}

struct Size { int w; int h; };

Size computeTarget(int srcW, int srcH, int maxW, int maxH) {
  if (maxW <= 0 && maxH <= 0) return {srcW, srcH};
  double rw = maxW > 0 ? static_cast<double>(maxW) / srcW : 1.0;
  double rh = maxH > 0 ? static_cast<double>(maxH) / srcH : 1.0;
  double r = std::min(rw, rh);
  if (r >= 1.0) return {srcW, srcH};
  int w = static_cast<int>(srcW * r);
  int h = static_cast<int>(srcH * r);
  w = w > 0 ? w : 1;
  h = h > 0 ? h : 1;
  return {w, h};
}
}

namespace cp {

ImageBuffer extractThumbnailRGBA(const std::uint8_t* bytes, std::size_t length, const ThumbnailOptions& options) {
  BufferReader reader{bytes, length, 0};
  const int avio_buffer_size = 64 * 1024;
  std::unique_ptr<unsigned char, void(*)(void*)> avio_buffer(static_cast<unsigned char*>(av_malloc(avio_buffer_size)), av_free);
  if (!avio_buffer) throw std::runtime_error("alloc");
  std::unique_ptr<AVIOContext, AvCloser> ioctx(avio_alloc_context(avio_buffer.get(), avio_buffer_size, 0, &reader, &read_packet, nullptr, &seek_packet));
  if (!ioctx) throw std::runtime_error("ioctx");
  ioctx->seekable = AVIO_SEEKABLE_NORMAL;

  std::unique_ptr<AVFormatContext, AvCloser> fmt(avformat_alloc_context());
  if (!fmt) throw std::runtime_error("fmt");
  fmt->pb = ioctx.get();
  if (avformat_open_input(&fmt.get(), nullptr, nullptr, nullptr) < 0) throw std::runtime_error("open");
  if (avformat_find_stream_info(fmt.get(), nullptr) < 0) throw std::runtime_error("info");
  int vstream = av_find_best_stream(fmt.get(), AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
  if (vstream < 0) throw std::runtime_error("vstream");
  AVStream* st = fmt->streams[vstream];
  const AVCodec* dec = avcodec_find_decoder(st->codecpar->codec_id);
  if (!dec) throw std::runtime_error("codec");
  std::unique_ptr<AVCodecContext, AvCloser> cc(avcodec_alloc_context3(dec));
  if (!cc) throw std::runtime_error("cc");
  if (avcodec_parameters_to_context(cc.get(), st->codecpar) < 0) throw std::runtime_error("par");
  if (avcodec_open2(cc.get(), dec, nullptr) < 0) throw std::runtime_error("open2");

  int64_t target_ts = AV_NOPTS_VALUE;
  if (options.atSeconds >= 0.0) {
    int64_t ts = static_cast<int64_t>(options.atSeconds * st->time_base.den / st->time_base.num);
    if (av_seek_frame(fmt.get(), vstream, ts, options.exactFrame ? AVSEEK_FLAG_BACKWARD : AVSEEK_FLAG_ANY) >= 0) {
      avcodec_flush_buffers(cc.get());
    }
    target_ts = ts;
  }

  std::unique_ptr<AVFrame, AvCloser> frame(av_frame_alloc());
  std::unique_ptr<AVPacket, AvCloser> pkt(av_packet_alloc());
  bool got = false;
  int srcW = 0, srcH = 0;
  while (av_read_frame(fmt.get(), pkt.get()) >= 0) {
    if (pkt->stream_index != vstream) { av_packet_unref(pkt.get()); continue; }
    if (avcodec_send_packet(cc.get(), pkt.get()) == 0) {
      av_packet_unref(pkt.get());
      while (true) {
        int r = avcodec_receive_frame(cc.get(), frame.get());
        if (r == AVERROR(EAGAIN) || r == AVERROR_EOF) break;
        if (r < 0) break;
        if (frame->best_effort_timestamp != AV_NOPTS_VALUE && target_ts != AV_NOPTS_VALUE && frame->best_effort_timestamp < target_ts && options.exactFrame) continue;
        srcW = frame->width; srcH = frame->height;
        got = true; break;
      }
      if (got) break;
    } else {
      av_packet_unref(pkt.get());
    }
  }
  if (!got) throw std::runtime_error("decode");

  Size tgt = computeTarget(srcW, srcH, options.maxWidth, options.maxHeight);
  AVPixelFormat dstFmt = AV_PIX_FMT_RGBA;
  std::unique_ptr<SwsContext, AvCloser> sws(sws_getContext(srcW, srcH, static_cast<AVPixelFormat>(frame->format), tgt.w, tgt.h, dstFmt, SWS_BILINEAR, nullptr, nullptr, nullptr));
  if (!sws) throw std::runtime_error("sws");
  std::vector<std::uint8_t> out(tgt.w * tgt.h * 4);
  std::uint8_t* dstData[4] = { out.data(), nullptr, nullptr, nullptr };
  int dstLinesize[4] = { tgt.w * 4, 0, 0, 0 };
  sws_scale(sws.get(), frame->data, frame->linesize, 0, srcH, dstData, dstLinesize);

  ImageBuffer img;
  img.data = std::move(out);
  img.width = tgt.w;
  img.height = tgt.h;
  img.stride = tgt.w * 4;
  img.channels = 4;
  img.mime = "image/raw+rgba";
  return img;
}

}
