#ifndef _LT_OLD_BINARY_
#define _LT_OLD_BINARY_

#include <cstdint>
#include <cstdio>
#include <lttoolbox/ustring.h>

namespace OldBinary {
  uint64_t read_u64(FILE* in);
  uint64_t read_int(FILE* in, bool compression=true);
  void read_ustr(FILE* in, UString& s, bool compression=true);
  void read_str(FILE* in, std::string& s, bool compression=true);
  double read_double(FILE* in, bool compression=true, bool endian_util=false);
};

#endif
