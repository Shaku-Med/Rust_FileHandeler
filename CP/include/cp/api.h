#pragma once
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  uint8_t* data;
  int width;
  int height;
  int stride;
  int channels;
  int mime_len;
} cp_image;

void cp_init(void);
void cp_free(void* p);
void* cp_malloc(size_t size);

int cp_thumbnail_rgba(const uint8_t* bytes, size_t length, int maxWidth, int maxHeight, double atSeconds, int exactFrame, cp_image* out);
int cp_thumbnail_png(const uint8_t* bytes, size_t length, int maxWidth, int maxHeight, double atSeconds, int exactFrame, int compressionLevel, uint8_t** out_data, size_t* out_len);
int cp_thumbnail_jpeg(const uint8_t* bytes, size_t length, int maxWidth, int maxHeight, double atSeconds, int exactFrame, int quality, uint8_t** out_data, size_t* out_len);
void cp_free_buf(uint8_t* p);

#ifdef __cplusplus
}
#endif
