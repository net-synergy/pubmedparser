#ifndef NODES_H
#define NODES_H

#include "paths.h"
#include "structure.h"

#include <stdio.h>
#include <zlib.h>

/* Where to stop when concatenating files in case a file failed to parse but
   leftover data remains in the out file. */
#define PP_EOF '\0'

typedef enum KeyTypes { IDX_NORMAL = 0, IDX_AUTO, IDX_CONDENSE } keytype;

typedef struct Key {
  keytype const type;
  char* value;
  char* template;
  size_t auto_index;
} key;

typedef struct Node {
  char const* name;
  path const path;
  struct Container* value;
  struct Container* attribute;
  struct NodeSet* child_ns;
  FILE* out;
  fpos_t eof;
} node;

typedef struct NodeSet {
  char const* root;
  size_t const key_idx;
  node** nodes;
  size_t n_nodes;
  size_t const max_path_depth;
  key* key;
} node_set;

void node_set_fprintf_node(
  FILE* stream, node_set*, size_t const node_i, size_t const str_max);
void node_set_fprintf_condensed_node(
  FILE* stream, node_set*, size_t const str_max);

node* node_root(node_set*);
bool path_attribute_matches_required(node const*);
void node_set_reset_index(node_set*);
void node_set_copy_parents_index(
  node_set* child, node_set* parent, size_t const str_max);

node_set* node_set_generate(path_struct const structure,
  char const* name_prefix, char const* cache_dir, int const overwrite,
  size_t const str_max);

void node_set_write_headers(node_set const* ns, size_t const str_max);

node_set* node_set_clone(node_set const* ns, char const* cache_dir,
  size_t const thread, size_t const str_max);
void node_set_destroy(node_set* ns);

void node_set_mark(node_set* ns);
void node_set_rewind(node_set* ns);

#endif
