// PNG read/write helpers for golden-image tests, on top of the vendored stb headers
// (public domain, see STB_VERSION). The including test defines STB_IMAGE_IMPLEMENTATION and
// STB_IMAGE_WRITE_IMPLEMENTATION once before including this header.
#pragma once

#include <cstdint>
#include <cstdlib>
#include <string>
#include <vector>

#include "eyes/Framebuffer.h"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#include "stb_image.h"
#include "stb_image_write.h"
#pragma GCC diagnostic pop

namespace cubbies::test {

inline void rgb565ToRgb888(uint16_t v, uint8_t* out) {
  const uint8_t r5 = (v >> 11) & 0x1F;
  const uint8_t g6 = (v >> 5) & 0x3F;
  const uint8_t b5 = v & 0x1F;
  out[0] = static_cast<uint8_t>((r5 << 3) | (r5 >> 2));
  out[1] = static_cast<uint8_t>((g6 << 2) | (g6 >> 4));
  out[2] = static_cast<uint8_t>((b5 << 3) | (b5 >> 2));
}

inline std::vector<uint8_t> toRgb888(const Framebuffer& fb) {
  std::vector<uint8_t> out(static_cast<size_t>(Framebuffer::kWidth) * Framebuffer::kHeight * 3);
  for (int y = 0; y < Framebuffer::kHeight; ++y) {
    for (int x = 0; x < Framebuffer::kWidth; ++x) {
      rgb565ToRgb888(fb.at(x, y).value, &out[(static_cast<size_t>(y) * Framebuffer::kWidth + x) * 3]);
    }
  }
  return out;
}

inline bool writePng(const std::string& path, const Framebuffer& fb) {
  const auto rgb = toRgb888(fb);
  return stbi_write_png(path.c_str(), Framebuffer::kWidth, Framebuffer::kHeight, 3, rgb.data(),
                        Framebuffer::kWidth * 3) != 0;
}

// Returns the number of pixels that differ from the PNG at `path`, or -1 if it cannot be read
// or has the wrong size.
inline long comparePng(const std::string& path, const Framebuffer& fb) {
  int w = 0, h = 0, channels = 0;
  unsigned char* ref = stbi_load(path.c_str(), &w, &h, &channels, 3);
  if (ref == nullptr) return -1;
  if (w != Framebuffer::kWidth || h != Framebuffer::kHeight) {
    stbi_image_free(ref);
    return -1;
  }
  const auto rgb = toRgb888(fb);
  long diff = 0;
  for (size_t i = 0; i < rgb.size(); i += 3) {
    if (rgb[i] != ref[i] || rgb[i + 1] != ref[i + 1] || rgb[i + 2] != ref[i + 2]) ++diff;
  }
  stbi_image_free(ref);
  return diff;
}

inline bool updateGoldenRequested() { return std::getenv("CUBBIES_UPDATE_GOLDEN") != nullptr; }

}  // namespace cubbies::test
