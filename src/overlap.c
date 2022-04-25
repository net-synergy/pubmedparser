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
#include <sys/sysinfo.h>

#define total_ram sysconf(_SC_PHYS_PAGES) * sysconf(_SC_PAGESIZE)
#define available_ram sysconf(_SC_AVPHYS_PAGES) * sysconf(_SC_PAGESIZE)
#define used_ram total_ram - available_ram

/* Move forward in file one line. */
char fskipl(FILE *fptr)
{
  char c = '\0';
  while (((c = getc(fptr)) != '\n') && (c != EOF));

  return c;
}

int fget_cols(FILE *fptr, char delim, int *col1, int *col2)
{
  char c = '\0';
  char buff[20];
  int i = 0;
  int coli = 0;

  while ((c = getc(fptr)) != '\n' && c != EOF) {
    if (c == delim) {
      buff[i] = '\0';
      *col1 = atoi(buff);
      coli++;
      i = 0;
    } else {
      buff[i] = c;
      i++;
    }
  }

  buff[i] = '\0';
  *col2 = atoi(buff);
  coli++;

  return coli;
}

void skip_header(FILE *fptr, char delim)
{
  int buff[2];
  int pos = ftell(fptr);
  while (fget_cols(fptr, delim, &buff[0], &buff[1]) == 2) {
    if ((buff[0] > 0) && (buff[1] > 0)) {
      break;
    }
    pos = ftell(fptr);
  }
  fseek(fptr, pos, SEEK_SET);
}

int read_edge_file(char *f, char delim, int *edges[2])
{
  FILE *fptr;
  if (!(fptr = fopen(f, "r"))) {
    fprintf(stderr, "Error: could not open file %s\n", f);
    return EXIT_FAILURE;
  }

  char c = '\0';
  int n_edges = 0;
  skip_header(fptr, delim);
  while ((c = fskipl(fptr)) != EOF) n_edges++;

  edges[0] = malloc(n_edges * sizeof * edges[0]);
  edges[1] = malloc(n_edges * sizeof * edges[1]);
  rewind(fptr);
  skip_header(fptr, delim);
  int i = 0;
  while ((fget_cols(fptr, delim, &edges[0][i], &edges[1][i])) == 2) i++;

  fclose(fptr);

  if (i != n_edges) {
    fprintf(stderr, "Error: %s.%s\n.",
            "could not read all edges in edge file.",
            "Ensure all lines after the header are of the form \%d\t\%d.");
    return EXIT_FAILURE;
  }

  return n_edges;
}

void flush_cache(int **cache, int *offset, int n_cache_rows, int node_offset)
{
  for (int i = 0; i < (n_cache_rows - 1); i++) {
    for (int j = (i + 1); j < n_cache_rows; j++) {
      if (cache[i][j] > 0) {
        printf("%d\t%d\t%d\n",
               i + *offset + node_offset,
               j + node_offset,
               cache[i][j]);
        cache[i][j] = 0;
      }
    }
  }
  *offset += n_cache_rows;
}

int main(int argc, char **argv)
{
  double max_ram = 0.8;
  if (max_ram < 1.0) {
    max_ram *= total_ram;
  }

  int primary_column = 0;
  int secondary_column = 1;
  char delim = '\t';
  int option;
  while ((option = getopt(argc, argv, "k:d:")) != -1) {
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
    case 'd':
      delim = optarg[0];
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

  int n_nodes = 0;
  int min_node = 0;
  int n_edges = 0;
  int *edges[] = { NULL, NULL };
  n_edges = read_edge_file(f, delim, edges);
  min_node = edges[primary_column][0];
  for (int i = 0; i < n_edges; i++) {
    edges[primary_column][i] -= min_node;
  }
  n_nodes = edges[primary_column][n_edges - 1] + 1;

  int **overlap = malloc(n_nodes * sizeof * overlap);
  int n_cache_rows = 0;
  while ((n_cache_rows < n_nodes)) {
    overlap[n_cache_rows] = calloc(n_nodes, sizeof * overlap[n_cache_rows]);
    n_cache_rows++;
  }

  int offset = 0;
  for (int i = 0; i < (n_edges - 1); i++) {
    if (edges[primary_column][i] > (n_cache_rows - 1 + offset)) {
      flush_cache(overlap, &offset, n_cache_rows, min_node);
    }
    for (int j = (i + 1); j < n_edges; j++) {
      if (edges[secondary_column][i] == edges[secondary_column][j]) {
        if (edges[primary_column][i] > edges[primary_column][j]) {
          fprintf(stderr, "Error: primary column is not sorted.\n");
          return EXIT_FAILURE;
        }
        overlap[edges[primary_column][i] - offset][edges[primary_column][j]]++;
      }
    }
  }
  flush_cache(overlap, &offset, n_cache_rows, min_node);

  free(edges[0]);
  free(edges[1]);

  for (int i = 0; i < n_cache_rows; i++) {
    free(overlap[i]);
  }
  free(overlap);

  return EXIT_SUCCESS;
}
