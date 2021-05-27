#include "string_utils.h"

void
u_fputs(const UnicodeString str, UFILE* output)
{
  u_fprintf(output, "%S", str.getTerminatedBuffer());
}
