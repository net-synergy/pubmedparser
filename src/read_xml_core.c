#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <zlib.h>
#include <omp.h>

#include "structure.h"
#include "nodes.h"
#include "error.h"

#define STR_MAX 10000

#define OUT_OF_ROOT_SCOPE(tag, ns) (tag)->is_close && (strcmp((tag)->value, (ns)->root) == 0)

#define CONTINUE_IF_EMPTY_TAG(tag, path) {				\
    if (tag->is_empty || tag->was_prev_empty) {				\
      path_drop_last_component(path);					\
      tag->was_prev_empty = false;					\
      continue;								\
    }									\
  }

static char *ensure_path_ends_with_slash(const char *p)
{
  char *out = malloc(sizeof(*out) * (STR_MAX + 1));
  strncpy(out, p, STR_MAX);

  int str_len;
  for (str_len = 0; p[str_len] != '\0'; str_len++);
  str_len--;

  if (out[str_len] != '/') {
    strncat(out, "/", STR_MAX);
  }

  return out;
}

static char *expand_file(const char *filename, const char *dirname)
{
  char temp[STR_MAX + 1];
  strncpy(temp, dirname, STR_MAX);
  strncat(temp, filename, STR_MAX);
  return strdup(temp);
}

static int parse_file_i(gzFile fptr, node_set *ns, tag *current_tag)
{
  path current_path = path_init_dynamic(ns->max_path_depth);
  char c = '\0';

  while ((strcmp(ns->root, current_tag->value) != 0) && (!(gzeof(fptr)))) {
    c = tag_get(c, fptr, current_tag);
  }

  node *n;
  while (!(gzeof(fptr)) && !(OUT_OF_ROOT_SCOPE(current_tag, ns))) {
    c = tag_get(c, fptr, current_tag);

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

          if (n->child_ns) {
            node_set_copy_parents_index(n->child_ns, ns, STR_MAX);
            parse_file_i(fptr, n->child_ns, current_tag);
            path_drop_last_component(current_path);
            node_set_fprintf_condensed_node(n->out, n->child_ns, STR_MAX);
            node_set_reset_index(n->child_ns);
            continue;
          }

          if (n->attribute->name) {
            c = attribute_get(c, fptr, n->attribute, current_tag);
            CONTINUE_IF_EMPTY_TAG(current_tag, current_path);

            if ((n->attribute->required_value) &&
                (!path_attribute_matches_required(n))) {
              continue;
            }
          }

          if ((i != ns->key_idx) || (ns->key->type == IDX_NORMAL)) {
            c = value_get(c, fptr, n->value, current_tag);
            CONTINUE_IF_EMPTY_TAG(current_tag, current_path);
          }

          node_set_fprintf_node(n->out, ns, i, STR_MAX);
        }
      }
    }
  }

  int tags_matched = path_is_empty(current_path);
  path_destroy(current_path);
  return tags_matched;
}

