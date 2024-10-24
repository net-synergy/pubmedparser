#include "error.h"
#include "nodes.h"
#include "paths.h"
#include "read_xml.h"
#include "structure.h"

#include <dirent.h>
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <zlib.h>

#define STR_MAX 10000

#ifdef _WIN32
  #define mkdir(a, b) mkdir((a))
#endif

#define WAIT(ms)                                                              \
  struct timespec pause = {                                                   \
    .tv_sec = 0,                                                              \
    .tv_nsec = (ms) * 1000000,                                                \
  };                                                                          \
  nanosleep(&pause, NULL)

#define OUT_OF_ROOT_SCOPE(tag, ns)                                            \
  (tag)->is_close && (strcmp((tag)->value, (ns)->root) == 0)

#define CONTINUE_IF_EMPTY_TAG(tag, path)                                      \
  {                                                                           \
    if (tag->is_empty || tag->was_prev_empty) {                               \
      path_drop_last_component(path);                                         \
      tag->was_prev_empty = false;                                            \
      continue;                                                               \
    }                                                                         \
  }

static pthread_mutex_t progress_mutex;
static size_t threads_finished = 0;
static pp_errno global_status = PP_SUCCESS; // Stores fatal errors.

static char* ensure_path_ends_with_slash(char const* p)
{
  char* out = malloc(sizeof(*out) * (STR_MAX + 1));
  strncpy(out, p, STR_MAX);

  int str_len;
  for (str_len = 0; p[str_len] != '\0'; str_len++)
    ;
  str_len--;

  if (out[str_len] != '/') {
    strncat(out, "/", STR_MAX - strlen(out));
  }

  return out;
}

static char* expand_file(char const* filename, char const* dirname)
{
  char temp[STR_MAX + 1];
  strncpy(temp, dirname, STR_MAX);
  strncat(temp, filename, STR_MAX);
  return strdup(temp);
}

#define CHECK(expr)                                                           \
  rs = (expr);                                                                \
  do {                                                                        \
    if ((global_status != PP_SUCCESS) || (rs != PP_SUCCESS)) {                \
      goto cleanup_file;                                                      \
    }                                                                         \
  } while (0)

static pp_errno parse_file_i(gzFile fptr, node_set* ns, tag* current_tag)
{
  pp_errno rs;
  path current_path = path_init_dynamic(ns->max_path_depth);
  char c = '\0';

  while ((strcmp(ns->root, current_tag->value) != 0) && (!(gzeof(fptr)))) {
    CHECK(tag_get(&c, fptr, current_tag));
  }

  node* n;
  while (!(gzeof(fptr)) && !(OUT_OF_ROOT_SCOPE(current_tag, ns))) {
    CHECK(tag_get(&c, fptr, current_tag));

    if (current_tag->is_empty) {
      continue;
    }

    if (current_tag->is_close || current_tag->was_prev_empty) {
      path_drop_last_component(current_path);
      current_tag->was_prev_empty = false;
    } else {
      path_append(current_path, current_tag);
      int att_unused = false;
      for (size_t i = 0; i < ns->n_nodes; i++) {
        n = ns->nodes[i];
        if (path_match(current_path, n->path)) {
          if (n->child_ns) {
            node_set_copy_parents_index(n->child_ns, ns, STR_MAX);
            CHECK(parse_file_i(fptr, n->child_ns, current_tag));
            path_drop_last_component(current_path);
            node_set_fprintf_condensed_node(n->out, n->child_ns, STR_MAX);
            node_set_reset_index(n->child_ns);
            continue;
          }

          if (n->attribute->name) {
            if (!att_unused) {
              CHECK(attribute_get(&c, fptr, n->attribute, current_tag));
            }
            CONTINUE_IF_EMPTY_TAG(current_tag, current_path);

            if ((n->attribute->required_value) &&
                (!path_attribute_matches_required(n))) {
              /* For case when multiple nodes match the same path but have
                 different required attributes. If the earlier node doesn't
                 match, the later node should try on the same attribute instead
                 of trying to read in a new attribute. */
              att_unused = true;
              continue;
            }
          }

          if ((i != ns->key_idx) || (ns->key->type == IDX_NORMAL)) {
            CHECK(value_get(&c, fptr, n->value, current_tag));
            CONTINUE_IF_EMPTY_TAG(current_tag, current_path);
          }

          node_set_fprintf_node(n->out, ns, i, STR_MAX);
          break;
        }
      }
    }
  }

  rs = path_is_empty(current_path) ? PP_SUCCESS : PP_ERR_TAG_MISMATCH;

cleanup_file:
  path_destroy(current_path);

  return rs;
}

