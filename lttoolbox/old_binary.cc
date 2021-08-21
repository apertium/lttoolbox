#include <lttoolbox/old_binary.h>
#include <lttoolbox/my_stdio.h>
#include <cmath>

using namespace OldBinary;

uint64_t
OldBinary::read_u64(FILE* in)
{
  uint64_t v = 0;
  if (fread_unlocked(reinterpret_cast<char*>(&v), 1, sizeof(v), in) != sizeof(v)) {
    throw std::runtime_error("Failed to read uint64_t");
  }
  // these are unconditional byte-swaps, so on little-endian platforms
  // this reads big-endian data
  // this is very bad, but it's the way all the old data was written,
  // so we have this here for backwards compatibility until we drop
  // support for lttoolbox/apertium <= 3
  //         -DGS 2021-08-21
  return (((v & 0xFF00000000000000) >> 56) |
          ((v & 0xFF000000000000) >> 40) |
          ((v & 0xFF0000000000) >> 24) |
          ((v & 0xFF00000000) >> 8) |
          ((v & 0xFF000000) << 8) |
          ((v & 0xFF0000) << 24) |
          ((v & 0xFF00) << 40) |
          ((v & 0xFF) << 56));
}

uint64_t read_byte(FILE* in)
{
  unsigned char ret = 0;
  if (fread_unlocked(&ret, 1, 1, in) != 1) {
    throw std::runtime_error("Failed to read byte");
  }
  return ret;
}

uint64_t
OldBinary::read_int(FILE* in, bool compression)
{
  if (compression) {
    uint64_t up = read_byte(in);
    if (up < 0x40) {
      return up;
    } else if (up < 0x80) {
      return ((up & 0x3f) << 8) | read_byte(in);
    } else if (up < 0xc0) {
      uint64_t ret = (up & 0x3f) << 8;
      ret |= read_byte(in);
      return (ret << 8) | read_byte(in);
    } else {
      uint64_t ret = ((up & 0x3f) << 8) | read_byte(in);
      ret = (ret << 8) | read_byte(in);
      ret = (ret << 8) | read_byte(in);
      return ret;
    }
  } else {
    uint64_t ret = 0;
    uint64_t size = read_byte(in);
    if (size > 8) {
      throw std::runtime_error("can't deserialise int");
    }
    uint8_t buffer[8];
    if (fread_unlocked(buffer, 1, size, in) != size) {
      throw std::runtime_error("can't deserialise int");
    }
    for (uint8_t i = 0; i < size; i++) {
      ret += static_cast<uint64_t>(buffer[i]) << (8 * (size - i - 1));
    }
    return ret;
  }
}

void
OldBinary::read_ustr(FILE* in, UString& s, bool compression)
{
  uint64_t count = read_int(in, compression);
  for (uint64_t i = 0; i < count; i++) {
    s += static_cast<UChar32>(read_int(in, compression));
  }
}

void
OldBinary::read_str(FILE* in, std::string& s, bool compression)
{
  uint64_t count = read_int(in, compression);
  for (uint64_t i = 0; i < count; i++) {
    s += static_cast<char>(read_int(in, compression));
  }
}

double
OldBinary::read_double(FILE* in, bool compression, bool endian_util)
{
  if (compression) {
    if (endian_util) {
      double retval;
#ifdef WORDS_BIGENDIAN
      fread_unlocked(&retval, sizeof(double), 1, input);
#else
      char *s = reinterpret_cast<char *>(&retval);

      for(int i = sizeof(double)-1; i != -1; i--) {
        if(fread_unlocked(&(s[i]), 1, 1, in)==0) {
          return 0;
        }
      }
#endif
      return retval;
    } else {
      uint64_t mantissa = read_int(in, true);
      if (mantissa >= 0x04000000) {
        mantissa = ((mantissa & 0x03ffffff) << 26) | read_int(in, true);
      }

      uint64_t exponent = read_int(in, true);
      if (exponent >= 0x04000000) {
        exponent = ((exponent & 0x03ffffff) << 26) | read_int(in, true);
      }

      double v = static_cast<double>(static_cast<int>(mantissa)) / 0x40000000;
      return ldexp(v, static_cast<int>(exponent));
    }
  } else {
    uint64_t d = read_int(in, false);
    return *reinterpret_cast<double*>(&d);
  }
}
