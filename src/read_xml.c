#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <getopt.h>
#include <omp.h>
#include <zlib.h>

#include "query.h"
#include "paths.h"

#define STR_MAX 100
#define IS_CLOSE(tag) (tag[0] == '/')

#define ADD_TAG(path, tag, ns) {			\
    if (path.length < ns->max_path_depth) {		\
      path.components[path.length] = strdup(tag);	\
    }							\
    path.length++;					\
  }

#define RM_TAG(path, ns) {			\
    path.length--;				\
    if (path.length < ns->max_path_depth) {	\
      free(path.components[path.length]);	\
    }						\
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

int path_match(const path *p1, const path *p2)
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

  gzclose(fptr);
  if (current.length == -1) {
    return 0;
  } else {
    fprintf(stderr, "Open and closing tags did not match.\n");
    return 1;
  }
}

static char *ensure_path_ends_with_slash(char *p)
{
  int str_len;
  for (str_len = 0; p[str_len] != '\0'; str_len++);
  str_len--;

  if (p[str_len] != '/') {
    char temp_dir[500];
    strcpy(temp_dir, p);
    strcat(temp_dir, "/");
    p = strdup(temp_dir);
  }

  return p;
}

static char *expandfile(char *filename, char *dirname)
{
  char temp[500];
  strcpy(temp, dirname);
  strcat(temp, filename);
  return strdup(temp);
}

static void usage(char *program_name, int failed)
{
  if (failed) {
    puts("Called with unknown argument.\n");
  }
  printf("Usage: %s OPTION ... [FILE]...\n", program_name);
  puts("Read XML files and print selected values to files.\n");
  puts("With no FILE read standard input.\n");
  puts("-c, --cache-dir=STRING\tdirectory output files are written to. \
Defualts to \"cache\".");
  puts("-s, --structure-file=STRING\ta yaml file with the xml paths to collect. \
Defaults to \"structure.yml\".");
}

static struct option const longopts[] = {
  {"cache-dir", required_argument, NULL, 'c'},
  {"structure-file", required_argument, NULL, 's'},
  {"help", no_argument, NULL, 'h'},
  {NULL, 0, NULL, 0}
};

int main(int argc, char **argv)
{
  int optc;
  char *structure_file = "structure.yml";
  char *cache_dir = "cache";
  char *program_name = argv[0];

  while ((optc = getopt_long(argc, argv, "c:s:h", longopts, NULL)) != EOF) {
    switch (optc) {
    case 'c':
      cache_dir = ensure_path_ends_with_slash(optarg);
      mkdir(cache_dir, 0777);
      break;
    case 's':
      structure_file = optarg;
      break;
    case 'h':
      usage(program_name, 0);
      return 0;
    default:
      usage(program_name, 1);
      return 1;
    }
  }

  char *parsed = expandfile("processed.txt", cache_dir);

  node_set *ns = construct_node_set(structure_file, cache_dir, STR_MAX);

  FILE *progress_ptr;
  if (!(progress_ptr = fopen(parsed, "a"))) {
    fprintf(stderr, "Failed to open parsed file.\n");
    return 1;
  }

  int status = 0;
  if (optind == argc) {
    status = parse_file("-", ns);
  } else {
    int n_threads = 0;
    if (!(getenv("OMP_NUM_THREADS"))) {
      fputs("Error: environment variable \"OMP_NUM_THREADS\" not set.", stderr);
    } else {
      n_threads = atoi(getenv("OMP_NUM_THREADS"));
    }

    node_set *ns_dup[n_threads];
    for (int i = 0; i < n_threads; i++)
      ns_dup[i] = clone_node_set(ns, cache_dir, i, STR_MAX);


    #pragma omp parallel for private (status)
    for (int i = optind; i < argc; i++) {
      status = parse_file(argv[i], ns_dup[omp_get_thread_num()]);

      if (status != 0) {
        fprintf(stderr, "Tag mismatch in file: %s\n", argv[i]);
        exit(1);
      }
      fputs(argv[i], progress_ptr);
    }
  }

  fclose(progress_ptr);

  return status;
}
