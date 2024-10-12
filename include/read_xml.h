#ifndef READ_XML_H
#define READ_XML_H

#include "structure.h"

#include <stdlib.h>

enum { CACHE_APPEND = 0, CACHE_OVERWRITE = 1 };

int read_xml(char** files, size_t const n_files, path_struct const ps,
  char const* cache_dir, int const overwrite_cache, char const* progress_file,
  size_t n_threads);
#endif
