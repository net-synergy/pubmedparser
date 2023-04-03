#ifndef ERROR_H
#define ERROR_H

typedef enum {
  PP_ERR_STRUCTURE_KEY = 0,
  PP_ERR_STRUCTURE_VALUE,
  PP_ERR_EOF,
  PP_ERR_OOM,
  PP_ERR_TAG_MISMATCH,
  PP_ERR_FILE_NOT_FOUND,
  PP_NUM_ERRORS
} PP_ERRNO;

void pubmedparser_error(const PP_ERRNO code, const char *fmt, ...);

#endif
