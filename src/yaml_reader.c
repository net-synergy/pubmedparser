#include "yaml_reader.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define STR_MAX 1000
#define BLOCK_MAX 50000
#define ISWHITESPACE(c) ((c == ' ') || (c == '\n') || (c == '\t'))
#define ISALPHA(c) (((c >= 'a') && (c <= 'z')) || ((c >= 'A') && (c <= 'Z')))

static int yaml_get_key(char *buffer, const size_t max_size, FILE *fptr)
{
  char c;

  do c = fgetc(fptr);
  while (!ISALPHA(c));

  int i;
  for (i = 0; (c != EOF) && (i < (int)max_size); i++) {
    if (c == ':') {
      buffer[i] = '\0';
      break;
    } else if (ISWHITESPACE(c)) {
      i = -1;
    } else {
      buffer[i] = c;
    }
    c = fgetc(fptr);
  }

  if (i == (int)max_size) {
    buffer[i - 1] = '\0';
    fprintf(stderr,
            "Warning: buffer too small to fit key. \
Increase buffer size to get entire key.\n");
    return YAML__WARN_BUFFER_OVERFLOW;
  }

  return c;
}

static int yaml_get_value(char *buffer, const size_t max_size, FILE *fptr)
{
  char c;

  do c = fgetc(fptr);
  while ((c == ' ') || (c == '\t') || (c == '{'));

  if (c == '}' || c == EOF || c == '\n') {
    return YAML__ERROR_VALUE;
  }

  if (c == '{') {
    do c = fgetc(fptr);
    while (ISWHITESPACE(c));
  }

  int i = 0;
  char delim = EOF;
  if (c == '"' || c == '\'') {
    delim = c;
    while ((c = fgetc(fptr)) != delim && c != EOF) {
      buffer[i] = c;
      i++;
    }
  } else {
    while (c != ',' && c != '\n' && c != '}' &&
	   i < (int)max_size && c != EOF) {
      buffer[i] = c;
      i++;
      c = fgetc(fptr);
    };
  }

  if (c == EOF) {
    return YAML__ERROR_VALUE;
  }

  if (i == (int)max_size) {
    buffer[i - 1] = '\0';
    fprintf(stderr, "Warning: value was larger than value buffer. \
Increase buffer size to get full value.\n");
    return YAML__WARN_BUFFER_OVERFLOW;
  }

  while (ISWHITESPACE(buffer[i - 1])) i--;
  buffer[i] = '\0';

  if (c == EOF) {
    return YAML__ERROR_VALUE;
  }

  return c;
}

int yaml_get_map_value(const char *structure_file, const char *key,
                       char *value, const size_t str_max)
{
  FILE *fptr;
  if (!(fptr = fopen(structure_file, "r"))) {
    fprintf(stderr, "Could not open %s\n", structure_file);
    return YAML__ERROR_FILE;
  }

  char buff[STR_MAX];
  char c;

  do c = yaml_get_key(buff, STR_MAX, fptr);
  while (strcmp(buff, key) != 0 && c != EOF);

  if (c == EOF) {
    fprintf(stderr, "Could not find key %s in %s\n", key, structure_file);
    return YAML__ERROR_KEY;
  }

  c = yaml_get_value(value, str_max, fptr);

  if (c == YAML__ERROR_VALUE) {
    fprintf(stderr, "Could not find value for key %s in %s\n", key,
            structure_file);
    return c;
  }

  fclose(fptr);
  return 0;
}

int yaml_get_map_contents(const char *structure_file, const char *key,
                          char ***key_value_pairs, size_t *n_items)
{
  FILE *fptr;
  if (!(fptr = fopen(structure_file, "r"))) {
    fprintf(stderr, "Could not open %s\n", structure_file);
    return YAML__ERROR_FILE;
  }

  char buff[STR_MAX];
  char c;

  do c = yaml_get_key(buff, STR_MAX, fptr);
  while (strcmp(buff, key) != 0 && c != EOF);

  if (c == EOF) {
    fprintf(stderr, "Could not find key %s in %s.\n", key, structure_file);
    return YAML__ERROR_KEY;
  }

  do c = fgetc(fptr);
  while (c != '{' && c != EOF);

  if (c == EOF) {
    fprintf(stderr, "Could not find values for key %s in %s.\n", key,
            structure_file);
    return YAML__ERROR_VALUE;
  }

  char block_buffer[BLOCK_MAX];
  int block_i = 0;
  *n_items = 0;
  int in_string = 0;
  while ((c != '}' || in_string) && c != EOF && block_i < BLOCK_MAX) {
    if (c == ':')
      (*n_items)++;

    if (c == '\'' || c == '"') in_string = !in_string;

    block_buffer[block_i] = c;
    block_i++;
    c = fgetc(fptr);
  }
  block_buffer[block_i] = '}';
  block_buffer[block_i + 1] = '\0';

  if (c == EOF) {
    fprintf(stderr, "File ended while searching for values for key %s in %s.\n",
            key, structure_file);
    return YAML__ERROR_VALUE;
  }

  if (*n_items == 0) {
    fprintf(stderr, "Did not find any values for key %s in %s.\n", key,
            structure_file);
    return YAML__ERROR_VALUE;
  }

  key_value_pairs[0] = malloc(sizeof *key_value_pairs[0] * (*n_items));
  key_value_pairs[1] = malloc(sizeof *key_value_pairs[1] * (*n_items));
  FILE *block_ptr = fmemopen(block_buffer, BLOCK_MAX, "r");
  for (int i = 0; i < (int)*n_items; i++) {
    c = yaml_get_key(buff, STR_MAX, block_ptr);
    key_value_pairs[0][i] = strdup(buff);
    c = yaml_get_value(buff, STR_MAX, block_ptr);
    key_value_pairs[1][i] = strdup(buff);
  }

  fclose(fptr);
  fclose(block_ptr);

  return 0;
}