struct parse_params {
  size_t tid;
  size_t iter;
  size_t n_threads;
  pp_errno status;
  char** files;
  size_t n_files;
  node_set* ns;
  FILE* progress_ptr;
};

static void* parse_files(void* parameters)
{
  struct parse_params* p = (struct parse_params*)parameters;

  for (p->iter = p->tid; p->iter < p->n_files; p->iter += p->n_threads) {
    node_set_mark(p->ns);

    gzFile fptr;
    if (strcmp(p->files[p->iter], "-") == 0) {
      fptr = gzdopen(fileno(stdin), "rb");
    } else {
      fptr = gzopen(p->files[p->iter], "rb");
    }

    if (!fptr) {
      p->status = PP_ERR_FILE_NOT_FOUND;
      goto clean_iter;
    }

    char s[STR_MAX] = "\0";
    tag current_tag = {
      .value = s,
      .buff_size = STR_MAX,
      .is_close = false,
      .is_empty = false,
      .was_prev_empty = false,
    };

    p->status = parse_file_i(fptr, p->ns, &current_tag);
    gzclose(fptr);

clean_iter:
    if (global_status != PP_SUCCESS) {
      node_set_rewind(p->ns);
      break;
    }

    if (p->status != PP_SUCCESS) {
      node_set_rewind(p->ns);
      // Main thread will reset status to success after handling error.
      while (p->status != PP_SUCCESS) {
        WAIT(32);
      }
    } else {
      pthread_mutex_lock(&progress_mutex);
      fprintf(p->progress_ptr, "%s\n", p->files[p->iter]);
      pthread_mutex_unlock(&progress_mutex);
    }
  }

  pthread_mutex_lock(&progress_mutex);
  threads_finished++;
  pthread_mutex_unlock(&progress_mutex);

  return NULL;
}

/* Used after new file has been written to, so should only be at position 0 if
nothing was written. */
static inline bool is_empty_file(FILE* f) { return ftell(f) == 0; }

