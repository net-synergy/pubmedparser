#include "error.h"
#include "read_xml.h"
#include "structure.h"
#include "yaml_reader.h"

#include <stdio.h>
#include <string.h>

static void read_elements(
  FILE* fptr, path_struct parent, int const fpos, size_t const str_max);

static void get_names(FILE* fptr, int const fpos, char*** names,
  size_t* n_names, size_t const str_max)
{
  *n_names = 0;
  pp_errno rc = 0;

  rc = yaml_get_keys(fptr, names, n_names, fpos, str_max);
  if (rc != PP_SUCCESS) {
    pubmedparser_error(rc, "Error reading keys from structure file");
    return;
  }

  char** keys = *names;
  size_t i = 0;
  for (i = 0; (i < *n_names) && (strcmp(keys[i], "root") != 0); i++)
    ;

  if (i == *n_names) {
    pubmedparser_error(
      PP_ERR_KEY, "Structure file must contain a key named \"root\".");
    return;
  }

  char* swap = NULL;
  for (size_t j = i; j > 0; j--) {
    swap = keys[j - 1];
    keys[j - 1] = keys[j];
    keys[j] = swap;
  }

  for (i = 0; (i < *n_names) && (strcmp(keys[i], "key") != 0); i++)
    ;

  if (i == *n_names) {
    pubmedparser_error(
      PP_ERR_KEY, "Structure file must contain a key named \"key\".");
    return;
  }

  for (size_t j = i; j > 1; j--) {
    swap = keys[j - 1];
    keys[j - 1] = keys[j];
    keys[j] = swap;
  }

  *names = keys;
}

static path_struct read_element(FILE* fptr, char const* name,
  path_struct parent, int const fpos, size_t const str_max)
{
  path_struct element = malloc(sizeof(*element));
  if (!element) {
    pubmedparser_error(PP_ERR_OOM, "");
    return NULL;
  }

  element->name = strdup(name);
  element->parent = parent;

  if (yaml_map_value_is_singleton(fptr, name, fpos, str_max)) {
    char xml_path[str_max];
    yaml_get_map_value(fptr, name, xml_path, fpos, str_max);
    element->path = strdup(xml_path);
    element->children = NULL;
    element->n_children = 0;
  } else {
    element->path = NULL;
    int fpos = ftell(fptr);
    read_elements(fptr, element, fpos, str_max);
  }

  return element;
}

static void read_elements(
  FILE* fptr, path_struct parent, int const fpos, size_t const str_max)
{
  size_t n_names = 0;
  char** names;

  get_names(fptr, fpos, &names, &n_names, str_max);

  path_struct* children = malloc(sizeof(*children) * n_names);
  if (!children) {
    pubmedparser_error(PP_ERR_OOM, "");
    return;
  }

  for (size_t i = 0; i < n_names; i++) {
    children[i] = read_element(fptr, names[i], parent, fpos, str_max);
  }
  parent->children = children;
  parent->n_children = n_names;

  for (size_t i = 0; i < n_names; i++) {
    free(names[i]);
  }
  free(names);
}

static void path_struct_print_i(path_struct const ps, size_t const depth)
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

void path_struct_print(path_struct const ps) { path_struct_print_i(ps, 0); }

path_struct parse_structure_file(
  char const* structure_file, size_t const str_max)
{
  FILE* fptr;
  path_struct ret = malloc(sizeof(*ret));
  ret->path = NULL;
  ret->children = NULL;

  if (!ret) {
    pubmedparser_error(PP_ERR_OOM, "");
    return NULL;
  }

  ret->name = strdup("top");

  if (!(fptr = fopen(structure_file, "r"))) {
    pubmedparser_error(0, "Could not open structure file.");
    return NULL;
  }

  read_elements(fptr, ret, 0, str_max);

  return ret;
};

void path_struct_destroy(path_struct ps)
{
  for (size_t i = 0; i < ps->n_children; i++) {
    path_struct_destroy(ps->children[i]);
  }
  free(ps->children);

  free(ps->name);
  if (ps->path) {
    free(ps->path);
  }

  free(ps);
};
