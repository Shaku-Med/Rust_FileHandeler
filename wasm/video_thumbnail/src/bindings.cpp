#include <emscripten/bind.h>
#include <vector>
#include "video_thumbnail/thumbnail.hpp"

using namespace emscripten;

EMSCRIPTEN_BINDINGS(video_thumbnail_module) {
    using namespace vthumb;
    register_vector<uint8_t>("Uint8Vector");

    value_object<ImageBuffer>("ImageBuffer")
        .field("data", &ImageBuffer::data)
        .field("width", &ImageBuffer::width)
        .field("height", &ImageBuffer::height)
        .field("channels", &ImageBuffer::channels)
        .field("mime", &ImageBuffer::mime);

    value_object<DecodeOptions>("DecodeOptions")
        .field("timestampSeconds", &DecodeOptions::timestampSeconds)
        .field("targetWidth", &DecodeOptions::targetWidth)
        .field("targetHeight", &DecodeOptions::targetHeight)
        .field("exactSeek", &DecodeOptions::exactSeek)
        .field("preserveAspect", &DecodeOptions::preserveAspect)
        .field("allowUpscale", &DecodeOptions::allowUpscale)
        .field("pixelFormat", &DecodeOptions::pixelFormat);

    function("extractThumbnailFromBytes", optional_override([](val typedArray, const DecodeOptions& options) {
        const size_t length = typedArray["length"].as<size_t>();
        std::vector<uint8_t> buf(length);
        val dstView = val(typed_memory_view(length, buf.data()));
        dstView.call<void>("set", typedArray);
        auto img = vthumb::extract_thumbnail_from_bytes(buf.data(), buf.size(), options);
        return img;
    }));
}