static void cat_concat_file_i(
  char const* file_prefix, char const* cache_dir, int const n_threads)
{
  char file_name[STR_MAX];
  snprintf(file_name, STR_MAX, "%s%s.tsv", cache_dir, file_prefix);
  char* agg_file_name = strdup(file_name);
  FILE* aggregate_file = fopen(file_name, "a");

  for (int i = 0; i < n_threads; i++) {
    snprintf(file_name, STR_MAX, "%s%s_%d.tsv", cache_dir, file_prefix, i);
    FILE* processor_file = fopen(file_name, "rb");
    char c = '\0';
    while ((c = getc(processor_file)) != PP_EOF) {
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

struct cat_params {
  size_t tid;
  char** node_names;
  char* cache_dir;
  size_t n_threads;
  size_t n_nodes;
};

static void* thread_cat_concat_files(void* parameters)
{
  struct cat_params* p = (struct cat_params*)parameters;
  for (size_t i = p->tid; i < p->n_nodes; i += p->n_threads) {
    cat_concat_file_i(p->node_names[i], p->cache_dir, p->n_threads);
  }

  return NULL;
}

static size_t cat_count_flat_nodes_i(node_set const* ns)
{
  size_t n_nodes = ns->n_nodes;
  for (size_t i = 0; i < ns->n_nodes; i++) {
    if (ns->nodes[i]->child_ns != NULL) {
      n_nodes += cat_count_flat_nodes_i(ns->nodes[i]->child_ns);
    }
  }

  return n_nodes;
}

static size_t cat_get_nodes_i(node_set const* ns, char** list)
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

static void cat_flatten_node_list_i(
  node_set const* ns, char*** list, size_t* n_nodes)
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
static void cat(
  node_set const* ns, char const* cache_dir, size_t const n_threads)
{
  char** node_names;
  size_t n_nodes;
  cat_flatten_node_list_i(ns, &node_names, &n_nodes);

  pthread_t threads[n_threads];
  struct cat_params params[n_threads];
  for (size_t i = 0; i < n_threads; i++) {
    params[i].tid = i;
    params[i].node_names = node_names;
    params[i].cache_dir = (char*)cache_dir;
    params[i].n_threads = n_threads;
    params[i].n_nodes = n_nodes;
  }

  for (size_t i = 0; i < n_threads; i++) {
    pthread_create(
      &threads[i], NULL, thread_cat_concat_files, (void*)&params[i]);
  }

  for (size_t i = 0; i < n_threads; i++) {
    pthread_join(threads[i], NULL);
  }

  for (size_t i = 0; i < n_nodes; i++) {
    free(node_names[i]);
  }
  free(node_names);
}

static char* dir_parent(char const* path)
{
  size_t path_len = strlen(path);
  char const* p_ptr = path + path_len - 1;
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
  char* parent = malloc(sizeof(*parent) * (new_len + 1));
  for (size_t i = 0; i < new_len; i++) {
    parent[i] = path[i];
  }
  parent[new_len] = '\0';

  return parent;
}

static int mkdir_and_parents(char const* path, mode_t mode)
{
  int status, err;

  status = mkdir(path, mode);
  err = errno;
  // Quietly succeed if the directory already exists.
  if ((status < 0) && (err == EEXIST)) {
    status = 0;
  }

  if ((status < 0) && (err == ENOENT)) {
    char* parent = dir_parent(path);
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
int read_xml(char** files, size_t const n_files, path_struct const ps,
  char const* cache_dir, int const overwrite_cache, char const* progress_file,
  size_t n_threads)
{
  char* cache_dir_i = ensure_path_ends_with_slash(cache_dir);
  char* parsed;
  FILE* progress_ptr;
  size_t n_threads_i = n_threads > n_files ? n_files : n_threads;

  if ((mkdir_and_parents(cache_dir_i, 0777)) < 0) {
    pubmedparser_error(0, "%s", "Failed to make cache directory.");
    free(cache_dir_i);
    return -1;
  }

  if ((progress_file == NULL) ||
      ((n_files == 1) && (strcmp(files[0], "-") == 0))) {
    parsed = strdup("/dev/null");
  } else {
    parsed = expand_file(progress_file, cache_dir_i);
  }

  progress_ptr = fopen(parsed, "a");
  free(parsed);
  if (!progress_ptr) {
    pubmedparser_error(
      PP_ERR_FILE_NOT_FOUND, "%s", "Failed to open progress file.");
    free(cache_dir_i);
    return -1;
  }

  node_set* ns =
    node_set_generate(ps, NULL, cache_dir_i, overwrite_cache, STR_MAX);

  node_set_write_headers(ns, STR_MAX);

  pthread_t threads[n_threads_i];
  pthread_mutex_init(&progress_mutex, NULL);
  struct parse_params params[n_threads_i];
  for (size_t i = 0; i < n_threads_i; i++) {
    params[i].tid = i;
    params[i].iter = 0;
    params[i].n_threads = n_threads_i;
    params[i].status = PP_SUCCESS;
    params[i].files = files;
    params[i].n_files = n_files;
    params[i].ns = node_set_clone(ns, cache_dir_i, i, STR_MAX);
    params[i].progress_ptr = progress_ptr;
  }

  size_t n_failed = 0;
  for (size_t i = 0; i < n_threads_i; i++) {
    pthread_create(&threads[i], NULL, parse_files, (void*)&params[i]);
  }

  while (threads_finished < n_threads_i) {
    WAIT(64);
    if (pubmedparser_interruption()) {
      global_status = PP_INTERRUPTION;
      goto cleanup;
    }

    for (size_t i = 0; i < n_threads_i; i++) {
      if (params[i].status == PP_ERR_OOM) {
        global_status = params[i].status;
        goto cleanup;
      }

      if (params[i].status != PP_SUCCESS) {
        pubmedparser_warn(
          params[i].status, "Error in file %s:", files[params[i].iter]);
        params[i].status = PP_SUCCESS;
        n_failed++;
      }
    }
  }

cleanup:
  for (size_t i = 0; i < n_threads_i; i++) {
    pthread_join(threads[i], NULL);
  }

  if (n_failed > 0) {
    pubmedparser_warn(
      0, "Failed to parse %zu file%s.", n_failed, n_failed > 1 ? "s" : "");
  }

  for (size_t i = 0; i < n_threads_i; i++) {
    node_set_destroy(params[i].ns);
  }
  fclose(progress_ptr);
  pthread_mutex_destroy(&progress_mutex);

  cat(ns, cache_dir_i, n_threads_i);
  node_set_destroy(ns);
  free(cache_dir_i);

  if (global_status != PP_SUCCESS) {
    pubmedparser_error(global_status, "%s\n", "Fatal error:");
  }

  return global_status;
}
