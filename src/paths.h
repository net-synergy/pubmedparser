#ifndef PATHS_H
#define PATHS_H

#include <stdio.h>

typedef struct Path {
  char **components;
  int length;
} path;

typedef struct Node {
  char *name;
  path *path;
  char **values;
  int n_values;
  char **sub_tags;
  int n_sub_tags;
  char *attribute;
  FILE *out;
} node;

typedef struct NodeSet {
  char *root;
  int max_path_depth;
  int key_idx;
  node **nodes;
  int n;
} node_set;

node_set *construct_node_set(char *root, char **xpaths, int n_nodes,
                             char **names, int key_idx, int str_max, char *cache_dir);

#endif
