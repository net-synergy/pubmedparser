#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <getopt.h>
#include <omp.h>

#include "read_xml.h"
#include "structure.h"

#define STR_MAX 10000

static void usage(char *program_name, int failed)
{
  if (failed) {
    puts("Called with unknown argument.\n");
  }
  printf("Usage: %s OPTION ... [FILE]...\n", program_name);
  puts("Read XML files and print selected values to files.\n");
  puts("With no FILE read standard input.\n");
  puts("-c, --cache-dir=STRING\tdirectory output files are written to. "
       "Defualts to \"cache\".");
  puts("-s, --structure-file=STRING\ta yaml file with the xml paths to collect. "
       "Defaults to \"structure.yml\".");
  puts("-n, --num-threads=INT\tnumber of indepent threads to use, defaults to OMP_NUM_THREADS.");
  puts("-p, --progress-file=STRING\ta file to collect the names of the xml files "
       "that have been parsed.");
  puts("-w, --overwrite-cache\tIf set, overwrite the files in cache instead of appending them.");
  puts("-h, --help\tIf Show this help.");
}

static struct option const longopts[] = {
  {"cache-dir", required_argument, NULL, 'c'},
  {"structure-file", required_argument, NULL, 's'},
  {"num-threads", required_argument, NULL, 'n'},
  {"progress-file", required_argument, NULL, 'p'},
  {"overwrite-cache", no_argument, NULL, 'w'},
  {"help", no_argument, NULL, 'h'},
  {NULL, 0, NULL, 0}
};

int main(int argc, char **argv)
{
  int optc;
  char *structure_file = "structure.yml";
  char *cache_dir = "cache/";
  int overwrite_cache = CACHE_APPEND;
  char *progress_file = "processed.txt";
  char *program_name = argv[0];
  size_t n_threads = 0;
  #pragma omp parallel
  {
    #pragma omp single
    n_threads = omp_get_num_threads();
  }

  while ((optc = getopt_long(argc, argv, "c:s:n:p:wh", longopts,
                             NULL)) != EOF) {
    switch (optc) {
    case 'c':
      cache_dir = optarg;
      break;
    case 's':
      structure_file = optarg;
      break;
    case 'n':
      n_threads = atoi(optarg);
      omp_set_num_threads(n_threads);
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

  char **files = argv + optind;
  size_t n_files = (size_t)(argc - optind);

  path_struct structure = parse_structure_file(structure_file, STR_MAX);

  int status = 0;
  if (n_files == 0) {
    *files = strdup("-");
    status = read_xml(files, 1, structure, cache_dir, overwrite_cache,
                      progress_file, 1);
    free(*files);
  } else {
    status = read_xml(files, n_files, structure, cache_dir, overwrite_cache,
                      progress_file,
                      n_threads);
  }

  path_struct_destroy(structure);

  return status;
}
