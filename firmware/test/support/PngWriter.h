// Minimal PNG writer for the golden images (test support, no third-party code): 8-bit RGB,
// zlib stream made of "stored" deflate blocks (no compression), CRC-32 and Adler-32 computed
// here. It only ever encodes bytes the tests produce, so it has no untrusted input. The
// reference comparison does not use PNG at all (raw RGB565 dumps, see Golden.h); PNGs exist
// for humans and GitHub.
#pragma once

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cstdint>
#include <cstdio>
#include <vector>

#include "eyes/Framebuffer.h"

namespace cubbies::test {

// Opens `path` for writing with an explicit 0644 mode (never world-writable), truncating it.
// Host-only test support (POSIX).
inline FILE* openForWrite(const char* path) {
  const int fd = ::open(path, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
  if (fd < 0) return nullptr;
  FILE* f = ::fdopen(fd, "wb");
  if (f == nullptr) ::close(fd);
  return f;
}

inline uint32_t crc32(const uint8_t* data, size_t len, uint32_t crc = 0) {
  crc = ~crc;
  for (size_t i = 0; i < len; ++i) {
    crc ^= data[i];
    for (int k = 0; k < 8; ++k) crc = (crc >> 1) ^ (0xEDB88320u & (0u - (crc & 1u)));
  }
  return ~crc;
}

inline uint32_t adler32(const uint8_t* data, size_t len) {
  uint32_t a = 1, b = 0;
  for (size_t i = 0; i < len; ++i) {
    a = (a + data[i]) % 65521u;
    b = (b + a) % 65521u;
  }
  return (b << 16) | a;
}

inline void putU32(std::vector<uint8_t>& out, uint32_t v) {
  out.push_back(static_cast<uint8_t>(v >> 24));
  out.push_back(static_cast<uint8_t>(v >> 16));
  out.push_back(static_cast<uint8_t>(v >> 8));
  out.push_back(static_cast<uint8_t>(v));
}

inline void putChunk(std::vector<uint8_t>& out, const char type[4], const std::vector<uint8_t>& data) {
  putU32(out, static_cast<uint32_t>(data.size()));
  std::vector<uint8_t> body(type, type + 4);
  body.insert(body.end(), data.begin(), data.end());
  out.insert(out.end(), body.begin(), body.end());
  putU32(out, crc32(body.data(), body.size()));
}

// zlib stream with stored blocks of at most 65535 bytes.
inline std::vector<uint8_t> zlibStored(const std::vector<uint8_t>& raw) {
  std::vector<uint8_t> z{0x78, 0x01};
  size_t pos = 0;
  do {
    const size_t len = raw.size() - pos > 65535 ? 65535 : raw.size() - pos;
    const bool last = pos + len == raw.size();
    z.push_back(last ? 1 : 0);
    z.push_back(static_cast<uint8_t>(len));
    z.push_back(static_cast<uint8_t>(len >> 8));
    z.push_back(static_cast<uint8_t>(~len));
    z.push_back(static_cast<uint8_t>(~len >> 8));
    z.insert(z.end(), raw.begin() + static_cast<long>(pos), raw.begin() + static_cast<long>(pos + len));
    pos += len;
  } while (pos < raw.size());
  putU32(z, adler32(raw.data(), raw.size()));
  return z;
}

inline void rgb565ToRgb888(uint16_t v, uint8_t* out) {
  const uint8_t r5 = (v >> 11) & 0x1F;
  const uint8_t g6 = (v >> 5) & 0x3F;
  const uint8_t b5 = v & 0x1F;
  out[0] = static_cast<uint8_t>((r5 << 3) | (r5 >> 2));
  out[1] = static_cast<uint8_t>((g6 << 2) | (g6 >> 4));
  out[2] = static_cast<uint8_t>((b5 << 3) | (b5 >> 2));
}

// Encodes the framebuffer as an 8-bit RGB PNG (filter type 0 on every row).
inline std::vector<uint8_t> encodePng(const Framebuffer& fb) {
  std::vector<uint8_t> png{0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A};
  std::vector<uint8_t> ihdr;
  putU32(ihdr, Framebuffer::kWidth);
  putU32(ihdr, Framebuffer::kHeight);
  ihdr.insert(ihdr.end(), {8, 2, 0, 0, 0});  // 8 bits, truecolour, deflate, filter 0, no interlace
  putChunk(png, "IHDR", ihdr);
  std::vector<uint8_t> raw;
  raw.reserve(static_cast<size_t>(Framebuffer::kHeight) * (1 + Framebuffer::kWidth * 3));
  for (int y = 0; y < Framebuffer::kHeight; ++y) {
    raw.push_back(0);  // filter: none
    for (int x = 0; x < Framebuffer::kWidth; ++x) {
      uint8_t rgb[3];
      rgb565ToRgb888(fb.at(x, y).value, rgb);
      raw.insert(raw.end(), rgb, rgb + 3);
    }
  }
  putChunk(png, "IDAT", zlibStored(raw));
  putChunk(png, "IEND", {});
  return png;
}

inline bool writePng(const char* path, const Framebuffer& fb) {
  const auto png = encodePng(fb);
  FILE* f = openForWrite(path);
  if (f == nullptr) return false;
  const bool ok = std::fwrite(png.data(), 1, png.size(), f) == png.size();
  std::fclose(f);
  return ok;
}

}  // namespace cubbies::test
