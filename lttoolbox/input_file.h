#ifndef _LT_INPUT_FILE_H_
#define _LT_INPUT_FILE_H_

#include <cstdio>
#include <unicode/uchar.h>

class InputFile
{
private:
  FILE* infile;
  UChar ubuffer[3];
  char cbuffer[4];
  int buffer_size;
  void internal_read();
public:
  InputFile();
  ~InputFile();
  bool open(char* fname);
  void close();
  UChar get();
  UChar peek();
  void unget(UChar c);
  bool eof();
};

#endif
