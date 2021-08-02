#ifndef _LT_MMAP_
#define _LT_MMAP_

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <errno.h>

#include <iostream>

inline bool mmap_file(FILE* fd, void*& ptr, int& len)
{
  std::cerr << "mmap_file()\n";
  struct stat sb;
  if (fstat(fileno(fd), &sb) == -1) {
    std::cerr << "fstat failed\n";
    return false;
  }
  len = sb.st_size;
  std::cerr << "file length is " << len << "\n";
  ptr = mmap(NULL, len, PROT_READ, MAP_SHARED, fileno(fd), 0);
  if (ptr == MAP_FAILED) {
    std::cerr << "mmap failed\nerrno = " << errno << "\n";
    return false;
  }
  std::cerr << "got pointer\n";
  return true;
}

#endif
