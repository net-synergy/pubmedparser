#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "nodes.h"
#include "error.h"

enum {
  ATT_NONE = 1,
  ATT_FOUND,
  ATT_EXPECTED
};

static node *node_generate(const path_struct ps, const size_t str_max,
                           const char *cache_dir);

static FILE *get_file(const char *name, const char *cache_dir)
{
  FILE *fptr = malloc(sizeof(*fptr));
  char out[8000];

  strcpy(out, cache_dir);
  strcat(out, name);
  strcat(out, ".tsv");

  fptr = fopen(out, "a");
  return fptr;
}

static size_t find_sub_tag_names(const char *p, size_t str_max,
                                 char ***sub_tags_holder)
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
    pubmedparser_error(3, "Could not find subtag; malformed path.\n");
  }

  sub_tags_holder[0] = malloc(sizeof(char *) * count);
  char tag[str_max];
  for (int j = 0; j < count; j++) {
    for (i = 0; (*p != ',') && (*p != '}') && (i < (str_max - 1)); i++, p++) {
      tag[i] = *p;
    }
    p++;
    tag[i] = '\0';
    sub_tags_holder[0][j] = strdup(tag);
  }

  return count;
}

static void find_attribute_name(const char *p, char **attribute,
                                char **expected_attribute, const size_t str_max)
{
  while ((*p != '\0') && (*p != '@') && (*p != '['))
    p++;

  *attribute = NULL;
  *expected_attribute = NULL;
  if (*p == '\0') {
    return;
  }

  int att_type;
  if (*p == '@') {
    att_type = ATT_FOUND;
  } else {
    att_type = ATT_EXPECTED;
    p++; /* Skip [ */
  }

  *attribute = malloc(sizeof(**attribute) * str_max);
  p++; /* Skip @ */
  size_t i;
  for (i = 0; (p[i] != '\0') && (p[i] != '=') && (i < (str_max - 1)); i++)
    (*attribute)[i] = p[i];
  (*attribute)[i] = '\0';

  if (att_type == ATT_EXPECTED) {
    *expected_attribute = malloc(sizeof(**expected_attribute) * str_max);
    p += i;
    while ((*p == '=') || (*p == '\'') || (*p == ' '))
      p++;

    for (i = 0; (p[i] != '\'') && (p[i] != ']') && (i < (str_max - 1)); i++)
      (*expected_attribute)[i] = p[i];
    (*expected_attribute)[i] = '\0';
  }
}

static value value_init(const char *xml_path, const size_t str_max)
{
  char *attribute;
  char *expected_attribute;
  find_attribute_name(xml_path, &attribute, &expected_attribute,
                      str_max);

  struct Value val_init = {
    .pos = {-1, 0},
    .att_pos = {-1, 0},
    .attribute = attribute,
    .desired_attribute = expected_attribute
  };

  value val = malloc(sizeof(*val));
  memcpy(val, &val_init, sizeof(*val));
  return val;
}

static void value_destroy(value val)
{
  free((char *)val->attribute);
  free((char *)val->desired_attribute);
  free(val);
}

static node_set *node_set_generate_from_sub_tags(const char *xml_path,
    char **sub_tags, const size_t n_sub_tags, const char *cache_dir,
    const size_t str_max)
{
  path p = path_init(xml_path, str_max);
  char *path_str = malloc(sizeof(*path_str) * str_max);

  for (size_t i = 0; i < p->length; i++) {
    strncat(path_str, "/", str_max);
    strncat(path_str, p->components[i], str_max);
  }

  struct PathStructure ps = {
    .name = strdup(p->components[p->length - 1]),
    .path = NULL,
    .parent = NULL,
    .children = malloc(sizeof(path_struct) * (n_sub_tags + 2)),
    .n_children = n_sub_tags + 2,
  };

  for (size_t i = 0; i < ps.n_children; i++) {
    ps.children[i] = malloc(sizeof(struct PathStructure));
  }

  ps.children[0]->name = strdup("root");
  ps.children[0]->path = path_str;
  ps.children[0]->parent = &ps;
  ps.children[0]->children = NULL;
  ps.children[0]->n_children = 0;

  ps.children[1]->name = strdup("key");
  ps.children[1]->path = strdup("//condensed");
  ps.children[1]->parent = &ps;
  ps.children[1]->children = NULL;
  ps.children[1]->n_children = 0;

  char **sub_tag_paths = malloc(sizeof(*sub_tag_paths) * n_sub_tags);
  for (size_t i = 0; i < n_sub_tags; i++) {
    sub_tag_paths[i] = malloc(sizeof(*sub_tag_paths[i]) * str_max);
    strncpy(sub_tag_paths[i], "//", str_max);
    strncat(sub_tag_paths[i], sub_tags[i], str_max);

    ps.children[i + 2]->name = sub_tags[i];
    ps.children[i + 2]->path = sub_tag_paths[i];
    ps.children[i + 2]->parent = &ps;
    ps.children[i + 2]->children = NULL;
    ps.children[i + 2]->n_children = 0;
  }

  node_set *ns = node_set_generate(&ps, ps.name, cache_dir, str_max);

  for (size_t i = 0; i < n_sub_tags; i++) {
    free(sub_tag_paths[i]);
  }
  free(sub_tag_paths);
  free(ps.name);
  for (size_t i = 0; i < ps.n_children; i++) {
    free(ps.children[i]);
  }

  return ns;
};

