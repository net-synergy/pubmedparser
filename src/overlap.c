/* overlap -- calculate the overlap between nodes
   Overlap is the intersect between the values of two nodes. Expects a
   table with two columns of IDs. Each column represents a node of a
   single type and each row indicates a graph edge.

   So the overlap is the number of common nodes in a bipartisian graph
   between nodes of one type.

   Requires the primary nodes column to be sorted in order to find
   highest ID and to reduce number of calculations (take advantage of
   symmetry).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int get_n_nodes(FILE *fptr)
{
  int val_1 = 0;
  int val_2 = 0;
  char c = EOF;

  fseek(fptr, 0, SEEK_END);
  while (c != '\n') {
    fseek(fptr, -2, SEEK_CUR);
    c = getc(fptr);
  }

  if (fscanf(fptr, "%d\t%d", &val_1, &val_2)) {
    rewind(fptr);
    return val_1;
  } else {
    return -1;
  }

}

int main(int argc, char **argv)
{
  char *f = argv[argc - 1];

  FILE *fptr;
  if (!(fptr = fopen(f, "r"))) {
    fprintf(stderr, "Error: could not open file %s\n", f);
    return EXIT_FAILURE;
  }

  int n_nodes = 0;
  if ((n_nodes = get_n_nodes(fptr)) == -1) {
    fprintf(stderr, "Error: %s.\n%s.\n",
	    "could not get number of nodes",
	    "Last line of input file should have two columns of IDs.");
    return EXIT_FAILURE;
  }

  int overlap[n_nodes][n_nodes];
  for (int i = 0; i < n_nodes; i++) {
    for (int j = 0; j < n_nodes; j++) {
      overlap[i][j] = 0;
    }
  }

  char header[2][100];
  if (fscanf(fptr, "%s\t%s", header[0], header[1]) != 2) {
    fprintf(stderr, "Error: could not read header\n");
    return EXIT_FAILURE;
  }

  int current_line = 0;
  int primary[] = { 0, 0 };
  int secondary[] = { 0, 0 };
  while (fscanf(fptr, "%d\t%d", &primary[0], &secondary[0]) == 2) {
    current_line = ftell(fptr);
    while (fscanf(fptr, "%d\t%d", &primary[1], &secondary[1]) == 2) {
      if (secondary[0] == secondary[1]) {
	if (primary[0] > primary[1]) {
	  fprintf(stderr, "Error: primary column is not sorted.\n");
	  return EXIT_FAILURE;
	}
        overlap[primary[0] - 1][primary[1] - 1]++;
      }
    }
    fseek(fptr, current_line, SEEK_SET);
  }

  printf("%s\t%s\tweight\n", header[0], header[0]);
  for (int i = 0; i < n_nodes; i++) {
    for (int j = i + 1; j < n_nodes; j++) {
      if (overlap[i][j] > 0) {
        printf("%d\t%d\t%d\n", i + 1, j + 1, overlap[i][j]);
      }
    }
  }

  fclose(fptr);

  return EXIT_SUCCESS;
}
