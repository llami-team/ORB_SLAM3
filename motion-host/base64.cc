#include "base64.h"

static const std::string base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                        "abcdefghijklmnopqrstuvwxyz"
                                        "0123456789+/";

std::string base64_encode(const std::vector<uint8_t> &in) {
  std::string out;

  int val = 0, valb = -6;
  for (uint8_t c : in) {
    val = (val << 8) + c;
    valb += 8;
    while (valb >= 0) {
      out.push_back(base64_chars[(val >> valb) & 0x3F]);
      valb -= 6;
    }
  }
  if (valb > -6)
    out.push_back(base64_chars[((val << 8) >> (valb + 8)) & 0x3F]);
  while (out.size() % 4)
    out.push_back('=');
  return out;
}

std::vector<uint8_t> base64_decode(const std::string &in) {
  std::vector<uint8_t> out;

  static std::vector<int> T(256, -1);
  static bool initialized = false;

  if (!initialized) {
    for (int i = 0; i < 64; i++)
      T[base64_chars[i]] = i;
    initialized = true;
  }

  int val = 0, valb = -8;
  for (uint8_t c : in) {
    if (T[c] == -1)
      break;
    val = (val << 6) + T[c];
    valb += 6;
    if (valb >= 0) {
      out.push_back((val >> valb) & 0xFF);
      valb -= 8;
    }
  }

  return out;
}
