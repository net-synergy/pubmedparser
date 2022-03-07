#ifndef PATHS_H
#define PATHS_H

#include <stdio.h>

typedef struct Path {
  char **components;
  int length;
} path;

typedef struct Node {
  const char *name;
  const path *path;
  char **values;
  const int n_values;
  const char **sub_tags;
  const int n_sub_tags;
  const char *attribute;
  const char *expected_attribute;
  FILE *out;
} node;

typedef struct NodeSet {
  const char *root;
  const int max_path_depth;
  const int key_idx;
  node **nodes;
  const int n;
} node_set;

node_set *construct_node_set(char *structure_file, char *cache_dir, int str_max);
void release_node_set(node_set *ns);

#endif
