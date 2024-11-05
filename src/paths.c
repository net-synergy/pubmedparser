#include "paths.h"

#include "error.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define IS_SPECIAL(p) ((p == '@') || (p == '{') || (p == '['))

static int n_components(char const* p)
{
  int n = 0;
  while (*p != '\0' && !(IS_SPECIAL(*p))) {
    if (*p == '/') {
      n++;
    }
    p++;
  }

  if (IS_SPECIAL(*p)) {
    n--;
  }

  return n;
}

static void get_components(
  char const* p, char** components, size_t const str_max)
{
  size_t tag_i = 0;
  size_t comp_i = 0;
  char name[str_max];
  if (*p != '/') {
    pubmedparser_error(0, "Path malformed. Most start with '/'");
  }
  p++; // Strip initial '/';
  while ((*p != '\0') && !IS_SPECIAL(*p)) {
    if (*p == '/') {
      name[tag_i] = '\0';
      components[comp_i] = strdup(name);
      comp_i++;
      tag_i = 0;
    } else if (tag_i < (str_max - 1)) {
      name[tag_i] = *p;
      tag_i++;
    }
    p++;
  }

  if (!IS_SPECIAL(*p)) {
    name[tag_i] = '\0';
    components[comp_i] = strdup(name);
  }
}

path path_init(char const* xml_path, size_t const str_max)
{
  int length = n_components(xml_path);
  char** components = malloc(sizeof(char*) * length);
  get_components(xml_path, components, str_max);

  struct Path p = {
    .length = length,
    .components = components,
    .max_path_depth = length,
  };

  path out = malloc(sizeof(*out));
  if (!out) {
    pubmedparser_error(PP_ERR_OOM, "");
  }

  memcpy(out, &p, sizeof(*out));

  return out;
}

#define MIN(a, b) (a) > (b) ? (b) : (a)

void path_destroy(path p)
{
  int length = MIN((int)p->length, (int)p->max_path_depth);
  for (int i = 0; i < length; i++) {
    free(p->components[i]);
  }
  free(p->components);
  free(p);
}

#undef MIN

path path_init_dynamic(size_t const max_path_depth)
{
  struct Path p = {
    .components = malloc(sizeof(char*) * max_path_depth),
    .length = 0,
    .max_path_depth = max_path_depth,
  };

  path out = malloc(sizeof(*out));
  memcpy(out, &p, sizeof(p));

  return out;
}

void path_drop_last_component(path p)
{
  p->length--;
  if (p->length < p->max_path_depth) {
    free(p->components[p->length]);
  }
}

void path_append(path p, tag const* t)
{
  if (p->length < p->max_path_depth) {
    p->components[p->length] = strdup(t->value);
  }
  p->length++;
}

int path_match(path const p1, path const p2)
{
  if (p1->length != p2->length) {
    return 0;
  }

  int i = p1->length;
  while (
    (i > 0) && (strcmp(p1->components[i - 1], p2->components[i - 1]) == 0)) {
    i--;
  }

  return i == 0;
}

inline int path_is_empty(path const p) { return (int)p->length == -1; }

void path_print(path const p)
{
  char sep = '/';
  size_t len = (p->length < p->max_path_depth) ? p->length : p->max_path_depth;

  if (path_is_empty(p)) {
    printf("\n");
    return;
  }

  for (size_t i = 0; i < len; i++) {
    printf("%c%s", sep, p->components[i]);
  }
  printf("\n");
}
