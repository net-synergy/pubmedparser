#include <string.h>

#include "yaml_reader.h"

#define BLOCK_MAX 50000
#define ISWHITESPACE(c) ((c == ' ') || (c == '\n') || (c == '\t'))
#define ISALPHA(c) (((c >= 'a') && (c <= 'z')) || ((c >= 'A') && (c <= 'Z')))

static void yaml_rewind_to_start_of_line(FILE *fptr)
{
  int pos = ftell(fptr);
  if (pos == 0) {
    return;
  }

  for (char c = fgetc(fptr); c != '\n' && pos >= 0; pos--) {
    c = fgetc(fptr);
    fseek(fptr, pos, SEEK_SET);
  };
}

static int yaml_get_key(char *buffer, const size_t max_size, FILE *fptr)
{
  char c;

  do c = fgetc(fptr);
  while (!ISALPHA(c) && c != EOF);

  size_t i;
  for (i = 0; (c != EOF) && (i < max_size); i++, c = fgetc(fptr)) {
    if (c == ':') {
      buffer[i] = '\0';
      break;
    } else if (ISWHITESPACE(c)) {
      i = -1;
    } else {
      buffer[i] = c;
    }
  }

  if (i == max_size) {
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

  size_t i = 0;
  char delim = EOF;
  if (c == '"' || c == '\'') {
    delim = c;
    while ((c = fgetc(fptr)) != delim && c != EOF) {
      buffer[i] = c;
      i++;
    }
  } else {
    while (c != ',' && c != '\n' && c != '}' &&
           i < max_size && c != EOF) {
      buffer[i] = c;
      i++;
      c = fgetc(fptr);
    };
  }

  if (c == EOF) {
    return YAML__ERROR_VALUE;
  }

  if (i == max_size) {
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

static size_t next_line_depth(FILE *fptr)
{
  char c = fgetc(fptr);
  size_t depth = 0;

  while (c != '\n' && c != EOF) c = fgetc(fptr);

  if (c == EOF) {
    return YAML__ERROR_EOF;
  }

  while (ISWHITESPACE(c)) {
    depth++;
    if (c == '\n') {
      depth = 0;
    }
    c = fgetc(fptr);
  }

  if (c == EOF) {
    return YAML__ERROR_EOF;
  }

  ungetc(c, fptr);
  return depth;
}

int yaml_get_keys(FILE *fptr, char ***keys, size_t *n_keys, const int start,
                  const size_t str_max)
{
  fseek(fptr, start, SEEK_SET);
  char buff[str_max];
  char c;
  *n_keys = 0;

  size_t initial_depth = 0;
  yaml_rewind_to_start_of_line(fptr);
  for (c = fgetc(fptr); ISWHITESPACE(c); c = fgetc(fptr), initial_depth++);
  yaml_rewind_to_start_of_line(fptr);

  size_t depth = initial_depth;
  while (((c = yaml_get_key(buff, str_max, fptr)) != EOF) &&
         (depth >= initial_depth) && (depth != YAML__ERROR_EOF)) {
    (*n_keys)++;

    do (depth = next_line_depth(fptr));
    while ((depth > initial_depth) && (depth != YAML__ERROR_EOF));
  }

  if ((depth == YAML__ERROR_EOF) && (initial_depth != 0)) {
    fprintf(stderr,
            "End of file while parsing key value in structure file\n. Possibly a missing \"}\"\n");
    return YAML__ERROR_KEY;
  }

  *keys = malloc(sizeof **keys * (*n_keys));
  fseek(fptr, start, SEEK_SET);
  for (size_t k = 0; k < (*n_keys); k++) {
    c = yaml_get_key(buff, str_max, fptr);
    (*keys)[k] = strdup(buff);

    do (c = fgetc(fptr));
    while (ISWHITESPACE(c));

    do depth = next_line_depth(fptr);
    while ((depth > initial_depth) && (depth != YAML__ERROR_EOF));
  }

  return EXIT_SUCCESS;
}

static int yaml_ff_to_key(FILE *fptr, const char *key, const int start,
                          const size_t str_max)
{
  fseek(fptr, start, SEEK_SET);
  char buff[str_max];
  char c;

  do c = yaml_get_key(buff, str_max, fptr);
  while (strcmp(buff, key) != 0 && c != EOF);

  if (c == EOF) {
    fprintf(stderr, "Could not find key %s in structure file\n", key);
    return YAML__ERROR_KEY;
  }

  return EXIT_SUCCESS;
}

int yaml_get_map_value(FILE *fptr, const char *key, char *value,
                       const int start, const size_t str_max)
{
  yaml_ff_to_key(fptr, key, start, str_max);

  char c;
  c = yaml_get_value(value, str_max, fptr);

  if (c == YAML__ERROR_VALUE) {
    fprintf(stderr, "Could not find value for key %s in structure file\n", key);
    return c;
  }

  return 0;
}

int yaml_map_value_is_singleton(FILE *fptr, const char *key, const int start,
                                const size_t str_max)
{
  yaml_ff_to_key(fptr, key, start, str_max);

  char c;
  do c = fgetc(fptr);
  while (ISWHITESPACE(c));

  if (c == EOF) {
    fprintf(stderr, "Could not find values for key %s in structure file.\n", key);
    return YAML__ERROR_VALUE;
  }

  if (c == '{') {
    return 0;
  } else {
    return 1;
  }
}
