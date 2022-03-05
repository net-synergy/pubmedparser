#ifndef YAML_READER_H
#define YAML_READER_H

#include <stdlib.h>

#define YAML__ERROR_FILE -10
#define YAML__ERROR_KEY -20
#define YAML__ERROR_VALUE -30
#define YAML__WARN_BUFFER_OVERFLOW -40

int yaml_get_map_value(const char *structure_file, const char *key,
                       char *value, const size_t str_max);
int yaml_get_map_contents(const char *structure_file, const char *key,
                          char ***key_value_pairs, size_t *n_items);

#endif
