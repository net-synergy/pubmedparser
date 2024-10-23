#include "yaml_reader.h"

#include "error.h"

#include <stdlib.h>
#include <string.h>

#define BLOCK_MAX 50000
#define ISWHITESPACE(c) ((c == ' ') || (c == '\n') || (c == '\t'))
#define ISALPHA(c) (((c >= 'a') && (c <= 'z')) || ((c >= 'A') && (c <= 'Z')))

static void yaml_rewind_to_start_of_line(FILE* fptr)
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

static int yaml_get_key(char* buffer, size_t const max_size, FILE* fptr)
{
  char c;

  do {
    c = fgetc(fptr);
  } while (!ISALPHA(c) && c != EOF);

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
    pubmedparser_error(PP_ERR_BUFFER_OVERFLOW, "%s",
      "Buffer too small to fit key. Increase buffer size to get "
      "entire key.");
    return EOF - 1;
  }

  return c;
}

static int yaml_get_value(char* buffer, size_t const max_size, FILE* fptr)
{
  char c;

  do {
    c = fgetc(fptr);
  } while ((c == ' ') || (c == '\t') || (c == '{'));

  if (c == '}' || c == EOF || c == '\n') {
    pubmedparser_error(PP_ERR_VALUE, "%s", "Found malformed value.");
    return EOF - 1;
  }

  if (c == '{') {
    do {
      c = fgetc(fptr);
    } while (ISWHITESPACE(c));
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
    while (c != ',' && c != '\n' && c != '}' && i < max_size && c != EOF) {
      buffer[i] = c;
      i++;
      c = fgetc(fptr);
    };
  }

  if (c == EOF) {
    pubmedparser_error(PP_ERR_EOF, "");
    return EOF - 1;
  }

  if (i == max_size) {
    buffer[i - 1] = '\0';
    pubmedparser_error(PP_ERR_BUFFER_OVERFLOW, "%s",
      "Value was larger than buffer. Increase buffer size to get full value.");
    return EOF - 1;
  }

  while (ISWHITESPACE(buffer[i - 1])) {
    i--;
  }
  buffer[i] = '\0';

  if (c == EOF) {
    pubmedparser_error(PP_ERR_EOF, "");
    return EOF - 1;
  }

  return c;
}

static int next_line_depth(FILE* fptr)
{
  char c = fgetc(fptr);
  int depth = 0;

  while (c != '\n' && c != EOF) {
    c = fgetc(fptr);
  }

  if (c == EOF) {
    return EOF;
  }

  while (ISWHITESPACE(c)) {
    depth++;
    if (c == '\n') {
      depth = 0;
    }
    c = fgetc(fptr);
  }

  if (c == EOF) {
    return EOF;
  }

  ungetc(c, fptr);
  return depth;
}

pp_errno yaml_get_keys(FILE* fptr, char*** keys, size_t* n_keys,
  int const start, size_t const str_max)
{
  fseek(fptr, start, SEEK_SET);
  char buff[str_max];
  char c;
  *n_keys = 0;

  int initial_depth = 0;
  yaml_rewind_to_start_of_line(fptr);
  for (c = fgetc(fptr); ISWHITESPACE(c); c = fgetc(fptr), initial_depth++)
    ;
  yaml_rewind_to_start_of_line(fptr);

  int depth = initial_depth;
  while (((c = yaml_get_key(buff, str_max, fptr)) != EOF) &&
         (depth >= initial_depth)) {
    if (c == (EOF - 1)) {
      return c;
    }
    (*n_keys)++;

    do {
      (depth = next_line_depth(fptr));
    } while ((depth > initial_depth));
  }

  if ((depth == EOF) && (initial_depth != 0)) {
    pubmedparser_error(PP_ERR_EOF, "%s",
      "End of file while parsing key value in structure file\n. "
      "Possibly a missing \"}\"");
    return PP_ERR_EOF;
  }

  *keys = malloc(sizeof **keys * (*n_keys));
  if (!keys) {
    pubmedparser_error(PP_ERR_OOM, "");
    return EOF - 1;
  }

  fseek(fptr, start, SEEK_SET);
  for (size_t k = 0; k < (*n_keys); k++) {
    c = yaml_get_key(buff, str_max, fptr);
    if (c == (EOF - 1)) {
      return c;
    }
    (*keys)[k] = strdup(buff);

    do {
      (c = fgetc(fptr));
    } while (ISWHITESPACE(c));

    do {
      depth = next_line_depth(fptr);
    } while ((depth > initial_depth));
  }

  return PP_SUCCESS;
}

static void yaml_ff_to_key(
  FILE* fptr, char const* key, int const start, size_t const str_max)
{
  fseek(fptr, start, SEEK_SET);
  char buff[str_max];
  char c;

  do {
    c = yaml_get_key(buff, str_max, fptr);
  } while (strcmp(buff, key) != 0 && c != EOF);

  if (c == EOF) {
    pubmedparser_error(
      PP_ERR_KEY, "Could not find key %s in structure file.", key);
  }
}

pp_errno yaml_get_map_value(FILE* fptr, char const* key, char* value,
  int const start, size_t const str_max)
{
  yaml_ff_to_key(fptr, key, start, str_max);
  return yaml_get_value(value, str_max, fptr);
}

int yaml_map_value_is_singleton(
  FILE* fptr, char const* key, int const start, size_t const str_max)
{
  yaml_ff_to_key(fptr, key, start, str_max);

  char c;
  do {
    c = fgetc(fptr);
  } while (ISWHITESPACE(c));

  if (c == EOF) {
    pubmedparser_error(PP_ERR_EOF, "");
    return EOF - 1;
  }

  return c == '{' ? 0 : 1;
}
