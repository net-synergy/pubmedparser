#include "read_xml.h"

#include "structure.h"

#include <getopt.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define STR_MAX 10000

static inline void putarg(
  char* short_opt, char* long_opt, char* type, char* description)
{
  size_t str_max = 512;
  char long_opt_i[str_max + 1];
  if (type) {
    snprintf(long_opt_i, str_max, "%s=%s", long_opt, type);
  } else {
    snprintf(long_opt_i, str_max, "%s", long_opt);
  }
  printf("  -%s, --%s\n", short_opt, long_opt_i);
  printf("\t\t\t%s\n", description);
}

static void usage(char* program_name, int failed)
{
  if (failed) {
    puts("Called with unknown argument.\n");
  }
  printf("Usage: %s [OPTION]... [FILE]...\n", program_name);
  puts("Read XML files and print selected values to files.\n");
  puts("With no FILE read standard input.\n");
  putarg("c", "cache-dir", "STRING",
    "Directory output files are written to.  Defualts to \"cache\".");
  putarg("s", "structure-file", "STRING",
    "A yaml file with the xml paths to collect.  Defaults to "
    "\"structure.yml\".");
  putarg("n", "num-threads", "INT",
    "Number of independent threads to use, defaults to 1.");
  putarg("p", "progress-file", "STRING",
    "A file to collect the names of the xml files that have been parsed.");
  putarg("w", "overwrite-cache", NULL,
    "If set, overwrite the files in cache instead of appending them.");
  putarg("h", "help", NULL, "Show this help.");
}

static struct option const longopts[] = {
  { "cache-dir", required_argument, NULL, 'c' },
  { "structure-file", required_argument, NULL, 's' },
  { "num-threads", required_argument, NULL, 'n' },
  { "progress-file", required_argument, NULL, 'p' },
  { "overwrite-cache", no_argument, NULL, 'w' },
  { "help", no_argument, NULL, 'h' }, { NULL, 0, NULL, 0 }
};

static void pp_error_handler_exit(
  pp_errno const _, char const* errstr, char const* msg)
{
  fprintf(stderr, "%s\n  %s\n\n", msg, errstr);
  exit(1);
}

static void pp_warn_handler_print(
  pp_errno const _, char const* errstr, char const* msg)
{
  fprintf(stderr, "%s\n  %s\n\n", msg, errstr);
}

static int interruption = 0;

static void signal_handler(int sig) { interruption = sig == SIGINT; }

static int pp_interruption_handler(void) { return interruption; }

int main(int argc, char** argv)
{
  signal(SIGINT, signal_handler);

  int optc;
  char* structure_file = "structure.yml";
  char* cache_dir = "cache/";
  int overwrite_cache = CACHE_APPEND;
  char* progress_file = "processed.txt";
  char* program_name = argv[0];
  size_t n_threads = 1;

  while (
    (optc = getopt_long(argc, argv, "c:s:n:p:wh", longopts, NULL)) != EOF) {
    switch (optc) {
      case 'c':
        cache_dir = optarg;
        break;
      case 's':
        structure_file = optarg;
        break;
      case 'n':
        n_threads = atoi(optarg);
        break;
      case 'p':
        progress_file = optarg;
        break;
      case 'w':
        overwrite_cache = CACHE_OVERWRITE;
        break;
      case 'h':
        usage(program_name, 0);
        return 0;
      default:
        usage(program_name, 1);
        return 1;
    }
  }

  pubmedparser_set_error_handler(pp_error_handler_exit);
  pubmedparser_set_warn_handler(pp_warn_handler_print);
  pubmedparser_set_interruption_handler(pp_interruption_handler);

  char** files = argv + optind;
  size_t n_files = (size_t)(argc - optind);

  path_struct structure = parse_structure_file(structure_file, STR_MAX);

  size_t n_failed = 0;
  if (n_files == 0) {
    *files = strdup("-");
    n_failed = read_xml(
      files, 1, structure, cache_dir, overwrite_cache, progress_file, 1);
    free(*files);
  } else {
    n_failed = read_xml(files, n_files, structure, cache_dir, overwrite_cache,
      progress_file, n_threads);
  }

  path_struct_destroy(structure);

  return n_failed;
}
