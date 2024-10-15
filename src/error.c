#include "error.h"

#include "read_xml.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static char* pp_error_strings[PP_NUM_ERRORS] = {
  [PP_ERR_KEY] = "Key not found in structure file",
  [PP_ERR_VALUE] = "Value malformed or missing in structure file",
  [PP_ERR_EOF] = "End of file reached during parsing",
  [PP_ERR_OOM] = "Out of memory",
  [PP_ERR_TAG_MISMATCH] = "Tags in XML file did not match",
  [PP_ERR_FILE_NOT_FOUND] = "Could not open file"
};

static pubmedparser_error_handler_t* pp_error_handler = NULL;
static pubmedparser_error_handler_t* pp_warn_handler = NULL;
static char* pp_errmsg = NULL;
static int pp_oom = 0;

void pubmedparser_set_error_handler(pubmedparser_error_handler_t* handler)
{
  pp_error_handler = handler;
}

void pubmedparser_set_warn_handler(pubmedparser_error_handler_t* handler)
{
  pp_warn_handler = handler;
}

void pubmedparser_error(pp_errno const code, char const* fmt, ...)
{
  va_list ap;
  char* errstr = pp_error_strings[code];
  char errmsg[1024];

  if (!pp_error_handler) {
    return;
  }

  if (code == PP_ERR_OOM) {
    pubmedparser_set_oom(1);
  }

  va_start(ap, fmt);
  vsnprintf(errmsg, sizeof(errmsg) - 1, fmt, ap);
  va_end(ap);

  if (pp_errmsg) {
    strncat(errmsg, "\n", sizeof(errmsg) - strlen(errmsg) - 1);
    strncat(errmsg, pp_errmsg, sizeof(errmsg) - strlen(errmsg) - 1);
  }

  pp_error_handler(errstr ? errstr : "", errmsg);
}

void pubmedparser_warn(pp_errno const code, char const* fmt, ...)
{
  va_list ap;
  char* errstr = pp_error_strings[code];
  char errmsg[1024];

  if (!pp_warn_handler) {
    return;
  }

  va_start(ap, fmt);
  vsnprintf(errmsg, sizeof(errmsg) - 1, fmt, ap);
  va_end(ap);

  pp_warn_handler(errstr ? errstr : "", errmsg);
}

void pubmedparser_set_errmsg(char const* fmt, ...)
{
  va_list ap;
  size_t bufflen = 1024;

  if (pp_errmsg) {
    free(pp_errmsg);
  }

  pp_errmsg = malloc(bufflen * sizeof('\0'));

  va_start(ap, fmt);
  vsnprintf(pp_errmsg, bufflen - 1, fmt, ap);
  va_end(ap);
}

char* pubmedparser_get_errmsg() { return pp_errmsg; }

void pubmedparser_set_oom(int const oom) { pp_oom = oom; }

int pubmedparser_get_oom() { return pp_oom; };
