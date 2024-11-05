#include "nodes.h"

#include "error.h"
#include "paths.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define KEYNODE(ns) (ns->nodes[ns->key_idx])

enum { ATT_NONE = 1, ATT_FOUND, ATT_EXPECTED };

static node* node_generate(path_struct const ps, char const* cache_dir,
  int const overwrite, size_t const str_max);

static FILE* get_file(
  char const* name, char const* cache_dir, int const overwrite)
{
  FILE* fptr;
  size_t str_max = 8000;
  char out[str_max + 1];
  char* mode = overwrite ? "w" : "a";

  strncpy(out, cache_dir, str_max);
  strncat(out, name, str_max - strlen(out));
  strncat(out, ".tsv", str_max - strlen(out));

  fptr = fopen(out, mode);
  return fptr;
}

static size_t find_sub_tag_names(
  char const* p, size_t str_max, char*** sub_tags_holder)
{
  while ((*p != '\0') && (*p != '{')) {
    p++;
  }

  if (*p == '\0') {
    sub_tags_holder[0] = NULL;
    return 0;
  }

  p++; /* Skip { */
  size_t count = 1;
  size_t i = 0;
  while ((p[i] != '}') && (p[i] != '\0')) {
    if (p[i] == ',') {
      count++;
    }
    i++;
  }

  if (p[i] == '\0') {
    pubmedparser_error(0, "Could not find subtag; malformed path.");
  }

  sub_tags_holder[0] = malloc(sizeof(char*) * count);
  if (!sub_tags_holder[0]) {
    pubmedparser_error(PP_ERR_OOM, "");
  }

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

static void find_attribute_name(char const* p, char** attribute,
  char** expected_attribute, size_t const str_max)
{
  while ((*p != '\0') && (*p != '@') && (*p != '[')) {
    p++;
  }

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
  if (!(*attribute)) {
    pubmedparser_error(PP_ERR_OOM, "");
  }

  p++; /* Skip @ */
  size_t i;
  for (i = 0; (p[i] != '\0') && (p[i] != '=') && (i < (str_max - 1)); i++) {
    (*attribute)[i] = p[i];
  }
  (*attribute)[i] = '\0';

  if (att_type == ATT_EXPECTED) {
    *expected_attribute = malloc(sizeof(**expected_attribute) * str_max);
    if (!(*expected_attribute)) {
      pubmedparser_error(PP_ERR_OOM, "");
    }

    p += i;
    while ((*p == '=') || (*p == '\'') || (*p == ' ')) {
      p++;
    }

    for (i = 0; (p[i] != '\'') && (p[i] != ']') && (i < (str_max - 1)); i++) {
      (*expected_attribute)[i] = p[i];
    }
    (*expected_attribute)[i] = '\0';
  }
}

static attribute attribute_init(char const* xml_path, size_t const str_max)
{
  char* attribute_name;
  char* expected_attribute;
  find_attribute_name(xml_path, &attribute_name, &expected_attribute, str_max);

  struct Container att_init = {
    .buff = malloc(sizeof(*att_init.buff) * (str_max + 1)),
    .buffsize = str_max,
    .name = attribute_name,
    .required_value = expected_attribute
  };

  attribute att = malloc(sizeof(*att));
  if (!att) {
    pubmedparser_error(PP_ERR_OOM, "");
  }

  memcpy(att, &att_init, sizeof(*att));
  att->buff[0] = '\0';
  return att;
}

static void container_destroy(container c)
{
  if (c->name) {
    free((char*)c->name);
  }
  if (c->required_value) {
    free((char*)c->required_value);
  }
  free(c->buff);
  free(c);
}

static value value_init(size_t const str_max)
{
  struct Container val_init = {
    .buff = malloc(sizeof(*val_init.buff) * (str_max + 1)),
    .buffsize = str_max,
    .name = NULL,
    .required_value = NULL
  };

  value val = malloc(sizeof(*val));
  if (!val) {
    pubmedparser_error(PP_ERR_OOM, "");
  }

  memcpy(val, &val_init, sizeof(*val));
  val->buff[0] = '\0';
  return val;
}

static node_set* node_set_generate_from_sub_tags(char const* xml_path,
  char const* name, char** sub_tags, size_t const n_sub_tags,
  char const* cache_dir, int const overwrite, size_t const str_max)
{
  path p = path_init(xml_path, str_max);

  struct PathStructure ps = {
    .name = strdup(name),
    .path = NULL,
    .parent = NULL,
    .children = malloc(sizeof(path_struct) * (n_sub_tags + 2)),
    .n_children = n_sub_tags + 2,
  };

  for (size_t i = 0; i < ps.n_children; i++) {
    ps.children[i] = malloc(sizeof(struct PathStructure));
    if (!ps.children[i]) {
      pubmedparser_error(PP_ERR_OOM, "");
    }
  }

  ps.children[0]->name = strdup("root");
  ps.children[0]->path = p->components[p->length - 1];
  ps.children[0]->parent = &ps;
  ps.children[0]->children = NULL;
  ps.children[0]->n_children = 0;

  ps.children[1]->name = strdup("key");
  ps.children[1]->path = "/condensed";
  ps.children[1]->parent = &ps;
  ps.children[1]->children = NULL;
  ps.children[1]->n_children = 0;

  char** sub_tag_paths = malloc(sizeof(*sub_tag_paths) * n_sub_tags);
  if (!sub_tag_paths) {
    pubmedparser_error(PP_ERR_OOM, "");
  }

  for (size_t i = 0; i < n_sub_tags; i++) {
    sub_tag_paths[i] = malloc(sizeof(*sub_tag_paths[i]) * str_max);
    if (!sub_tag_paths[i]) {
      pubmedparser_error(PP_ERR_OOM, "");
    }

    strncpy(sub_tag_paths[i], "/", str_max);
    strncat(sub_tag_paths[i], sub_tags[i], str_max);

    ps.children[i + 2]->name = strdup(sub_tags[i]);
    ps.children[i + 2]->path = sub_tag_paths[i];
    ps.children[i + 2]->parent = &ps;
    ps.children[i + 2]->children = NULL;
    ps.children[i + 2]->n_children = 0;
  }

  node_set* ns =
    node_set_generate(&ps, ps.name, cache_dir, overwrite, str_max);

  path_destroy(p);
  for (size_t i = 0; i < n_sub_tags; i++) {
    free(sub_tag_paths[i]);
  }
  free(sub_tag_paths);

  free(ps.name);
  for (size_t i = 0; i < ps.n_children; i++) {
    free(ps.children[i]->name);
    free(ps.children[i]);
  }
  free(ps.children);

  return ns;
};

static node* node_generate(path_struct const ps, char const* cache_dir,
  int const overwrite, size_t const str_max)
{
  char** sub_tags;
  node_set* ns = NULL;
  value v = NULL;
  attribute a = NULL;
  path p;

  if (ps->n_children > 0) {
    p = path_init(ps->children[0]->path, str_max);
    /* By dropping the last component of the path then calling that the root we
    can use the same pattern as in the top level where we search for the first
    instance of root then loop until hitting the root end tag. This allows us
    to use recursion in the main parser. */
    free(ps->children[0]->name);
    free(ps->children[0]->path);
    ps->children[0]->name = strdup(p->components[p->length - 1]);
    ps->children[0]->path = strdup(p->components[p->length - 1]);

    ns = node_set_generate(ps, ps->name, cache_dir, overwrite, str_max);
  } else {
    size_t n_sub_tags = find_sub_tag_names(ps->path, str_max, &sub_tags);
    p = path_init(ps->path, str_max);
    if (n_sub_tags > 0) {
      ns = node_set_generate_from_sub_tags(ps->path, ps->name, sub_tags,
        n_sub_tags, cache_dir, overwrite, str_max);

      for (size_t i = 0; i < n_sub_tags; i++) {
        free(sub_tags[i]);
      }
      free(sub_tags);
    } else {
      v = value_init(str_max);
      a = attribute_init(ps->path, str_max);
    }
  }

  node n_init = {
    .name = strdup(ps->name),
    .path = p,
    .value = v,
    .attribute = a,
    .child_ns = ns,
    .out = get_file(ps->name, cache_dir, overwrite),
  };

  node* n = malloc(sizeof *n);
  if (!n) {
    pubmedparser_error(PP_ERR_OOM, "");
  }

  memcpy(n, &n_init, sizeof *n);

  return n;
}

static void node_destroy(node* n)
{
  free((char*)n->name);
  path_destroy((path)n->path);
  if (n->value) {
    container_destroy(n->value);
  }

  if (n->attribute) {
    container_destroy(n->attribute);
  }

  if (n->child_ns) {
    node_set_destroy(n->child_ns);
  }

  if (n->out) {
    fprintf(n->out, "%c\n", PP_EOF);
    fclose(n->out);
  }
  free(n);
}

static key* key_generate(keytype const type)
{
  key key_init = {
    .auto_index = 0, .template = NULL, .value = NULL, .type = type
  };

  key* k = malloc(sizeof(*k));
  if (!k) {
    pubmedparser_error(PP_ERR_OOM, "");
  }

  memcpy(k, &key_init, sizeof(*k));
  return k;
}

static void key_destroy(key* k)
{
  if (k->template != NULL) {
    free(k->template);
  }

  if (k->value != NULL) {
    free(k->value);
  }

  free(k);
}

node_set* node_set_generate(path_struct const ps, char const* name_prefix,
  char const* cache_dir, int const overwrite, size_t const str_max)
{
  if (name_prefix) {
    char* name_prefix_i =
      malloc(sizeof(*name_prefix_i) * (strlen(name_prefix) + 2));
    strcpy(name_prefix_i, name_prefix);
    strcat(name_prefix_i, "_");

    for (size_t i = 1; i < ps->n_children; i++) {
      char* old_name = ps->children[i]->name;
      size_t name_sz = strlen(name_prefix_i) + strlen(old_name) + 1;
      char* new_name = malloc(sizeof(*new_name) * name_sz);

      strcpy(new_name, name_prefix_i);
      strcat(new_name, old_name);
      ps->children[i]->name = new_name;
      free(old_name);
    }
    free(name_prefix_i);
  }

  node** nodes = malloc(sizeof(*nodes) * (ps->n_children - 1));
  if (!nodes) {
    pubmedparser_error(PP_ERR_OOM, "");
  }

  for (size_t i = 0; i < (ps->n_children - 1); i++) {
    nodes[i] =
      node_generate(ps->children[i + 1], cache_dir, overwrite, str_max);
  }

  size_t max_p_depth = 0;
  for (size_t i = 0; i < (ps->n_children - 1); i++) {
    max_p_depth =
      (max_p_depth > nodes[i]->path->length) ?
        max_p_depth :
        nodes[i]->path->length;
  }

  int key_type = IDX_NORMAL;
  char* key_path = nodes[0]->path->components[nodes[0]->path->length - 1];
  if (strcmp(key_path, "auto_index") == 0) {
    key_type = IDX_AUTO;
    path_drop_last_component(nodes[0]->path);
  } else if (strcmp(key_path, "condensed") == 0) {
    key_type = IDX_CONDENSE;
    path_drop_last_component(nodes[0]->path);
  }

  key* ns_key = key_generate(key_type);

  char* root = ps->children[0]->path;
  while (*root == '/') {
    root++;
  }

  node_set ns_init = {
    .root = strdup(root),
    .key_idx = 0, // Always 0 since get_names moves it to 0 if it's not.
    .nodes = nodes,
    .n_nodes = ps->n_children - 1,
    .max_path_depth = max_p_depth,
    .key = ns_key,
  };

  node_set* ns = malloc(sizeof(*ns));
  if (!ns) {
    pubmedparser_error(PP_ERR_OOM, "");
  }

  memcpy(ns, &ns_init, sizeof(*ns));

  return ns;
}

void node_set_destroy(node_set* ns)
{
  for (size_t i = 0; i < ns->n_nodes; i++) {
    node_destroy(ns->nodes[i]);
  }
  key_destroy(ns->key);

  free(ns->nodes);
  free((char*)ns->root);
  free(ns);
}

static int is_file_empty(FILE* fptr)
{
  int p;
  if ((p = ftell(fptr)) > 0) {
    return false;
  }

  fseek(fptr, 0L, SEEK_END);
  if (ftell(fptr) == 0) {
    return true;
  }

  fseek(fptr, p, SEEK_SET);

  return false;
}

static inline char* write_header_get_top_index_name(node_set const* ns)
{
  path key_path = KEYNODE(ns)->path;

  return key_path->components[key_path->length - 1];
}

static void write_header_condensed_ns_i(node_set const* ns, node* parent,
  char const* idx_header, size_t const str_max)
{
  if (!(is_file_empty(parent->out))) {
    return;
  }

  node* n;
  char header[str_max + 1];
  strncpy(header, idx_header, str_max);
  strncat(header, "\t", str_max);
  // Skip key since it doesn't hold a real value in condensed case.
  for (size_t i = 1; i < ns->n_nodes; i++) {
    n = ns->nodes[i];
    if ((n->attribute->name) && !(n->attribute->required_value)) {
      strncat(header, "\t", str_max);
      strncat(header, n->attribute->name, str_max);
    }
    strncat(header, n->path->components[n->path->length - 1], str_max);
    strncat(header, i == (ns->n_nodes - 1) ? "\n" : "\t", str_max);
  }
  fprintf(parent->out, "%s", header);

  return;
}

static void write_header_node_i(node const* n, char const* idx_header,
  char const* name_prefix, size_t const str_max)
{
  if (!(is_file_empty(n->out))) {
    return;
  }

  char header[str_max + 1];
  strncpy(header, idx_header, str_max);
  strncat(header, "\t", str_max);

  if (name_prefix) {
    strncat(header, name_prefix, str_max);
    strncat(header, "Index\t", str_max);
  }

  if ((n->attribute->name) && !(n->attribute->required_value)) {
    strncat(header, n->attribute->name, str_max);
    strncat(header, "\t", str_max);
  }

  strncat(header, n->path->components[n->path->length - 1], str_max);
  strncat(header, "\n", str_max);

  fprintf(n->out, "%s", header);
}

static void node_set_write_headers_i(node_set const* ns, node* parent,
  char const* idx_header, char const* name_prefix, size_t const str_max)
{
  if (ns->key->type == IDX_CONDENSE) {
    write_header_condensed_ns_i(ns, parent, idx_header, str_max);
    fclose(parent->out);
    parent->out = NULL;
    return;
  }

  for (size_t i = 1; i < ns->n_nodes; i++) {
    if (ns->nodes[i]->child_ns) {
      node_set_write_headers_i(ns->nodes[i]->child_ns, ns->nodes[i],
        idx_header, ns->nodes[i]->name, str_max);
    } else {
      write_header_node_i(ns->nodes[i], idx_header, name_prefix, str_max);
      // Results written to cloned nodeset's files not original.
      fclose(ns->nodes[i]->out);
      ns->nodes[i]->out = NULL;
    }
  }

  return;
}

/* Write headers if the files don't already exist.
   If the files have already been written to, assume we are appending to them
   and that the path structure is the same as they were when the files were
   first created. */
void node_set_write_headers(node_set const* ns, size_t const str_max)
{
  char* idx = write_header_get_top_index_name(ns);
  node_set_write_headers_i(ns, NULL, idx, NULL, str_max);
}

static attribute container_clone(container const c)
{
  char* attribute_name = NULL;
  char* expected_attribute = NULL;
  if (c->name) {
    attribute_name = strdup(c->name);
  }
  if (c->required_value) {
    expected_attribute = strdup(c->required_value);
  }

  struct Container dup_container_init = { .name = attribute_name,
    .required_value = expected_attribute,
    .buffsize = c->buffsize,
    .buff = malloc(sizeof(*c->buff) * c->buffsize) };

  attribute dup_container = malloc(sizeof(*dup_container));
  if (!dup_container) {
    pubmedparser_error(PP_ERR_OOM, "");
  }

  memcpy(dup_container, &dup_container_init, sizeof(*dup_container));
  dup_container->buff[0] = '\0';

  return dup_container;
}

static path path_clone(path const p)
{
  path dup_p = malloc(sizeof(*dup_p));
  memcpy(dup_p, p, sizeof(*dup_p));
  dup_p->components = malloc(sizeof(*dup_p->components) * dup_p->length);
  for (size_t i = 0; i < p->length; i++) {
    dup_p->components[i] = strdup(p->components[i]);
  }

  return dup_p;
}

static node* node_clone(
  node const* n, char const* cache_dir, int const thread, size_t const str_max)
{
  value val = NULL;
  attribute att = NULL;
  node_set* child_ns = NULL;
  char name[str_max];

  snprintf(name, str_max, "%s_%d", n->name, thread);
  if (n->value) {
    val = container_clone(n->value);
  }

  if (n->attribute) {
    att = container_clone(n->attribute);
  }

  if (n->child_ns) {
    child_ns = node_set_clone(n->child_ns, cache_dir, thread, str_max);
  }

  node dup_n_init = {
    .name = strdup(name),
    .path = path_clone(n->path),
    .value = val,
    .attribute = att,
    .child_ns = child_ns,
    .out = get_file(name, cache_dir,
      CACHE_OVERWRITE), // Clone files are ephemeral so always overwrite.
  };

  node* dup_n = malloc(sizeof(*dup_n));
  if (!dup_n) {
    pubmedparser_error(PP_ERR_OOM, "");
  }

  memcpy(dup_n, &dup_n_init, sizeof(*dup_n));

  return dup_n;
}

key* key_clone(key const* k)
{
  key* dup_k = malloc(sizeof(*dup_k));
  if (!dup_k) {
    pubmedparser_error(PP_ERR_OOM, "");
  }

  memcpy(dup_k, k, sizeof(*dup_k));
  dup_k->template = k->template == NULL ? NULL : strdup(k->template);
  dup_k->value = k->value == NULL ? NULL : strdup(k->value);

  return dup_k;
}

node_set* node_set_clone(node_set const* ns, char const* cache_dir,
  size_t const thread, size_t const str_max)
{
  node_set* dup_ns = malloc(sizeof(*dup_ns));
  if (!dup_ns) {
    pubmedparser_error(PP_ERR_OOM, "");
  }

  memcpy(dup_ns, ns, sizeof(*ns));
  dup_ns->root = strdup(ns->root);

  dup_ns->nodes = malloc(sizeof(*ns->nodes) * ns->n_nodes);
  for (size_t i = 0; i < dup_ns->n_nodes; i++) {
    dup_ns->nodes[i] = node_clone(ns->nodes[i], cache_dir, thread, str_max);
  }
  dup_ns->key = key_clone(ns->key);

  return dup_ns;
}

static void collect_auto_index(node_set* ns, char** key)
{
  size_t idx_size = 6; // Assume 4 digits (+2 '\t'/'\0') is plenty for index.
  *key = malloc(sizeof(**key) * idx_size);
  if (!key) {
    pubmedparser_error(PP_ERR_OOM, "");
  }

  snprintf(*key, idx_size - 1, "\t%zu", ns->key->auto_index);
}

static void collect_index(node_set* ns, size_t const str_max)
{
  char* ns_key;
  if (ns->key->type == IDX_AUTO) {
    collect_auto_index(ns, &ns_key);
  } else {
    ns_key = strdup(ns->nodes[ns->key_idx]->value->buff);
  }

  if (ns->key->value) {
    free(ns->key->value);
  }

  if (ns->key->template) {
    ns->key->value = malloc(sizeof(*ns->key->value) * str_max);
    if (!ns->key->value) {
      pubmedparser_error(PP_ERR_OOM, "");
    }

    snprintf(ns->key->value, str_max, ns->key->template, ns_key);
  } else {
    ns->key->value = strdup(ns_key);
  }

  free(ns_key);
}

static void node_fprintf(FILE* stream, node* n)
{
  if ((n->attribute->name) && !(n->attribute->required_value)) {
    fprintf(stream, "\t%s", n->attribute->buff);
    n->attribute->buff[0] = '\0';
  }
  fprintf(stream, "\t%s", n->value->buff);
  n->value->buff[0] = '\0';
}

void node_set_fprintf_node(
  FILE* stream, node_set* ns, size_t const node_i, size_t const str_max)
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
    collect_index(ns, str_max);
    ns->key->auto_index++;
    return;
  }

  fprintf(stream, "%s", ns->key->value);
  node_fprintf(stream, ns->nodes[node_i]);
  fprintf(stream, "\n");
}

