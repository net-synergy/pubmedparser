#ifndef NODES_H
#define NODES_H

#include "paths.h"
#include "structure.h"

typedef struct Value {
  int pos[2]; // { start, offset }
  int att_pos[2];
  const char *attribute;
  const char *desired_attribute;
} *value;

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
} node_set;

void node_printf_key(FILE *, node_set *);
void node_printf_value(FILE *, node *);

node *node_root(node_set *);
int node_is_root(node_set *);

node_set *node_set_generate(const path_struct structure,
                            const char *name_prefix,
                            const char *cache_dir,
                            const size_t str_max);

void node_set_destroy(node_set *ns);

node_set *node_set_clone(const node_set *ns, const char *cache_dir,
                         const size_t thread, const size_t str_max);

#endif
