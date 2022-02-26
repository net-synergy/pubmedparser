#include "paths.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int n_components(char *p)
{
  int n = 0;
  while (*p != '\0') {
    if (*p == '/') n++;
    p++;
  }
  return n;
}

static void get_components(char *p, char **components, int str_max)
{
  int tag_i = 0;
  int comp_i = 0;
  char name[str_max];
  if ((p[0] != '.') | (p[1] != '/')) {
    printf("Path malformed. Most start with './'");
    exit(2);
  }
  p += 2; // Strip initial './';
  while (*p != '\0') {
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
  name[tag_i] = '\0';
  components[comp_i] = strdup(name);
}

static FILE *get_file(char *name)
{
  FILE *fptr = malloc(sizeof(FILE));
  char out[100];
  strcpy(out, "../cache/");
  strcat(out, name);
  strcat(out, ".tsv");

  fptr = fopen(out, "a");
  return fptr;
}

path *construct_path(char *xml_path, int str_max)
{
  int length = n_components(xml_path);
  char **components = malloc(sizeof(char *) * length);
  get_components(xml_path, components, str_max);

  path *p = malloc(sizeof(path));
  p->length = length;
  p->components = components;
  return p;
}

node *construct_node(char *xml_path, int str_max, node *key)
{
  path *p = construct_path(xml_path, str_max);

  char *value = malloc(sizeof(char) * str_max);
  value[0] = '\0';

  node *n = malloc(sizeof(node));
  n->out = get_file(p->components[p->length - 1]); /* Change to argument label. */
  n->name = p->components[p->length - 1];
  n->value = value;
  n->path = p;
  n->key = key;
  return n;
}
