/* overlap -- calculate the overlap between nodes
   Overlap is the intersect between the values of two nodes. Expects a
   table with two columns of IDs. Each column represents a node of a
   single type and each row indicates a graph edge.

   So the overlap is the number of common nodes in a bipartisian graph
   between nodes of one type.

   Requires the outer nodes column to be sorted in order to find
   highest ID and to reduce number of calculations (take advantage of
   symmetry).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int get_n_nodes(FILE *fptr, int col)
{
  int vals[] = { 0, 0 };
  char c = EOF;

  fseek(fptr, 0, SEEK_END);
  while (c != '\n') {
    fseek(fptr, -2, SEEK_CUR);
    c = getc(fptr);
  }

  if (fscanf(fptr, "%d\t%d", &vals[0], &vals[1])) {
    rewind(fptr);
    return vals[col];
  } else {
    return -1;
  }

}

int main(int argc, char **argv)
{
  int primary_column = 0;
  int secondary_column = 1;
  int option;
  while ((option = getopt(argc, argv, "k:")) != -1) {
    switch (option) {
    case 'k':
      primary_column = atoi(optarg) - 1;
      if ((primary_column != 0) && (primary_column != 1)) {
        fprintf(stderr, "Error: %s.\n\n%s%d.\n",
                "primary column must be 1 or 2",
                "Recieved \"-k \".", primary_column + 1);
        return EXIT_FAILURE;
      }
      secondary_column = !primary_column;
      break;
    case '?':
      if (optopt == 'k') {
        fprintf(stderr, "Option -k requires an argument.\n");
      } else {
        fprintf(stderr, "Option -%c unknown.\n", optopt);
      }
      return EXIT_FAILURE;
    default:
      abort();
    }
  }

  if (!(optind < argc)) {
    fprintf(stderr, "Error: no edge file provided\n.");
    return EXIT_FAILURE;
  }

  char *f = argv[optind];
  FILE *fptr;
  if (!(fptr = fopen(f, "r"))) {
    fprintf(stderr, "Error: could not open file %s\n", f);
    return EXIT_FAILURE;
  }

  int n_nodes = 0;
  if ((n_nodes = get_n_nodes(fptr, primary_column)) == -1) {
    fprintf(stderr, "Error: %s.\n%s.\n",
            "could not get number of nodes",
            "Last line of input file should have two columns of IDs.");
    return EXIT_FAILURE;
  }

  int **overlap = calloc(n_nodes, sizeof overlap);
  for (int i = 0; i < n_nodes; i++) {
    overlap[i] = calloc(n_nodes, sizeof * overlap);
  }

  // Skip header
  char c;
  while ((c = getc(fptr)) != '\n');

  int current_line = 0;
  int outer[] = { 0, 0 };
  int inner[] = { 0, 0 };
  while (fscanf(fptr, "%d\t%d", &outer[0], &outer[1]) == 2) {
    current_line = ftell(fptr);
    while (fscanf(fptr, "%d\t%d", &inner[0], &inner[1]) == 2) {
      if (outer[secondary_column] == inner[secondary_column]) {
        if (outer[primary_column] > inner[primary_column]) {
          fprintf(stderr, "Error: primary column is not sorted.\n");
          return EXIT_FAILURE;
        }
        overlap[outer[primary_column] - 1][inner[primary_column] - 1]++;
      }
    }
    fseek(fptr, current_line, SEEK_SET);
  }

  for (int i = 0; i < n_nodes; i++) {
    for (int j = i + 1; j < n_nodes; j++) {
      if (overlap[i][j] > 0) {
        printf("%d\t%d\t%d\n", i + 1, j + 1, overlap[i][j]);
      }
    }
  }

  fclose(fptr);
  for (int i = 0; i < n_nodes; i++) {
    free(overlap[i]);
  }
  free(overlap);

  return EXIT_SUCCESS;
}
