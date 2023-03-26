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

/* Used after new file has been written to, so should only be at position 0 if
nothing was written. */
static inline bool is_empty_file(FILE *f)
{
  return ftell(f) == 0;
}

void cat_concat_file_i(const char *file_prefix, const char *cache_dir,
                       const int n_threads)
{
  char file_name[STR_MAX];
  snprintf(file_name, STR_MAX, "%s%s.tsv", cache_dir, file_prefix);
  char *agg_file_name = strdup(file_name);
  FILE *aggregate_file = fopen(file_name, "w");

  for (int i = 0; i < n_threads; i++) {
    snprintf(file_name, STR_MAX, "%s%s_%d.tsv", cache_dir, file_prefix, i);
    FILE *processor_file = fopen(file_name, "r");
    char c = '\0';
    while ((c = getc(processor_file)) != EOF) {
      putc(c, aggregate_file);
    }
    fclose(processor_file);
    remove(file_name);
  }

  if (is_empty_file(aggregate_file)) {
    remove(agg_file_name);
  }

  fclose(aggregate_file);
  free(agg_file_name);
}

static size_t cat_count_flat_nodes_i(const node_set *ns)
{
  size_t n_nodes = ns->n_nodes;
  for (size_t i = 0; i < ns->n_nodes; i++) {
    if (ns->nodes[i]->child_ns != NULL) {
      n_nodes += cat_count_flat_nodes_i(ns->nodes[i]->child_ns);
    }
  }

  return n_nodes;
}

static size_t cat_get_nodes_i(const node_set *ns, char **list)
{
  size_t count = ns->n_nodes;
  for (size_t i = 0; i < ns->n_nodes; i++) {
    list[i] = strdup(ns->nodes[i]->name);
  }

  for (size_t i = 0; i < ns->n_nodes; i++) {
    if (ns->nodes[i]->child_ns != NULL) {
      count += cat_get_nodes_i(ns->nodes[i]->child_ns, list + count);
    }
  }

  return count;
}

static void cat_flatten_node_list_i(const node_set *ns, char ***list,
                                    size_t *n_nodes)
{
  *n_nodes = cat_count_flat_nodes_i(ns);
  *list = malloc(sizeof(**list) * *n_nodes);
  cat_get_nodes_i(ns, *list);
}

/* Concatenate the output files from each processor.

   Each processor gets their own set of output files to prevent cobbling
   results without having to add any locks which could slow down performance.

   *cat* concatenate each processor's files into individual files then deletes
   the extra processor specific files. Additionally, some files that are opened
   for writing are not used, these files will also be cleaned up.
 */
void cat(const node_set *ns, const char *cache_dir, const int n_threads)
{
  char **node_names;
  size_t n_nodes;
  cat_flatten_node_list_i(ns, &node_names, &n_nodes);
  #pragma omp parallel for
  for (size_t i = 0; i < n_nodes; i++) {
    cat_concat_file_i(node_names[i], cache_dir, n_threads);
  }

  for (size_t i = 0; i < n_nodes; i++) {
    free(node_names[i]);
  }
  free(node_names);
}
