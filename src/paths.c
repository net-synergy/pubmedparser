#include "paths.h"
#include "yaml_reader.h"

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
    fprintf(stderr, "Path malformed. Most start with '/'");
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

static FILE *get_file(const char *name, const char *cache_dir)
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
    fprintf(stderr, "Could not find subtag; malformed path.\n");
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

  node n_init = {
    .name = strdup(name),
    .path = p,
    .values = values,
    .n_values = n_values,
    .sub_tags = (const char **)sub_tags,
    .n_sub_tags = n_sub_tags,
    .attribute = attribute_holder[0],
    .expected_attribute = attribute_holder[1],
    .out = get_file(name, cache_dir)
  };

  node *n = malloc(sizeof * n);
  memcpy(n, &n_init, sizeof * n);

  return n;
}

static void release_node(node *node)
{
  free((char *)node->name);
  for (int i = 0; i < node->n_values; i++) {
    free(node->values[i]);
  }
  for (int i = 0; i < node->n_sub_tags; i++) {
    free((char *)node->sub_tags[i]);
  }
  free((char *)node->attribute);
  free((char *)node->expected_attribute);

  fclose(node->out);
}

#define N_NAMES 3
node_set *construct_node_set(char *structure_file, char *cache_dir,
                             int str_max)
{
  char root[str_max];
  char *keys[N_NAMES] = { "key", "key_features", "nodes" };
  size_t n_keys[N_NAMES];
  char **key_values_pairs[N_NAMES][2];
  int rc = 0;

  yaml_get_map_value(structure_file, "root", root, str_max);
  for (int i = 0; i < N_NAMES; i++) {
    rc &= yaml_get_map_contents(structure_file, keys[i],
                                key_values_pairs[i], &n_keys[i]);
  }

  if (rc) {
    fprintf(stderr, "Structure file not formatted correctly; terminating.\n");
    exit(rc);
  }

  if (n_keys[0] > 1) {
    fprintf(stderr, "Too many key values in %s. Must have exactly one key.\n",
            structure_file);
    exit(1);
  }

  size_t n_nodes = 0;
  for (int i = 0; i < N_NAMES; i++) {
    n_nodes += n_keys[i];
  }

  char *names[n_nodes];
  char *xpaths[n_nodes];

  int pos = 0;
  for (int i = 0; i < N_NAMES; i++) {
    for (int j = 0; j < (int)n_keys[i]; j++) {
      names[pos] = key_values_pairs[i][0][j];
      xpaths[pos] = key_values_pairs[i][1][j];
      pos++;
    }
  }

  node **nodes = malloc(sizeof * nodes * n_nodes);
  for (int i = 0; i < (int)n_nodes; i++)
    nodes[i] = construct_node(xpaths[i], names[i], str_max, cache_dir);

  int max_p_depth = 0;
  for (int i = 0; i < (int)n_nodes; i++)
    max_p_depth = (max_p_depth > nodes[i]->path->length) ? max_p_depth :
                  nodes[i]->path->length;

  node_set ns_init = {
    .root = strdup(root),
    .max_path_depth = max_p_depth,
    .key_idx = 0,
    .nodes = nodes,
    .n = (int)n_nodes
  };

  node_set *ns = malloc(sizeof * ns);
  memcpy(ns, &ns_init, sizeof * ns);

  for (int i = 0; i < N_NAMES; i++) {
    for (int j = 0; j < (int)n_keys[i]; j++) {
      free(key_values_pairs[i][0][j]);
      free(key_values_pairs[i][1][j]);
    }
    free(key_values_pairs[i][0]);
    free(key_values_pairs[i][1]);
  }

  return ns;
}

void release_node_set(node_set *ns)
{
  for (int i = 0; i < ns->n; i++)
    release_node(ns->nodes[i]);

  free(ns->nodes);
  free((char *)ns->root);
  free(ns);
}

node_set *clone_node_set(node_set *ns, char *cache_dir, int thread,
                         int str_max)
{
  node_set *dup_ns = malloc(sizeof * dup_ns);
  memcpy(dup_ns, ns, sizeof * ns);

  dup_ns->nodes = malloc(sizeof * ns->nodes * ns->n);
  for (int i = 0; i < dup_ns->n; i++) {
    dup_ns->nodes[i] = malloc(sizeof * ns->nodes[i]);
    memcpy(dup_ns->nodes[i], ns->nodes[i], sizeof * dup_ns->nodes[i]);
    dup_ns->nodes[i]->name = malloc(sizeof(char *) * str_max);
    sprintf((char *)dup_ns->nodes[i]->name, "%s_%d", ns->nodes[i]->name, thread);
    dup_ns->nodes[i]->values = malloc(sizeof * ns->nodes[i]->values *
                                      ns->nodes[i]->n_values);

    for (int j = 0; j < ns->nodes[i]->n_values; j++)
      dup_ns->nodes[i]->values[j] = malloc(sizeof(char *) * str_max);

    dup_ns->nodes[i]->out = get_file(dup_ns->nodes[i]->name, cache_dir);
  }

  return dup_ns;
}

void release_clone(node_set *ns)
{
  for (int i = 0; i < ns->n; i++) {
    for (int j = 0; j < ns->nodes[i]->n_values; j++)
      free(ns->nodes[i]->values[j]);

    free(ns->nodes[i]->values);
    fclose(ns->nodes[i]->out);
    free(ns->nodes[i]);
  }
  free(ns);
}
