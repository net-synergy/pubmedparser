#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <zlib.h>

#include "read_xml.h"

#define STR_MAX 10000

#define OUT_OF_ROOT_SCOPE(tag, ns) (tag)->is_close && (strcmp((tag)->value, (ns)->root) == 0)

#define CONTINUE_IF_EMPTY_TAG(tag, path) {				\
    if (tag->is_empty || tag->was_prev_empty) {				\
      path_drop_last_component(path);					\
      tag->was_prev_empty = false;					\
      continue;								\
    }									\
  }

static int parse_file_i(gzFile fptr, node_set *ns, tag *current_tag)
{
  path current_path = path_init_dynamic(ns->max_path_depth);

  while ((strcmp(ns->root, current_tag->value) != 0) && (!(gzeof(fptr)))) {
    tag_get(fptr, current_tag);
  }

  node *n;
  while (!(gzeof(fptr)) && !(OUT_OF_ROOT_SCOPE(current_tag, ns))) {
    tag_get(fptr, current_tag);

    if (current_tag->is_empty) {
      continue;
    }

    if (current_tag->is_close || current_tag->was_prev_empty) {
      path_drop_last_component(current_path);
      current_tag->was_prev_empty = false;
    } else {
      path_append(current_path, current_tag);
      for (size_t i = 0; i < ns->n_nodes; i++) {
        n = ns->nodes[i];
        if (path_match(current_path, n->path)) {

          if (n->child_ns != NULL) {
            node_set_copy_parents_index(n->child_ns, ns, STR_MAX);
            parse_file_i(fptr, n->child_ns, current_tag);
            path_drop_last_component(current_path);
            node_set_fprintf_condensed_node(n->out, fptr, n->child_ns, STR_MAX);
            node_set_reset_index(n->child_ns);
            continue;
          }

          if (n->value->attribute_name != NULL) {
            attribute_get(fptr, n->value->att_pos, current_tag);
            CONTINUE_IF_EMPTY_TAG(current_tag, current_path);

            if ((n->value->required_attribute_value != NULL) &&
                (!path_attribute_matches_required(fptr, n->value))) {
              continue;
            }

          }

          value_get(fptr, n->value->pos, current_tag);
          CONTINUE_IF_EMPTY_TAG(current_tag, current_path);

          node_set_fprintf_node(n->out, fptr, ns, i, STR_MAX);
        }
      }
    }
  }

  int tags_matched = path_is_empty(current_path);
  path_destroy(current_path);
  return tags_matched;
}

int parse_file(const char *input, node_set *ns)
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

  char s[STR_MAX];
  tag current_tag = {
    .value = s,
    .buff_size = STR_MAX,
    .is_close = false,
    .is_empty = false,
    .was_prev_empty = false
  };

  int status = parse_file_i(fptr, ns, &current_tag);
  gzclose(fptr);

  if (status) {
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
