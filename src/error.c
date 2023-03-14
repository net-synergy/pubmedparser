#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

void pubmedparser_error(const int status, const char *msg, ...)
{
  va_list argp;
  vfprintf(stderr, msg, argp);
  exit(status);
}
