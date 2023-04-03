#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "error.h"

static char *pp_error_strings[PP_NUM_ERRORS] = {
  [PP_ERR_STRUCTURE_KEY] = "Key not found in structure file",
  [PP_ERR_STRUCTURE_VALUE] = "Value malformed or missing in structure file",
  [PP_ERR_EOF] = "End of file reached during parsing",
  [PP_ERR_OOM] = "Out of memory",
  [PP_ERR_TAG_MISMATCH] = "Tags in XML file did not match",
  [PP_ERR_FILE_NOT_FOUND] = "Could not open file"
};

void *pubmedparser_error_handler(const PP_ERRNO code, const char *errstr,
                                 const char *msg);

void pp_nonreturning_error_handler(const PP_ERRNO code, const char *errstr,
                                   const char *msg)
{
  fprintf(stderr, "%s\n\n", errstr);
  fprintf(stderr, "%s\n", msg);
  exit(code);
}

void pubmedparser_error(const PP_ERRNO code, const char *fmt, ...)
{
  va_list ap;
  char *errstr = pp_error_strings[code];
  char errmsg[500];

  va_start(ap, fmt);
  vsnprintf(errmsg, (sizeof(errmsg) / sizeof(*errmsg)) - 1, fmt, ap);
  va_end(ap);

  // TODO: Change to error handler that can be set by enduser.
  pp_nonreturning_error_handler(code, errstr, errmsg);
}
