#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "query.h"
#include "paths.h"

#define IS_CLOSE(tag) (tag[0] == '/')

#define ADD_TAG(path, tag) {				\
    if (path.length < max_p_depth) {			\
      path.components[path.length] = strdup(tag);	\
    }							\
    path.length++;					\
  }

#define RM_TAG(path) {				\
    path.length--;				\
    if (path.length < max_p_depth) {		\
      free(path.components[path.length]);	\
    }						\
  }

#define PRINT_NODE(node) {			\
    if (node->key != NULL) {			\
      fprintf(node->out,			\
	      "%s\t%s\n",			\
	      node->key->value,			\
	      node->value);			\
    }						\
  }

int path_match(path *p1, path *p2)
{
  if (p1->length != p2->length) return 0;

  int i = p1->length;
  while ((i > 0) &&
         (strcmp(p1->components[i - 1], p2->components[i - 1]) == 0)) i--;

  return i == 0;
}

int main()
{
  char *input = "../data/pubmed21n0001.xml";
  FILE *fptr;
  if (!(fptr = fopen(input, "r"))) {
    printf("Couldn't open file");
    exit(1);
  }

  int str_max = 100;
  char *root = "PubmedArticleSet";
  char *kp = "./PubmedArticleSet/PubmedArticle/MedlineCitation/PMID";
  node *key = construct_node(kp, str_max, NULL);

  char *xpaths[] = {
    "./PubmedArticleSet/PubmedArticle/MedlineCitation/Article/Journal/ISSN",
    "./PubmedArticleSet/PubmedArticle/MedlineCitation/DateRevised/Year"
  };

  int n_nodes = (sizeof xpaths / sizeof * xpaths) + 1;
  node *nodes[n_nodes];
  nodes[0] = key;

  for (int i = 0; i < (n_nodes - 1); i++)
    nodes[i + 1] = construct_node(xpaths[i], str_max, key);

  int max_p_depth = 0;
  for (int i = 0; i < n_nodes; i++)
    max_p_depth = (max_p_depth > nodes[i]->path->length) ? max_p_depth :
                  nodes[i]->path->length;

  path current = {
    .length = 0,
    .components = malloc(sizeof(char *) * max_p_depth)
  };

  int c = 0;
  char tag[str_max];
  /* char value[str_max]; */

  while (c != EOF) {
    c = get_tag(fptr, c, tag, str_max);
    if (current.length > 0) {
      if (IS_CLOSE(tag)) {
        RM_TAG(current);
      } else {
        ADD_TAG(current, tag);
        for (int i = 0; i < n_nodes; i++) {
          if (path_match(&current, nodes[i]->path)) {
            c = get_value(fptr, c, nodes[i]->value, str_max);
            PRINT_NODE(nodes[i]);
          }
        }
      }
    } else {
      if (strcmp(root, tag) == 0) {
        ADD_TAG(current, tag);
      }
    }
  }
  if (current.length == 0) {
    return 0;
  } else {
    fprintf(stderr, "Open and closing tags did not match.");
    exit(1);
  }
}
