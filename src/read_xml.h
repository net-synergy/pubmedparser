#ifndef READ_XML_H
#define READ_XML_H

#include "query.h"
#include "paths.h"
#include "nodes.h"

int parse_file(const char *input, node_set *ns);
void cat(const char *node_name, const char *cache_dir, const int n_threads);

#endif