#ifndef READ_XML_H
#define READ_XML_H

#include <stdlib.h>
#include "structure.h"

int read_xml(char **files, const size_t n_files, const path_struct ps,
             const char *cache_dir, const char *progress_file, size_t n_threads);
#endif
