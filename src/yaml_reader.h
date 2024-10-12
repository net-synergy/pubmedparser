#ifndef YAML_READER_H
#define YAML_READER_H

#include <stdio.h>
#include <stdlib.h>

enum {
  YAML__ERROR_FILE = 50,
  YAML__ERROR_EOF,
  YAML__ERROR_KEY,
  YAML__ERROR_VALUE,
  YAML__WARN_BUFFER_OVERFLOW,
};

int yaml_get_keys(FILE* fptr, char*** keys, size_t* n_keys, int const start,
  size_t const str_max);
int yaml_map_value_is_singleton(
  FILE* fptr, char const* key, int const start, size_t const str_max);
int yaml_get_map_value(FILE* fptr, char const* key, char* value,
  int const start, size_t const str_max);

#endif
