// Golden-image support: the reference of a rendered state is the raw RGB565 framebuffer
// (`<name>.rgb565`, 12 KB, compared byte for byte); a PNG of the same frame is written next to
// it for humans and GitHub (PngWriter.h). Regeneration is explicit (CUBBIES_UPDATE_GOLDEN=1)
// and refused under CI, so a leaked variable can never turn the comparison into a silent pass.
#pragma once

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include "PngWriter.h"
#include "eyes/Framebuffer.h"

namespace cubbies::test {

inline bool updateGoldenRequested() {
  const char* v = std::getenv("CUBBIES_UPDATE_GOLDEN");
  return v != nullptr && std::strcmp(v, "1") == 0 && std::getenv("CI") == nullptr;
}

inline bool writeRaw(const std::string& path, const Framebuffer& fb) {
  FILE* f = std::fopen(path.c_str(), "wb");
  if (f == nullptr) return false;
  const size_t bytes = Framebuffer::kPixels * sizeof(uint16_t);
  const bool ok = std::fwrite(fb.data(), 1, bytes, f) == bytes;
  std::fclose(f);
  return ok;
}

// Number of pixels that differ from the raw reference, or -1 if it cannot be read or has the
// wrong size.
inline long compareRaw(const std::string& path, const Framebuffer& fb) {
  FILE* f = std::fopen(path.c_str(), "rb");
  if (f == nullptr) return -1;
  uint16_t ref[Framebuffer::kPixels];
  const size_t got = std::fread(ref, sizeof(uint16_t), Framebuffer::kPixels, f);
  const bool extra = std::fgetc(f) != EOF;
  std::fclose(f);
  if (got != Framebuffer::kPixels || extra) return -1;
  long diff = 0;
  for (size_t i = 0; i < Framebuffer::kPixels; ++i)
    if (ref[i] != fb.data()[i]) ++diff;
  return diff;
}

}  // namespace cubbies::test
