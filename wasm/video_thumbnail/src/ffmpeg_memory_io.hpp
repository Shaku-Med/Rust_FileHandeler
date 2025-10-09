#pragma once
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <cstdio>
extern "C" {
#include <libavformat/avformat.h>
}

namespace vthumb {
struct MemoryContext {
    const uint8_t* data;
    size_t size;
    size_t pos;
};

static inline int read_packet(void* opaque, uint8_t* buf, int buf_size) {
    MemoryContext* ctx = reinterpret_cast<MemoryContext*>(opaque);
    if (ctx->pos >= ctx->size) return AVERROR_EOF;
    size_t remaining = ctx->size - ctx->pos;
    size_t to_copy = remaining < static_cast<size_t>(buf_size) ? remaining : static_cast<size_t>(buf_size);
    std::memcpy(buf, ctx->data + ctx->pos, to_copy);
    ctx->pos += to_copy;
    return static_cast<int>(to_copy);
}

static inline int64_t seek_packet(void* opaque, int64_t offset, int whence) {
    MemoryContext* ctx = reinterpret_cast<MemoryContext*>(opaque);
    if (whence == AVSEEK_SIZE) return static_cast<int64_t>(ctx->size);
    size_t new_pos;
    if (whence == SEEK_SET) new_pos = static_cast<size_t>(offset);
    else if (whence == SEEK_CUR) new_pos = ctx->pos + static_cast<size_t>(offset);
    else if (whence == SEEK_END) new_pos = ctx->size + static_cast<size_t>(offset);
    else return -1;
    if (new_pos > ctx->size) return -1;
    ctx->pos = new_pos;
    return static_cast<int64_t>(ctx->pos);
}

inline int open_memory_io(AVFormatContext** fmt_ctx, const uint8_t* data, size_t size, AVIOContext** io_ctx, uint8_t** io_buffer, size_t io_buffer_size = 4096) {
    *fmt_ctx = avformat_alloc_context();
    if (!*fmt_ctx) return AVERROR(ENOMEM);
    MemoryContext* mem_ctx = static_cast<MemoryContext*>(av_mallocz(sizeof(MemoryContext)));
    if (!mem_ctx) return AVERROR(ENOMEM);
    mem_ctx->data = data;
    mem_ctx->size = size;
    mem_ctx->pos = 0;
    *io_buffer = static_cast<uint8_t*>(av_malloc(io_buffer_size));
    if (!*io_buffer) return AVERROR(ENOMEM);
    *io_ctx = avio_alloc_context(*io_buffer, static_cast<int>(io_buffer_size), 0, mem_ctx, &read_packet, nullptr, &seek_packet);
    if (!*io_ctx) return AVERROR(ENOMEM);
    (*fmt_ctx)->pb = *io_ctx;
    (*fmt_ctx)->flags |= AVFMT_FLAG_CUSTOM_IO;
    return 0;
}

inline void close_memory_io(AVFormatContext* fmt_ctx, AVIOContext* io_ctx) {
    MemoryContext* mem = io_ctx ? reinterpret_cast<MemoryContext*>(io_ctx->opaque) : nullptr;
    if (fmt_ctx) avformat_close_input(&fmt_ctx);
    if (io_ctx) avio_context_free(&io_ctx);
    if (mem) av_free(mem);
}
}
