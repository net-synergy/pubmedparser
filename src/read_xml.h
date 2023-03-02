#ifndef READ_XML_H
#define READ_XML_H

#include "query.h"
#include "paths.h"

int parse_file(char *input, node_set *ns);
void cat(const char *node_name, const char *cache_dir, const int n_threads);

#endif