static int parse_file(const char *input, node_set *ns)
{
  gzFile fptr;
  if (strcmp(input, "-") == 0) {
    fptr = gzdopen(fileno(stdin), "rb");
  } else {
    fptr = gzopen(input, "rb");
  }
  if (!fptr) {
    pubmedparser_error(PP_ERR_FILE_NOT_FOUND, "Could not open file %s\n", input);
    return PP_ERR_FILE_NOT_FOUND;
  }

  char s[STR_MAX] = "\0";
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
    return PP_ERR_TAG_MISMATCH;
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
  FILE *aggregate_file = fopen(file_name, "a");

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

void cat_delete_empty_files_i(const char *file_prefix, const char *cache_dir)
{
  char file_name[STR_MAX];
  snprintf(file_name, STR_MAX, "%s%s.tsv", cache_dir, file_prefix);
  FILE *fptr = fopen(file_name, "r");
  fseek(fptr, 0L, SEEK_END);

  if (ftell(fptr) == 0) {
    remove(file_name);
  }
  fclose(fptr);
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
static void cat(const node_set *ns, const char *cache_dir,
                const int n_threads)
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

static char *dir_parent(const char *path)
{
  size_t path_len = strlen(path);
  const char *p_ptr = path + path_len - 1;
  size_t count = 0;
  if (*p_ptr == '/') {
    count++;
    p_ptr--;
  }

  while ((*p_ptr != '/') && (count != (path_len - 1))) {
    count++;
    p_ptr--;
  }

  size_t new_len = (size_t)(p_ptr - path);
  char *parent = malloc(sizeof(*parent) * (new_len + 1));
  for (size_t i = 0; i < new_len; i++) {
    parent[i] = path[i];
  }
  parent[new_len] = '\0';

  return parent;
}

static int mkdir_and_parents(const char *path, mode_t mode)
{
  int status, err;

  status = mkdir(path, mode);
  err = errno;
  // Quietly succeed if the directory already exists.
  if ((status < 0) && (err == EEXIST)) {
    status = 0;
  }

  if ((status < 0) && (err == ENOENT)) {
    char *parent = dir_parent(path);
    mkdir_and_parents(parent, mode);
    free(parent);
    status = mkdir_and_parents(path, mode);
  }

  return status;
}

/* Read the elements of XML files specified by the path structure.

   parameters
   ==========
   files: a list of XML files to parse, if "-" read from stdin.
   n_files: number of files in *files*.
   ps: a path structure indicating which values to read from the files using
       xpath syntax.
   cache_dir: the directory to store the results in (created if it doesn't
       exist).
   progress_file: the name of a text file to save the names of the input files
       that have been read. This file will be appended to on repeated calls. It
       is intended to be used to allow the caller to filter the list of files
       to those that have not already been read before calling the read_xml in
       the case new XML files are being collected regularly. If set to NULL, it
       will not be used.
   n_threads: number of threads to use for parallel processing, if 1 don't
       use OMP.
 */
int read_xml(char **files, const size_t n_files, const path_struct ps,
             const char *cache_dir, const int overwrite_cache, const char *progress_file,
             size_t n_threads)
{
  char *cache_dir_i = ensure_path_ends_with_slash(cache_dir);
  char *parsed;
  FILE *progress_ptr;

  if ((mkdir_and_parents(cache_dir_i, 0777)) < 0) {
    pubmedparser_error(1, "Failed to make cache directory.");
  }

  if ((progress_file != NULL) || ((n_files == 1) &&
                                  (strcmp(files[0], "-") == 0))) {
    parsed = expand_file(progress_file, cache_dir_i);
  } else {
    parsed = strdup("/dev/null");
  }

  if (!(progress_ptr = fopen(parsed, "a"))) {
    pubmedparser_error(PP_ERR_FILE_NOT_FOUND, "Failed to open progress file.\n");
  }
  free(parsed);

  node_set *ns = node_set_generate(ps, NULL, cache_dir_i, overwrite_cache,
                                   STR_MAX);
  node_set *ns_dup[n_threads];
  for (size_t i = 0; i < n_threads; i++) {
    ns_dup[i] = node_set_clone(ns, cache_dir_i, i, STR_MAX);
  }

  node_set_write_headers(ns, STR_MAX);

  #pragma omp parallel for
  for (size_t i = 0; i < n_files; i++) {
    int status = parse_file(files[i], ns_dup[omp_get_thread_num()]);

    if (status != 0) {
      pubmedparser_error(status, "Error in file %s\n", files[i]);
    }

    fprintf(progress_ptr, "%s\n", files[i]);
  }

  for (size_t i = 0; i < n_threads; i++) {
    node_set_destroy(ns_dup[i]);
  }
  fclose(progress_ptr);

  cat(ns, cache_dir_i, n_threads);
  node_set_destroy(ns);
  free(cache_dir_i);

  return EXIT_SUCCESS;
}
