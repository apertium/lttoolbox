#include <lttoolbox/input_file.h>
#include <utf8.h>
#include <stdexcept>
#include <unicode/ustdio.h>
#include <cstring>

InputFile::InputFile()
  : infile(stdin), buffer_size(0)
{}

InputFile::~InputFile()
{
  close();
}

bool
InputFile::open(char* fname)
{
  close();
  if (fname == NULL) {
    infile = stdin;
  } else {
    infile = fopen(fname, "r");
  }
  return (infile != NULL);
}

void
InputFile::close()
{
  if (infile != NULL) {
    if (infile != stdin) {
      fclose(infile);
      delete infile;
    }
    infile = NULL;
  }
}

void
InputFile::internal_read()
{
  if (buffer_size) {
    return;
  }
  if (feof(infile)) {
    ubuffer[buffer_size++] = U_EOF;
    return;
  }
  int i = 1;
  cbuffer[0] = fgetc(infile);
  if (cbuffer[0] == EOF) {
    ubuffer[buffer_size++] = U_EOF;
    return;
  } else if (cbuffer[0] == '\0') {
    ubuffer[buffer_size++] = '\0';
    return;
  }
  if ((cbuffer[0] & 0xF0) == 0xF0) {
    i += 3;
    if (fread(cbuffer+1, 1, 3, infile) != 3) {
      throw std::runtime_error("Could not read 3 expected bytes from stream");
    }
  } else if ((cbuffer[0] & 0xE0) == 0xE0) {
    i += 2;
    if (fread(cbuffer+1, 1, 2, infile) != 2) {
      throw std::runtime_error("Could not read 2 expected bytes from stream");
    }
  } else if ((cbuffer[0] & 0xC0) == 0xC0) {
    i += 1;
    if (fread(cbuffer+1, 1, 1, infile) != 1) {
      throw std::runtime_error("Could not read 1 expected byte from stream");
    }
  }
  memset(ubuffer, 0, 3*sizeof(UChar));
  utf8::utf8to32(cbuffer, cbuffer+i, ubuffer);
  buffer_size = 1;
}

UChar32
InputFile::get()
{
  if (!buffer_size) {
    internal_read();
  }
  return ubuffer[--buffer_size];
}

UChar32
InputFile::peek()
{
  if (!buffer_size) {
    internal_read();
  }
  return ubuffer[buffer_size-1];
}

void
InputFile::unget(UChar32 c)
{
  // this will probably segfault if called multiple times
  ubuffer[buffer_size++] = c;
}

bool
InputFile::eof()
{
  return (infile == NULL) || feof(infile);
}
