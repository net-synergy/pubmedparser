#include <stdlib.h>
#include <string.h>
#include <zlib.h>

#include "read_xml.h"

#define STR_MAX 10000
#define IS_CLOSE(tag) (tag[0] == '/')

#define ADD_TAG(path, tag, ns) {				\
    if (path.length < ns->max_path_depth && path.length >= 0) {	\
      path.components[path.length] = strdup(tag);		\
    }								\
    path.length++;						\
  }

#define RM_TAG(path, ns) {					\
    path.length--;						\
    if (path.length < ns->max_path_depth && path.length >= 0) {	\
      free(path.components[path.length]);			\
    }								\
  }

#define CONTINUE_IF_EMPTY_TAG(c, path, ns) {	\
    if (c == EMPTY_TAG) {			\
      RM_TAG(path, ns)				\
      continue;					\
    }						\
  }

/* Assumes key will only ever have 1 value. */
#define PRINT_NODE(key, node) {				\
    fprintf(node->out, "%s\t", key->values[0]);		\
    for (int pi = 0; pi < (node->n_values - 1); pi++) {	\
      fprintf(node->out,				\
	      "%s\t", node->values[pi]);		\
    }							\
    fprintf(node->out,					\
	    "%s\n",					\
	    node->values[node->n_values - 1]);		\
  }

#define matching_tags(open, close) (strcmp(open, close + 1) == 0)

static int path_match(const path *p1, const path *p2)
{
  if (p1->length != p2->length) return 0;

  int i = p1->length;
  while ((i > 0) &&
         (strcmp(p1->components[i - 1], p2->components[i - 1]) == 0)) i--;

  return i == 0;
}

int parse_file(char *input, node_set *ns)
{
  gzFile fptr;
  if (strcmp(input, "-") == 0) {
    fptr = gzdopen(fileno(stdin), "rb");
  } else {
    fptr = gzopen(input, "rb");
  }
  if (!fptr) {
    fprintf(stderr, "Couldn't open file: %s\n", input);
    exit(1);
  }

  path current = {
    .length = -1,
    .components = malloc(sizeof(char *) * ns->max_path_depth)
  };

  int c = 0;
  char tag[STR_MAX];
  char extra_element[STR_MAX];
  int vali = 0;

  while (c != EOF) {
    c = get_tag(fptr, c, tag, STR_MAX);

    if ((current.length >= 0) && (tag[0] != '?') && (c != EMPTY_TAG)) {
      if (IS_CLOSE(tag) || (c == PREV_EMPTY_TAG)) {
        RM_TAG(current, ns);
      } else {
        ADD_TAG(current, tag, ns);
        for (int i = 0; i < ns->n; i++) {
          if (path_match(&current, ns->nodes[i]->path)) {
            vali = 0;
            if (ns->nodes[i]->attribute != NULL &&
                ns->nodes[i]->expected_attribute == NULL) {
              c = get_attribute(fptr, c, ns->nodes[i]->values[vali], STR_MAX);
              CONTINUE_IF_EMPTY_TAG(c, current, ns);
              vali++;
            }

            if (ns->nodes[i]->n_sub_tags == 0) {
              if (ns->nodes[i]->expected_attribute != NULL) {
                c = get_attribute(fptr, c, extra_element, STR_MAX);
                CONTINUE_IF_EMPTY_TAG(c, current, ns);
                if (strcmp(extra_element, ns->nodes[i]->expected_attribute) == 0) {
                  c = get_value(fptr, c, ns->nodes[i]->values[vali], STR_MAX);
                  CONTINUE_IF_EMPTY_TAG(c, current, ns);
                } else {
                  continue;
                }
              } else {
                c = get_value(fptr, c, ns->nodes[i]->values[vali], STR_MAX);
                CONTINUE_IF_EMPTY_TAG(c, current, ns);
              }
            } else {
              while ((c = get_tag(fptr, c, extra_element, STR_MAX)) != EOF &&
                     (!matching_tags(tag, extra_element))) {
                for (int j = 0; j < ns->nodes[i]->n_sub_tags; j++) {
                  if (!IS_CLOSE(extra_element) &&
                      (strcmp(extra_element, ns->nodes[i]->sub_tags[j]) == 0)) {
                    c = get_value(fptr, c, ns->nodes[i]->values[vali], STR_MAX);
                    vali++;
                  }
                }
              }
              RM_TAG(current, ns);
            }

            if (i != ns->key_idx) {
              PRINT_NODE(ns->nodes[ns->key_idx], ns->nodes[i]);
              for (int j = 0; j < ns->nodes[i]->n_values; j++)
                ns->nodes[i]->values[j][0] = '\0';
            } else {
              fprintf(ns->nodes[ns->key_idx]->out, "%s\n",
                      ns->nodes[ns->key_idx]->values[0]);
            }
          }
        }
      }
    } else {
      if (strcmp(ns->root, tag) == 0)
        ADD_TAG(current, tag, ns);
    }
  }

  free(current.components);
  gzclose(fptr);
  if (current.length == -1) {
    return 0;
  } else {
    fprintf(stderr, "Open and closing tags did not match.\n");
    return 1;
  }
}

void cat(const char *node_name, const char *cache_dir, const int n_threads)
{
  char file_name[1000];
  sprintf(file_name, "%s%s.tsv", cache_dir, node_name);
  FILE *aggregate_file = fopen(file_name, "w");

  for (int i = 0; i < n_threads; i++) {
    sprintf(file_name, "%s%s_%d.tsv", cache_dir, node_name, i);
    FILE *processor_file = fopen(file_name, "r");
    char c = '\0';
    while ((c = getc(processor_file)) != EOF) {
      putc(c, aggregate_file);
    }
    fclose(processor_file);
    remove(file_name);
  }

  fclose(aggregate_file);
}
