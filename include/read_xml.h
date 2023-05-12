#ifndef READ_XML_H
#define READ_XML_H

#include <stdlib.h>
#include "structure.h"

enum {
  CACHE_APPEND = 0,
  CACHE_OVERWRITE = 1
};

int read_xml(char **files, const size_t n_files, const path_struct ps,
             const char *cache_dir, const int overwrite_cache, const char *progress_file,
             size_t n_threads);
#endif
