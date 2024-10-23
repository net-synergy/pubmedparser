#ifndef ERROR_H
#define ERROR_H

#include "read_xml.h"

#define PP_RETURN_ERROR_WITH_MSG(errcode, fmt, ...)                           \
  pubmedparser_set_errmsg((fmt), __VA_ARGS__);                                \
  return (errcode)

void pubmedparser_error(pp_errno const code, char const* fmt, ...);
void pubmedparser_warn(pp_errno const code, char const* fmt, ...);
int pubmedparser_interruption(void);
void pubmedparser_set_errmsg(char const* fmt, ...);
char* pubmedparser_get_errmsg();

#endif
