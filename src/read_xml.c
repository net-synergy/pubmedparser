#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <getopt.h>
#include <omp.h>

#include "read_xml.h"
#include "structure.h"

#define STR_MAX 5000

static char *ensure_path_ends_with_slash(char *p)
{
  int str_len;
  for (str_len = 0; p[str_len] != '\0'; str_len++);
  str_len--;

  if (p[str_len] != '/') {
    char temp_dir[STR_MAX];
    strcpy(temp_dir, p);
    strcat(temp_dir, "/");
    p = strdup(temp_dir);
  }

  return p;
}

static char *expand_file(char *filename, char *dirname)
{
  char temp[STR_MAX];
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
  char *cache_dir = "cache/";
  char *program_name = argv[0];

  while ((optc = getopt_long(argc, argv, "c:s:h", longopts, NULL)) != EOF) {
    switch (optc) {
    case 'c':
      cache_dir = ensure_path_ends_with_slash(optarg);
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
  mkdir(cache_dir, 0777);

  char *parsed = expand_file("processed.txt", cache_dir);

  path_struct structure = parse_structure_file(structure_file, STR_MAX);
  node_set *ns = node_set_generate(structure, NULL, cache_dir, STR_MAX);
  path_struct_destroy(structure);

  FILE *progress_ptr;
  if (!(progress_ptr = fopen(parsed, "a"))) {
    fprintf(stderr, "Failed to open parsed file.\n");
    return 1;
  }

  int status = 0;
  if (optind == argc) {
    status = parse_file("-", ns);
  } else {
    /* omp_get_num_threads() returns 1 outside of parallel blocks so
    this is a work around to get the real number of threads ahead of
    time. */
    int n_threads = 0;
    #pragma omp parallel
    {
      #pragma omp single
      n_threads = omp_get_num_threads();
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

      fprintf(progress_ptr, "%s\n", argv[i]);
    }
    for (int i = 0; i < n_threads; i++) {
      release_clone(ns_dup[i]);
    }
    fclose(progress_ptr);

    #pragma omp parallel for
    for (int n = 0; n < ns->n; n++) {
      cat(ns->nodes[n]->name, cache_dir, n_threads);
    }
  }

  return status;
}
