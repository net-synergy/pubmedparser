#ifndef NODES_H
#define NODES_H

#include "paths.h"
#include "structure.h"
#include "zlib.h"

typedef enum KeyTypes {
  IDX_NORMAL = 0,
  IDX_AUTO,
  IDX_CONDENSE
} keytype;

typedef struct Value {
  z_off_t pos[2]; // { start, offset }
  z_off_t att_pos[2];
  const char *attribute_name;
  const char *required_attribute_value;
} *value;

typedef struct Key {
  const keytype type;
  char *value;
  char *template;
  size_t auto_index;
} key;

typedef struct Node {
  const char *name;
  const path path;
  struct Value *value;
  struct NodeSet *child_ns;
  FILE *out;
} node;

typedef struct NodeSet {
  const char *root;
  const size_t key_idx;
  node **nodes;
  size_t n_nodes;
  const size_t max_path_depth;
  key *key;
} node_set;

void node_set_fprintf_node(FILE *out, gzFile in, node_set *,
                           const size_t node_i, const size_t str_max);
void node_set_fprintf_condensed_node(FILE *out, gzFile in, node_set *,
                                     const size_t str_max);

node *node_root(node_set *);
bool path_attribute_matches_required(gzFile, const value);
void node_set_reset_index(node_set *);
void node_set_copy_parents_index(node_set *child, node_set *parent,
                                 const size_t str_max);

node_set *node_set_generate(const path_struct structure,
                            const char *name_prefix,
                            const char *cache_dir,
                            const size_t str_max);

void node_set_destroy(node_set *ns);

node_set *node_set_clone(const node_set *ns, const char *cache_dir,
                         const size_t thread, const size_t str_max);

#endif
