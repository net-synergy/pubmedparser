#ifndef YAML_READER_H
#define YAML_READER_H

#include <stdlib.h>
#include <stdio.h>

enum {
  YAML__ERROR_FILE = 50,
  YAML__ERROR_EOF,
  YAML__ERROR_KEY,
  YAML__ERROR_VALUE,
  YAML__WARN_BUFFER_OVERFLOW,
};

int yaml_get_keys(FILE *fptr, char ***keys, size_t *n_keys, const int start,
                  const size_t str_max);
int yaml_map_value_is_singleton(FILE *fptr, const char *key, const int start,
                                const size_t str_max);
int yaml_get_map_value(FILE *fptr, const char *key, char *value,
                       const int start, const size_t str_max);

#endif
