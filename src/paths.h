#ifndef PATHS_H
#define PATHS_H

#include "query.h"

#include <stdbool.h>

typedef struct Path {
  char** components;
  size_t length;
  size_t const max_path_depth;
}* path;

path path_init(char const* xml_path, size_t const str_max);
path path_init_dynamic(size_t const max_path_size);
void path_destroy(path p);
void path_append(path p, tag const* t);
void path_drop_last_component(path p);
int path_match(path const p1, path const p2);
int path_is_empty(path const p);
void path_print(path const p);

#endif
