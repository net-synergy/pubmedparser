#include <stdio.h>
#include <string.h>

#include "structure.h"

#include "yaml_reader.h"
#include "error.h"

static void read_elements(FILE *fptr, path_struct parent, const int fpos,
                          const size_t str_max);

static void get_names(FILE *fptr, const int fpos, char ***names,
                      size_t *n_names, const size_t str_max)
{
  *n_names = 0;
  int rc = 0;

  rc = yaml_get_keys(fptr, names, n_names, fpos, str_max);
  if (rc > 0) {
    pubmedparser_error(rc, "Error reading keys from structure file");
  }

  char **keys = *names;
  size_t i = 0;
  for (i = 0; (i < *n_names) && (strcmp(keys[i], "root") != 0); i++);

  if (i == *n_names) {
    pubmedparser_error(PP_ERR_STRUCTURE_KEY,
                       "Structure file must contain a key named \"root\"\n");
  }

  char *swap = NULL;
  for (size_t j = i; j > 0; j--) {
    swap = keys[j - 1];
    keys[j - 1] = keys[j];
    keys[j] = swap;
  }

  for (i = 0; (i < *n_names) && (strcmp(keys[i], "key") != 0); i++);

  if (i == *n_names) {
    pubmedparser_error(PP_ERR_STRUCTURE_KEY,
                       "Structure file must contain a key named \"key\"\n");
  }

  for (size_t j = i; j > 1; j--) {
    swap = keys[j - 1];
    keys[j - 1] = keys[j];
    keys[j] = swap;
  }

  *names = keys;
}

static path_struct read_element(FILE *fptr, const char *name,
                                path_struct parent, const int fpos, const size_t str_max)
{
  struct PathStructure el_init;

  el_init.name = strdup(name);
  el_init.parent = parent;

  if (yaml_map_value_is_singleton(fptr, name, fpos, str_max)) {
    char xml_path[str_max];
    yaml_get_map_value(fptr, name, xml_path, fpos, str_max);
    el_init.path = strdup(xml_path);
    el_init.children = NULL;
    el_init.n_children = 0;
  } else {
    el_init.path = NULL;
    int fpos = ftell(fptr);
    read_elements(fptr, &el_init, fpos, str_max);
  }

  path_struct element = malloc(sizeof(*element));
  memcpy(element, &el_init, sizeof(*element));
  return element;
}

static void read_elements(FILE *fptr, path_struct parent, const int fpos,
                          const size_t str_max)
{
  size_t n_names = 0;
  char **names;

  get_names(fptr, fpos, &names, &n_names, str_max);

  path_struct *children = malloc(sizeof(*children) * n_names);
  for (size_t i = 0; i < n_names; i++) {
    children[i] = read_element(fptr, names[i], parent, fpos, str_max);
  }
  parent->children = children;
  parent->n_children = n_names;

  free(names);
}

static void path_struct_print_i(const path_struct ps, const size_t depth)
{
  char tab[depth + 1];
  for (size_t i = 0; i < depth; i++) {
    tab[i] = ' ';
  }
  tab[depth] = '\0';

  printf("%s%s: ", tab, ps->name);
  if (ps->path != NULL) {
    printf("%s", ps->path);
  }
  printf("\n");

  for (size_t i = 0; i < ps->n_children; i++) {
    printf("%s", tab);
    path_struct_print_i(ps->children[i], depth + 1);
  }
}

void path_struct_print(const path_struct ps)
{
  path_struct_print_i(ps, 0);
}

path_struct parse_structure_file(const char *structure_file,
                                 const size_t str_max)
{
  FILE *fptr;
  struct PathStructure top;
  top.name = strdup("top");
  top.parent = NULL;
  top.path = NULL;

  if (!(fptr = fopen(structure_file, "r"))) {
    pubmedparser_error(1, "Could not open structure file");
  }

  read_elements(fptr, &top, 0, str_max);

  path_struct ret = malloc(sizeof(*ret));
  memcpy(ret, &top, sizeof(*ret));
  return ret;
};

void path_struct_destroy(path_struct ps)
{
  for (size_t i = 0; i < ps->n_children; i++) {
    path_struct_destroy(ps->children[i]);
  }

  free(ps->name);
  if (ps->path != NULL) {
    free(ps->path);
  }

  free(ps);
};
