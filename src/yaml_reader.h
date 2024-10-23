#ifndef YAML_READER_H
#define YAML_READER_H

#include "read_xml.h"

#include <stdio.h>

pp_errno yaml_get_keys(FILE* fptr, char*** keys, size_t* n_keys,
  int const start, size_t const str_max);
int yaml_map_value_is_singleton(
  FILE* fptr, char const* key, int const start, size_t const str_max);
pp_errno yaml_get_map_value(FILE* fptr, char const* key, char* value,
  int const start, size_t const str_max);

#endif
