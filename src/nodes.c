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
  size_t count = 1;
  size_t i = 0;
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
  for (size_t j = 0; j < count; j++) {
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
    .attribute_name = attribute,
    .required_attribute_value = expected_attribute
  };

  value val = malloc(sizeof(*val));
  memcpy(val, &val_init, sizeof(*val));
  return val;
}

static void value_destroy(value val)
{
  free((char *)val->attribute_name);
  free((char *)val->required_attribute_value);
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
  ps.children[0]->path = strdup(ps.name);
  ps.children[0]->parent = &ps;
  ps.children[0]->children = NULL;
  ps.children[0]->n_children = 0;

  ps.children[1]->name = strdup("key");
  ps.children[1]->path = strdup("/condensed");
  ps.children[1]->parent = &ps;
  ps.children[1]->children = NULL;
  ps.children[1]->n_children = 0;

  char **sub_tag_paths = malloc(sizeof(*sub_tag_paths) * n_sub_tags);
  for (size_t i = 0; i < n_sub_tags; i++) {
    sub_tag_paths[i] = malloc(sizeof(*sub_tag_paths[i]) * str_max);
    strncpy(sub_tag_paths[i], "/", str_max);
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
    /* p->length--; */
    ps->children[0]->name = strdup(p->components[p->length - 1]);
    ps->children[0]->path = strdup(p->components[p->length - 1]);
    /* free(p->components[p->length]); */

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

static key *key_generate(const keytype type)
{
  key key_init = {
    .auto_index = 0,
    .template = NULL,
    .value = NULL,
    .type = type
  };

  key *k = malloc(sizeof(*k));
  memcpy(k, &key_init, sizeof(*k));
  return k;
}

static void key_destroy(key *k)
{
  if (k->template != NULL) {
    free(k->template);
  }

  if (k->value != NULL) {
    free(k->value);
  }

  free(k);
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

  int key_type = IDX_NORMAL;
  char *key_path = nodes[0]->path->components[nodes[0]->path->length - 1];
  if (strcmp(key_path, "auto_index") == 0) {
    key_type = IDX_AUTO;
    path_drop_last_component(nodes[0]->path);
  } else if (strcmp(key_path, "condensed") == 0) {
    key_type = IDX_CONDENSE;
    path_drop_last_component(nodes[0]->path);
  }

  key *ns_key = key_generate(key_type);

  char *root = strdup(ps->children[0]->path);
  while (root[0] == '/') root++;
  node_set ns_init = {
    .root = root,
    .key_idx = 0, // Always 0 since get_names moves it to 0 if it's not.
    .nodes = nodes,
    .n_nodes = ps->n_children - 1,
    .max_path_depth = max_p_depth,
    .key = ns_key
  };

  node_set *ns = malloc(sizeof(*ns));
  memcpy(ns, &ns_init, sizeof(*ns));

  return ns;
}

void node_set_destroy(node_set *ns)
{
  for (size_t i = 0; i < ns->n_nodes; i++) {
    node_destroy(ns->nodes[i]);
  }
  key_destroy(ns->key);

  free(ns->nodes);
  free((char *)ns->root);
  free(ns);
}

static value value_clone(const value v)
{
  char *att = NULL;
  char *desired_att = NULL;
  if (v->attribute_name != NULL) {
    att = strdup(v->attribute_name);
  }
  if (v->required_attribute_value) {
    desired_att = strdup(v->required_attribute_value);
  }

  struct Value dup_v_init = {
    .attribute_name = att,
    .required_attribute_value = desired_att,
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

  snprintf(name, str_max, "%s_%d", n->name, thread);
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

key *key_clone(const key *k)
{
  key *dup_k = malloc(sizeof(*dup_k));
  memcpy(dup_k, k, sizeof(*dup_k));
  dup_k->template = k->template == NULL ? NULL : strdup(k->template);
  dup_k->value = k->value == NULL ? NULL : strdup(k->value);

  return dup_k;
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
  dup_ns->key = key_clone(ns->key);

  return dup_ns;
}

static void collect_value(gzFile fptr, const z_off_t pos[2], char **value)
{
  char c;
  if ((pos[0] == -1) || (pos[1] == 0)) {
    *value = malloc(sizeof(*value));
    (*value)[0] = '\0';
    return;
  }

  *value = malloc(sizeof(*value) * (pos[1] + 1));
  const z_off_t start = gztell(fptr);

  gzseek(fptr, pos[0], SEEK_SET);
  for (z_off_t i = 0; i < pos[1]; i++) {
    c = gzgetc(fptr);
    (*value)[i] = c;
  }
  (*value)[pos[1]] = '\0';

  gzseek(fptr, start, SEEK_SET);
}

static void collect_auto_index(node_set *ns, char **key)
{
  size_t idx_size = 5; // Assume 4 digits (+1 '\0') is plenty for index.
  *key = malloc(sizeof(**key) * idx_size);
  snprintf(*key, idx_size, "%zu", ns->key->auto_index);
}

static void collect_index(gzFile fptr, node_set *ns, const size_t str_max)
{
  char *ns_key;
  if (ns->key->type == IDX_AUTO) {
    collect_auto_index(ns, &ns_key);
  } else {
    collect_value(fptr, ns->nodes[ns->key_idx]->value->pos, &ns_key);
  }

  if (ns->key->value != NULL) {
    free(ns->key->value);
  }

  if (ns->key->template != NULL) {
    ns->key->value = malloc(sizeof(*ns->key->value) * str_max);
    snprintf(ns->key->value, str_max, ns->key->template, ns_key);
  } else {
    ns->key->value = strdup(ns_key);
  }

  free(ns_key);
}

static void node_fprintf(FILE *out, gzFile fptr, node *n)
{
  char *value;
  char *att;

  collect_value(fptr, n->value->pos, &value);
  n->value->pos[0] = -1;
  n->value->pos[1] = 0;
  fprintf(out, "%s\t", value);
  free(value);
  if ((n->value->attribute_name != NULL) &&
      (n->value->required_attribute_value == NULL)) {
    collect_value(fptr, n->value->att_pos, &att);
    n->value->att_pos[0] = -1;
    n->value->att_pos[1] = 0;
    fprintf(out, "%s\t", att);
    free(att);
  }
}

void node_set_fprintf_node(FILE *out, gzFile fptr, node_set *ns,
                           const size_t node_i, const size_t str_max)
{
  if (ns->key->type == IDX_CONDENSE) {
    return;
  }

  /* WARNING: Assumes the index will be the first part of the element to be
  found. For example PMID should be the first tag under the current article,
  otherwise, the value found will be associated with the previous article.

  To avoid, could hold onto a buffer of found values for each search path then
  print all at once when the end tag for the current article is found. Downside
  would be there are some elements (such as abstract) which have a large range
  of possible lengths and list type elements (such as authors) that can have
  any number elements. This would require adding realloc logic. */
  if (node_i == ns->key_idx) {
    collect_index(fptr, ns, str_max);
    ns->key->auto_index++;
    return;
  }

  fprintf(out, "%s\t", ns->key->value);
  node_fprintf(out, fptr, ns->nodes[node_i]);
  fprintf(out, "\n");
}

void node_set_fprintf_condensed_node(FILE *out, gzFile fptr, node_set *ns,
                                     const size_t str_max)
{
  if (!(ns->key->type == IDX_CONDENSE)) {
    return;
  }

  collect_index(fptr, ns, str_max);
  fprintf(out, "%s\t", ns->key->value);
  // WARNING: Assumes key_value is always 0. This is true now but could change.
  for (size_t i = 1; i < ns->n_nodes; i++) {
    node_fprintf(out, fptr, ns->nodes[i]);
  }
  fprintf(out, "\n");
}

void node_set_reset_index(node_set *ns)
{
  ns->key->auto_index = 0;
}

void node_set_copy_parents_index(node_set *child, node_set *parent,
                                 const size_t str_max)
{
  if (child->key->template != NULL) {
    free(child->key->template);
  }

  child->key->template = malloc(sizeof(*child->key->template) * str_max);
  strncpy(child->key->template, parent->key->value, str_max - 1);

  /* HACK: "%s" will be replaced by the key value, if there is not a specific
  key (in the case of a child node set, it's possible the child inherits a key
  from the parent but does not have it's own key.), there is no value so this
  will be replaced with '\0'. */
  if (child->nodes[child->key_idx]->path->length == 0) {
    strncat(child->key->template, "%s", str_max - 1);
  } else {
    strncat(child->key->template, "\t%s", str_max - 1);
  }
}

bool path_attribute_matches_required(gzFile fptr, const value v)
{
  char *path_attribute;
  collect_value(fptr, v->att_pos, &path_attribute);

  return strcmp(path_attribute, v->required_attribute_value) == 0;
}