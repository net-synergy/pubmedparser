#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <string.h>

#include "yaml_reader.h"

#define STRMAX 500
#define get_next_value(p, i, container)		\
  while (*p != '\0' && *p != '/') {		\
    container[i] = *p;				\
    p++;					\
    i++;					\
  };						\
  names[i] = '\0';				\

static char *get_names_from_path(char *p)
{
  if (*p != '/') {
    printf("Path malformed in structure file. Must start with '/'.");
    exit(2);
  }
  p++; // Strip initial '/';

  int last_seperators_idx[2] = { -1, -1 };
  for (int i = 0; p[i] != '\0'; i++) {
    if (p[i] == '/') {
      last_seperators_idx[1] = last_seperators_idx[0];
      last_seperators_idx[0] = i;
    }
  }

  if (last_seperators_idx[0] == -1 || last_seperators_idx[1] == -1) {
    printf("Did not find seperators in path. Path malformed in structure file.");
    exit(2);
  }

  /*
    Cases
      Normal: Last value in "/" seperated path.
      {Multiple,Values}: All values in "," seperated and "{}" delimited list at end of path.
      Value/@Attribute: 2nd to last value in path plus last value in path (attribute) with "@" stripped.
      Value/[@Attribute='filter']: 2nd to last value in path *not* last value which is not stored.
  */
  char test_val = p[(*last_seperators_idx) + 1];
  char names[STRMAX];
  int i = 0;
  if (test_val == '{') {          /* Multiple values */
    p += last_seperators_idx[0] + 2; /* Skip '/{' */
    while (*p != '}' && *p != '\0') {
      names[i] = *p;
      i++;
      p++;
    }

    if (*p == '\0') {
      printf("Missing '}' at end of multiple values. Malformed path in structure file.");
      exit(2);
    }

    names[i] = '\0';
  } else if (test_val == '@') {   /* Attribute */
    p += last_seperators_idx[1] + 1;
    get_next_value(p, i, names);
    names[i] = ',';
    i++;
    p += 2; /* Skip '/@' */
    get_next_value(p, i, names);
  } else if (test_val == '[') {   /* Filter */
    p += last_seperators_idx[1] + 1;
    get_next_value(p, i, names);
  } else {                        /* Normal */
    p += last_seperators_idx[0] + 1;
    get_next_value(p, i, names);
  }

  return strdup(names);
}

static struct option const longopts[] = {
  {"structure-file", required_argument, NULL, 's'},
  {NULL, 0, NULL, 0}
};

int main(int argc, char **argv)
{
  int optc;
  char *structure_file = "structure.yml";
  int rc;
  size_t n_items = 0;
  char **key_value_pairs[2];

  while ((optc = getopt_long(argc, argv, "s:", longopts, NULL)) != EOF) {
    switch (optc) {
    case 's':
      structure_file = optarg;
      break;
    default:
      fputs("Recieved an unknown argument.", stderr);
      return 1;
    }
  }

  if ((argc - optind) != 1) {
    fputs("Too many arguments given; requires exactly one key.", stderr);
    return 1;
  }

  rc = yaml_get_map_contents(structure_file, argv[optind], key_value_pairs,
                             &n_items);
  if (rc > 0) {
    return 1;
  }

  for (size_t i = 0; i < n_items; i++) {
    printf("%s: %s\n", key_value_pairs[0][i],
           get_names_from_path(key_value_pairs[1][i]));
  }

  return 0;
}