void node_set_fprintf_condensed_node(
  FILE* stream, node_set* ns, size_t const str_max)
{
  if (!(ns->key->type == IDX_CONDENSE)) {
    return;
  }

  collect_index(ns, str_max);
  fprintf(stream, "%s", ns->key->value);
  // WARNING: Assumes key_value is always 0. This is true now but could change.
  for (size_t i = 1; i < ns->n_nodes; i++) {
    node_fprintf(stream, ns->nodes[i]);
  }
  fprintf(stream, "\n");
}

void node_set_reset_index(node_set* ns) { ns->key->auto_index = 0; }

void node_set_copy_parents_index(
  node_set* child, node_set* parent, size_t const str_max)
{
  if (child->key->template) {
    free(child->key->template);
  }

  child->key->template = malloc(sizeof(*child->key->template) * str_max);
  strncpy(child->key->template, parent->key->value, str_max - 1);
  strncat(child->key->template, "%s", str_max - 1);
}

bool path_attribute_matches_required(node const* n)
{
  return strcmp(n->attribute->buff, n->attribute->required_value) == 0;
}

static void node_mark_i(node* n)
{
  fgetpos(n->out, &(n->eof));
  if (n->child_ns) {
    node_set_mark(n->child_ns);
  }
}

void node_set_mark(node_set* ns)
{
  for (size_t i = 0; i < ns->n_nodes; i++) {
    node_mark_i(ns->nodes[i]);
  }
}

static void node_rewind_i(node* n)
{
  fsetpos(n->out, &(n->eof));
  if (n->child_ns) {
    node_set_rewind(n->child_ns);
  }
}

void node_set_rewind(node_set* ns)
{
  for (size_t i = 0; i < ns->n_nodes; i++) {
    node_rewind_i(ns->nodes[i]);
  }
}
