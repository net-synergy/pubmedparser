#ifndef READ_XML_H
#define READ_XML_H

#include "structure.h"

#include <stdlib.h>

enum { CACHE_APPEND = 0, CACHE_OVERWRITE };

typedef enum {
  PP_SUCCESS = 0,
  PP_ERR_KEY,
  PP_ERR_VALUE,
  PP_ERR_BUFFER_OVERFLOW,
  PP_ERR_EOF,
  PP_ERR_OOM,
  PP_ERR_TAG_MISMATCH,
  PP_ERR_FILE_NOT_FOUND,
  PP_INTERRUPTION,
  PP_NUM_ERRORS
} pp_errno;

typedef void pubmedparser_error_handler_t(
  pp_errno const code, char const* errstr, char const* msg);
typedef int pubmedparser_interruption_handler_t(void);

void pubmedparser_set_error_handler(pubmedparser_error_handler_t* handler);
void pubmedparser_set_warn_handler(pubmedparser_error_handler_t* handler);
void pubmedparser_set_interruption_handler(
  pubmedparser_interruption_handler_t* handler);

int read_xml(char** files, size_t const n_files, path_struct const ps,
  char const* cache_dir, int const overwrite_cache, char const* progress_file,
  size_t n_threads);
#endif
