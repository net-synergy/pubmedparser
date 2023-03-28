#ifndef STRUCTURE_H
#define STRUCTURE_H

#include <stdlib.h>

typedef struct PathStructure {
  char *name;
  char *path;
  struct PathStructure *parent;
  struct PathStructure **children;
  size_t n_children;
} *path_struct;

path_struct parse_structure_file(const char *structure_file,
                                 const size_t str_max);

void path_struct_destroy(path_struct ps);
void path_struct_print(const path_struct ps);

#endif
