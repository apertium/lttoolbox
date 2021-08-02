#ifndef _LT_MMAP_
#define _LT_MMAP_

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

//DEBUG
//#include <iostream>
//#include <errno.h>

inline bool mmap_file(FILE* fd, void*& ptr, int& len)
{
  struct stat sb;
  if (fstat(fileno(fd), &sb) == -1) {
    return false;
  }
  len = sb.st_size;
  ptr = mmap(NULL, len, PROT_READ, MAP_SHARED, fileno(fd), 0);
  if (ptr == MAP_FAILED) {
    //std::cerr << "mmap failed\nerrno = " << errno << "\n";
    return false;
  }
  return true;
}

#endif
