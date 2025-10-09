#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace cp {
struct ImageBuffer {
  std::vector<std::uint8_t> data;
  int width{0};
  int height{0};
  int stride{0};
  int channels{4};
  std::string mime;
};

struct ThumbnailOptions {
  int maxWidth{320};
  int maxHeight{180};
  double atSeconds{-1.0};
  bool exactFrame{false};
};

ImageBuffer extractThumbnailRGBA(const std::uint8_t* bytes, std::size_t length, const ThumbnailOptions& options);
ImageBuffer extractThumbnailPNG(const std::uint8_t* bytes, std::size_t length, const ThumbnailOptions& options, int pngCompressionLevel);
ImageBuffer extractThumbnailJPEG(const std::uint8_t* bytes, std::size_t length, const ThumbnailOptions& options, int jpegQuality);
}
