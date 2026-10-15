// PngWriter: structure of the produced stream (signature, chunks, CRC-32, Adler-32, stored
// deflate) checked against the PNG and zlib specifications with independent known answers.
#include <unity.h>

#include <cstring>

#include "eyes/Framebuffer.h"  // first: lets the PlatformIO dependency finder link cubbies_core
#include "PngWriter.h"

using namespace cubbies;
using namespace cubbies::test;

void setUp() {}
void tearDown() {}

static uint32_t readU32(const std::vector<uint8_t>& v, size_t at) {
  return (static_cast<uint32_t>(v[at]) << 24) | (static_cast<uint32_t>(v[at + 1]) << 16) |
         (static_cast<uint32_t>(v[at + 2]) << 8) | v[at + 3];
}

static void test_crc32_known_answer() {
  // Standard check value: CRC-32 of "123456789" is 0xCBF43926.
  const char* s = "123456789";
  TEST_ASSERT_EQUAL_HEX32(0xCBF43926u, crc32(reinterpret_cast<const uint8_t*>(s), 9));
}

static void test_adler32_known_answer() {
  // Adler-32 of "Wikipedia" is 0x11E60398 (RFC 1950 example).
  const char* s = "Wikipedia";
  TEST_ASSERT_EQUAL_HEX32(0x11E60398u, adler32(reinterpret_cast<const uint8_t*>(s), 9));
}

static void test_png_has_signature_ihdr_idat_iend_with_valid_crcs() {
  Framebuffer fb;
  fb.clear(rgb565(255, 0, 0));
  const auto png = encodePng(fb);
  const uint8_t sig[8] = {0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A};
  TEST_ASSERT_EQUAL_MEMORY(sig, png.data(), 8);
  size_t at = 8;
  const char* expected[] = {"IHDR", "IDAT", "IEND"};
  for (const char* type : expected) {
    const uint32_t len = readU32(png, at);
    TEST_ASSERT_EQUAL_MEMORY(type, &png[at + 4], 4);
    const uint32_t crc = readU32(png, at + 8 + len);
    TEST_ASSERT_EQUAL_HEX32(crc, crc32(&png[at + 4], 4 + len));
    if (std::strcmp(type, "IHDR") == 0) {
      TEST_ASSERT_EQUAL_UINT32(13, len);
      TEST_ASSERT_EQUAL_UINT32(Framebuffer::kWidth, readU32(png, at + 8));
      TEST_ASSERT_EQUAL_UINT32(Framebuffer::kHeight, readU32(png, at + 12));
      TEST_ASSERT_EQUAL_UINT8(8, png[at + 16]);  // bit depth
      TEST_ASSERT_EQUAL_UINT8(2, png[at + 17]);  // truecolour
    }
    at += 12 + len;
  }
  TEST_ASSERT_EQUAL_size_t(png.size(), at);
}

static void test_idat_is_a_stored_zlib_stream_of_the_filtered_rows() {
  Framebuffer fb;
  fb.clear(rgb565(0, 0, 0));
  fb.fillRect(0, 0, 1, 1, rgb565(255, 255, 255));
  const auto png = encodePng(fb);
  const size_t idat = 8 + 12 + 13;  // after IHDR
  const uint32_t len = readU32(png, idat);
  const uint8_t* z = &png[idat + 8];
  TEST_ASSERT_EQUAL_UINT8(0x78, z[0]);  // zlib header, deflate, 32K window
  TEST_ASSERT_EQUAL_UINT8(0x01, z[1]);  // no preset dictionary, fastest level
  TEST_ASSERT_EQUAL_UINT8(0x01, z[2]);  // BFINAL=1, BTYPE=00 (stored): single block
  const size_t rawLen = static_cast<size_t>(Framebuffer::kHeight) * (1 + Framebuffer::kWidth * 3);
  TEST_ASSERT_EQUAL_UINT16(rawLen, z[3] | (z[4] << 8));
  TEST_ASSERT_EQUAL_UINT16(static_cast<uint16_t>(~rawLen), z[5] | (z[6] << 8));
  const uint8_t* raw = z + 7;
  TEST_ASSERT_EQUAL_UINT8(0, raw[0]);    // filter none on row 0
  TEST_ASSERT_EQUAL_UINT8(255, raw[1]);  // pixel (0,0) white
  TEST_ASSERT_EQUAL_UINT8(255, raw[2]);
  TEST_ASSERT_EQUAL_UINT8(255, raw[3]);
  TEST_ASSERT_EQUAL_UINT8(0, raw[4]);    // pixel (1,0) black
  TEST_ASSERT_EQUAL_HEX32(adler32(raw, rawLen), readU32(png, idat + 8 + len - 4));
  TEST_ASSERT_EQUAL_size_t(2 + 5 + rawLen + 4, len);
}

static void test_rgb565_expands_to_full_range() {
  uint8_t rgb[3];
  rgb565ToRgb888(0xFFFF, rgb);
  TEST_ASSERT_EQUAL_UINT8(255, rgb[0]);
  TEST_ASSERT_EQUAL_UINT8(255, rgb[1]);
  TEST_ASSERT_EQUAL_UINT8(255, rgb[2]);
  rgb565ToRgb888(rgb565(0, 200, 255).value, rgb);
  TEST_ASSERT_EQUAL_UINT8(0, rgb[0]);
  TEST_ASSERT_EQUAL_UINT8(203, rgb[1]);  // 200 → 6-bit 50 → (50 << 2) | (50 >> 4) = 203
  TEST_ASSERT_EQUAL_UINT8(255, rgb[2]);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_crc32_known_answer);
  RUN_TEST(test_adler32_known_answer);
  RUN_TEST(test_png_has_signature_ihdr_idat_iend_with_valid_crcs);
  RUN_TEST(test_idat_is_a_stored_zlib_stream_of_the_filtered_rows);
  RUN_TEST(test_rgb565_expands_to_full_range);
  return UNITY_END();
}
