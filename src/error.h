#ifndef ERROR_H
#define ERROR_H

typedef enum {
  PP_SUCCESS = 0,
  PP_ERR_KEY,
  PP_ERR_VALUE,
  PP_ERR_BUFFER_OVERFLOW,
  PP_ERR_EOF,
  PP_ERR_OOM,
  PP_ERR_TAG_MISMATCH,
  PP_ERR_FILE_NOT_FOUND,
  PP_NUM_ERRORS
} pp_errno;

#define PP_RETURN_ERROR_WITH_MSG(errcode, fmt, ...)                           \
  pubmedparser_set_errmsg((fmt), __VA_ARGS__);                                \
  return (errcode)

void pubmedparser_error(pp_errno const code, char const* fmt, ...);
void pubmedparser_warn(pp_errno const code, char const* fmt, ...);
void pubmedparser_set_errmsg(char const* fmt, ...);
char* pubmedparser_get_errmsg();
void pubmedparser_set_oom(int const oom);

#endif
