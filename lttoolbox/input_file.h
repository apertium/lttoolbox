#ifndef _LT_INPUT_FILE_H_
#define _LT_INPUT_FILE_H_

#include <cstdio>
#include <unicode/uchar.h>

class InputFile
{
private:
  FILE* infile;
  UChar32 ubuffer[3];
  char cbuffer[4];
  int buffer_size;
  void internal_read();
public:
  InputFile();
  ~InputFile();
  bool open(char* fname);
  void close();
  UChar32 get();
  UChar32 peek();
  void unget(UChar32 c);
  bool eof();
};

#endif