static node *node_generate(const path_struct ps, const size_t str_max,
                           const char *cache_dir)
{
  char **sub_tags;
  node_set *ns = NULL;
  value v = NULL;
  path p;

  if (ps->n_children > 0) {
    p = path_init(ps->children[0]->path, str_max);
    /* By dropping the last component of the path then calling that the root we
    can use the same pattern as in the top level where we search for the first
    instance of root then loop until hitting the root end tag. This allows us to
    use recursion in the main parser. */
    p->length--;
    ps->children[0]->name = p->components[p->length];

    ns = node_set_generate(ps, ps->name, cache_dir, str_max);
  } else {
    size_t n_sub_tags = find_sub_tag_names(ps->path, str_max, &sub_tags);
    p = path_init(ps->path, str_max);
    if (n_sub_tags > 0) {
      ns = node_set_generate_from_sub_tags(ps->path, sub_tags, n_sub_tags,
                                           cache_dir, str_max);
    } else {
      v = value_init(ps->path, str_max);
    }
  }

  node n_init = {
    .name = strdup(ps->name),
    .path = p,
    .value = v,
    .child_ns = ns,
    .out = get_file(ps->name, cache_dir)
  };

  node *n = malloc(sizeof * n);
  memcpy(n, &n_init, sizeof * n);

  return n;
}

static void node_destroy(node *n)
{
  free((char *)n->name);
  path_destroy((path)n->path);
  if (n->value != NULL) {
    value_destroy(n->value);
  }

  if (n->child_ns != NULL) {
    node_set_destroy(n->child_ns);
  }

  fclose(n->out);
  free(n);
}

node_set *node_set_generate(const path_struct ps, const char *name_prefix,
                            const char *cache_dir, const size_t str_max)
{
  if (name_prefix != NULL) {
    char *new_name;
    char *old_name;
    char *name_prefix_i = strdup(name_prefix);
    strcat(name_prefix_i, "_");
    for (size_t i = 0; i < (ps->n_children - 1); i++) {
      new_name = malloc(sizeof(*new_name) * str_max);
      old_name = ps->children[i + 1]->name;
      strncpy(new_name, name_prefix_i, str_max);
      strncat(new_name, old_name, str_max);
      ps->children[i + 1]->name = new_name;
      free(old_name);
    }
    free(name_prefix_i);
  }

  node **nodes = malloc(sizeof(*nodes) * (ps->n_children - 1));
  for (size_t i = 0; i < (ps->n_children - 1); i++) {
    nodes[i] = node_generate(ps->children[i + 1], str_max, cache_dir);
  }

  size_t max_p_depth = 0;
  for (size_t i = 0; i < (ps->n_children - 1); i++) {
    max_p_depth = (max_p_depth > nodes[i]->path->length) ? max_p_depth :
                  nodes[i]->path->length;
  }

  node_set ns_init = {
    .root = strdup(ps->children[0]->path),
    .key_idx = 0, // Always 0 since get_names moves it to 0 if it's not.
    .nodes = nodes,
    .n_nodes = ps->n_children - 1,
    .max_path_depth = max_p_depth,
  };

  node_set *ns = malloc(sizeof * ns);
  memcpy(ns, &ns_init, sizeof * ns);

  return ns;
}

void node_set_destroy(node_set *ns)
{
  for (size_t i = 0; i < ns->n_nodes; i++) {
    node_destroy(ns->nodes[i]);
  }

  free(ns->nodes);
  free((char *)ns->root);
  free(ns);
}

static value value_clone(const value v)
{
  char *att = NULL;
  char *desired_att = NULL;
  if (v->attribute != NULL) {
    att = strdup(v->attribute);
  }
  if (v->desired_attribute) {
    desired_att = strdup(v->desired_attribute);
  }

  struct Value dup_v_init = {
    .attribute = att,
    .desired_attribute = desired_att,
    .pos = {-1, 0}, // Don't care about transferring pos since it's transient.
    .att_pos = {-1, 0}
  };
  value dup_v = malloc(sizeof(*dup_v));
  memcpy(dup_v, &dup_v_init, sizeof(*dup_v));

  return dup_v;
}

static path path_clone(const path p)
{
  path dup_p = malloc(sizeof(*dup_p));
  memcpy(dup_p, p, sizeof(*dup_p));
  dup_p->components = malloc(sizeof(*dup_p->components) * dup_p->length);
  for (size_t i = 0; i < p->length; i++) {
    dup_p->components[i] = p->components[i];
  }

  return dup_p;
}

static node *node_clone(const node *n, const char *cache_dir,
                        const int thread, const size_t str_max)
{
  value v = NULL;
  node_set *child_ns = NULL;
  char name[str_max];

  sprintf(name, "%s_%d", n->name, thread);
  if (n->value != NULL) {
    v = value_clone(n->value);
  }

  if (n->child_ns != NULL) {
    child_ns = node_set_clone(n->child_ns, cache_dir, thread, str_max);
  }

  node dup_n_init = {
    .name = strdup(name),
    .path = path_clone(n->path),
    .value = v,
    .child_ns = child_ns,
    .out = get_file(name, cache_dir)
  };
  node *dup_n = malloc(sizeof(*dup_n));
  memcpy(dup_n, &dup_n_init, sizeof(*dup_n));

  return dup_n;
}

node_set *node_set_clone(const node_set *ns, const char *cache_dir,
                         const size_t thread, const size_t str_max)
{
  node_set *dup_ns = malloc(sizeof(*dup_ns));
  memcpy(dup_ns, ns, sizeof(*ns));
  dup_ns->root = strdup(ns->root);

  dup_ns->nodes = malloc(sizeof(*ns->nodes) * ns->n_nodes);
  for (size_t i = 0; i < dup_ns->n_nodes; i++) {
    dup_ns->nodes[i] = node_clone(ns->nodes[i], cache_dir, thread, str_max);
  }

  return dup_ns;
}
