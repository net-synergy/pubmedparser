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

int fgetl(FILE *fptr)
{
  char c = EOF;
  while (c != '\n') {
    c = getc(fptr);
  }
  return EXIT_SUCCESS;
}

int main(int argc, char **argv)
{
  char *f = argv[argc - 1];

  FILE *fptr_1, *fptr_2;
  if (!(fptr_1 = fopen(f, "r")) || !(fptr_2 = fopen(f, "r"))) {
    fprintf(stderr, "Error: could not open file %s\n", f);
    return EXIT_FAILURE;
  }

  int n_nodes = 0;
  if ((n_nodes = get_n_nodes(fptr_1)) == -1) {
    fprintf(stderr, "Error: %s.\n%s.",
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

  int primary[] = { 0, 0 };
  int secondary[] = { 0, 0 };
  fgetl(fptr_1); // Skip header
  while (fscanf(fptr_1, "%d\t%d", &primary[0], &secondary[0]) == 2) {
    rewind(fptr_2);
    fgetl(fptr_2);
    while (fscanf(fptr_2, "%d\t%d", &primary[1], &secondary[1]) == 2) {
      if (secondary[0] == secondary[1]) {
        overlap[primary[0] - 1][primary[1] - 1]++;
      }
    }
  }

  char header[2][100];
  rewind(fptr_1);
  if (fscanf(fptr_1, "%s\t%s", header[0], header[1]) != 2) {
    fprintf(stderr, "Error: could not read header");
    return EXIT_FAILURE;
  }

  printf("%s\t%s\tweight\n", header[0], header[0]);
  for (int i = 0; i < n_nodes; i++) {
    for (int j = i + 1; j < n_nodes; j++) {
      if (overlap[i][j] > 0) {
        printf("%d\t%d\t%d\n", i + 1, j + 1, overlap[i][j]);
      }
    }
  }

  fclose(fptr_1);
  fclose(fptr_2);

  return EXIT_SUCCESS;
}
