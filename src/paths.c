#include "paths.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define IS_SPECIAL(p) ((p == '@') || (p == '{') || (p == '['))

#define NOATT 0
#define ATT 1
#define EXPATT 2

static int n_components(char *p)
{
  int n = 0;
  while (*p != '\0' && !(IS_SPECIAL(*p))) {
    if (*p == '/') n++;
    p++;
  }

  if (IS_SPECIAL(*p)) {
    n--;
  }

  return n;
}

static void get_components(char *p, char **components, int str_max)
{
  int tag_i = 0;
  int comp_i = 0;
  char name[str_max];
  if (*p != '/') {
    printf("Path malformed. Must start with '/'");
    exit(2);
  }
  p++; // Strip initial '/';
  while (*p != '\0' && !IS_SPECIAL(*p)) {
    if (*p == '/') {
      name[tag_i] = '\0';
      components[comp_i] = strdup(name);
      comp_i++;
      tag_i = 0;
    } else {
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

static FILE *get_file(char *name, char *cache_dir)
{
  FILE *fptr = malloc(sizeof(FILE));
  char out[100];
  strcpy(out, cache_dir);
  strcat(out, name);
  strcat(out, ".tsv");

  fptr = fopen(out, "a");
  return fptr;
}

static path *construct_path(char *xml_path, int str_max)
{
  int length = n_components(xml_path);
  char **components = malloc(sizeof(char *) * length);
  get_components(xml_path, components, str_max);

  path *p = malloc(sizeof(path));
  p->length = length;
  p->components = components;
  return p;
}

static int find_attribute_name(char *p, int str_max, char **attribute_holder)
{
  while ((*p != '\0') && (*p != '@') && (*p != '['))
    p++;

  attribute_holder[0] = NULL;
  attribute_holder[1] = NULL;
  if (*p == '\0') {
    return NOATT;
  }

  int att_type;
  if (*p == '@') {
    att_type = ATT;
  } else {
    att_type = EXPATT;
    p++; /* Skip [ */
  }

  p++; /* Skip @ */
  char attribute[str_max];
  int i;
  for (i = 0; (p[i] != '\0') && (p[i] != '=') && (i < (str_max - 1)); i++)
    attribute[i] = p[i];
  attribute[i] = '\0';

  attribute_holder[0] = strdup(attribute);

  if (att_type == EXPATT) {
    p += i;
    while ((*p == '=') || (*p == '\'') || (*p == ' '))
      p++;

    for (i = 0; (p[i] != '\'') && (p[i] != ']') && (i < (str_max - 1)); i++)
      attribute[i] = p[i];
    attribute[i] = '\0';

    attribute_holder[1] = strdup(attribute);
  }

  return att_type;
}

static int find_sub_tag_names(char *p, int str_max, char ***sub_tags_holder)
{
  while ((*p != '\0') && (*p != '{'))
    p++;

  if (*p == '\0') {
    sub_tags_holder[0] = NULL;
    return 0;
  }

  p++; /* Skip { */
  int count = 1;
  int i = 0;
  while ((p[i] != '}') && (p[i] != '\0')) {
    if (p[i] == ',')
      count++;
    i++;
  }

  if (p[i] == '\0') {
    fprintf(stderr, "Malformed path");
    exit(3);
  }

  sub_tags_holder[0] = malloc(sizeof(char *) * count);
  char tag[str_max];
  for (int j = 0; j < count; j++) {
    for (i = 0; (*p != ',') && (*p != '}'); i++, p++) {
      tag[i] = *p;
    }
    p++;
    tag[i] = '\0';
    sub_tags_holder[0][j] = strdup(tag);
  }

  return count;
}

static node *construct_node(char *xml_path, char *name, int str_max,
                            char *cache_dir)
{
  path *p = construct_path(xml_path, str_max);
  char *attribute_holder[2];
  int att_type = find_attribute_name(xml_path, str_max, attribute_holder);
  char **sub_tags_holder[1];
  int n_sub_tags = find_sub_tag_names(xml_path, str_max, sub_tags_holder);
  char **sub_tags = sub_tags_holder[0];

  int n_values = 0;
  if (att_type == ATT) {
    n_values++;
  }

  if (n_sub_tags > 0) {
    n_values += n_sub_tags;
  } else {
    n_values++;
  }

  char **values = malloc(sizeof(char *) * n_values);
  for (int i = 0; i < n_values; i++) {
    values[i] = malloc(sizeof(char) * str_max);
  }

  node *n = malloc(sizeof(node));
  n->name = strdup(name);
  n->path = p;
  n->values = values;
  n->n_values = n_values;
  n->sub_tags = sub_tags;
  n->n_sub_tags = n_sub_tags;
  n->attribute = attribute_holder[0];
  n->expected_attribute = attribute_holder[1];
  n->out = get_file(name, cache_dir);
  return n;
}

node_set *construct_node_set(char *root, char **xpaths, int n_nodes,
                             char **names, int key_idx, int str_max, char *cache_dir)
{
  node **nodes = malloc(sizeof(node *) * n_nodes);

  for (int i = 0; i < (n_nodes); i++)
    nodes[i] = construct_node(xpaths[i], names[i], str_max, cache_dir);

  int max_p_depth = 0;
  for (int i = 0; i < n_nodes; i++)
    max_p_depth = (max_p_depth > nodes[i]->path->length) ? max_p_depth :
                  nodes[i]->path->length;

  node_set *ns = malloc(sizeof(node_set));
  ns->root = strdup(root);
  ns->max_path_depth = max_p_depth;
  ns->key_idx = key_idx;
  ns->nodes = nodes;
  ns->n = n_nodes;

  return ns;
}
