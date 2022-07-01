#ifndef _LT_BINARY_HEADERS_
#define _LT_BINARY_HEADERS_

#include <cstdint>
#include <cstdio>

// Global lttoolbox features
constexpr char HEADER_LTTOOLBOX[4]{'L', 'T', 'T', 'B'};
enum LT_FEATURES : uint64_t {
  LTF_MMAP = (1ull << 0), // using mmap-compatible format rather than compressed format
  LTF_UNKNOWN = (1ull << 1), // Features >= this are unknown, so throw an error; Inc this if more features are added
  LTF_RESERVED = (1ull << 63), // If we ever reach this many feature flags, we need a flag to know how to extend beyond 64 bits
};

// Invididual transducer features
constexpr char HEADER_TRANSDUCER[4]{'L', 'T', 'T', 'D'};
enum TD_FEATURES : uint64_t {
  TDF_WEIGHTS = (1ull << 0),
  TDF_MMAP = (1ull << 1),
  TDF_UNKNOWN = (1ull << 2), // Features >= this are unknown, so throw an error; Inc this if more features are added
  TDF_RESERVED = (1ull << 63), // If we ever reach this many feature flags, we need a flag to know how to extend beyond 64 bits
};

bool readHeader(FILE* input, const char* expect_header, uint64_t& feats);

#endif
