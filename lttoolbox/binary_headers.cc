#include <lttoolbox/binary_headers.h>

#include <lttoolbox/endian_util.h>
#include <cstring>

bool
readHeader(FILE* input, const char* expect_header, uint64_t& feats)
{
  feats = 0;
  fpos_t pos;
  if (fgetpos(input, &pos) == 0) {
    char header[4]{};
    auto r = fread_unlocked(header, 1, 4, input);
    if (r == 4 && strncmp(header, expect_header, 4) == 0) {
      feats = read_le_64(input);
      return true;
    } else {
      fsetpos(input, &pos);
    }
  }
  return false;
}
