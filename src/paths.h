#ifndef PATHS_H
#define PATHS_H

#include <stdio.h>
#include <stdbool.h>

#include "query.h"

typedef struct Path {
  char **components;
  size_t length;
  const size_t max_path_depth;
} *path;

path path_init(const char *xml_path, const size_t str_max);
path path_init_dynamic(const size_t max_path_size);
void path_destroy(path p);
void path_append(path p, const tag *t);
void path_drop_last_component(path p);
int path_match(const path p1, const path p2);
int path_is_empty(const path p);
void path_print(const path p);

#endif
