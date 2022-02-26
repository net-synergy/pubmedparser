#ifndef PATHS_H
#define PATHS_H

#include <stdio.h>

typedef struct Path {
  char **components;
  int length;
} path;

typedef struct Node {
  char *name;
  char *value;
  path *path;
  FILE *out;
  struct Node *key;
} node;

path* construct_path(char *xml_path, int lim);
node* construct_node(char *xml_path, int lim, node *key);

#endif
