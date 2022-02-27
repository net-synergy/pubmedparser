#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

/* Assumes key will only ever has 1 value. */
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

int path_match(path *p1, path *p2)
{
  if (p1->length != p2->length) return 0;

  int i = p1->length;
  while ((i > 0) &&
         (strcmp(p1->components[i - 1], p2->components[i - 1]) == 0)) i--;

  return i == 0;
}

int matching_tags(char *open, char *close)
{
  close++;
  return strcmp(open, close) == 0;
}

int parse_file(char *input, node_set *ns)
{
  gzFile fptr;
  if (!(fptr = gzopen(input, "rb"))) {
    fprintf(stderr, "Couldn't open file: %s", input);
    exit(1);
  }

  path current = {
    .length = 0,
    .components = malloc(sizeof(char *) * ns->max_path_depth)
  };

  int c = 0;
  char tag[STR_MAX];
  char extra_element[STR_MAX];
  int vali = 0;

  while (c != EOF) {
    c = get_tag(fptr, c, tag, STR_MAX);
    if (current.length > 0 && tag[0] != '?') {
      if (IS_CLOSE(tag)) {
        RM_TAG(current, ns);
      } else {
        ADD_TAG(current, tag, ns);
        for (int i = 0; i < ns->n; i++) {
          if (path_match(&current, ns->nodes[i]->path)) {
            vali = 0;
            if (ns->nodes[i]->attribute != NULL &&
                ns->nodes[i]->expected_attribute == NULL) {
              c = get_attribute(fptr, c, ns->nodes[i]->values[vali], STR_MAX);
              vali++;
            }

            if (ns->nodes[i]->n_sub_tags == 0) {
	      if (ns->nodes[i]->expected_attribute != NULL) {
		c = get_attribute(fptr, c, extra_element, STR_MAX);
		if (strcmp(extra_element, ns->nodes[i]->expected_attribute) == 0) {
		  c = get_value(fptr, c, ns->nodes[i]->values[vali], STR_MAX);
		}
	      } else {
		c = get_value(fptr, c, ns->nodes[i]->values[vali], STR_MAX);
	      }
	    } else {
              strcpy(extra_element, tag);
              while ((c = get_tag(fptr, c, tag, STR_MAX)) != EOF &&
                     (!matching_tags(extra_element, tag))) {
                for (int j = 0; j < ns->nodes[i]->n_sub_tags; j++) {
                  if (!IS_CLOSE(tag) &&
                      (strcmp(tag, ns->nodes[i]->sub_tags[j]) == 0)) {
                    c = get_value(fptr, c, ns->nodes[i]->values[vali], STR_MAX);
                    vali++;
                  }
                }
              }
              RM_TAG(current, ns);
            }

            if (i != ns->key_idx) {
              PRINT_NODE(ns->nodes[ns->key_idx], ns->nodes[i]);
              for (int j = 0; j < ns->nodes[i]->n_values; j++) {
                ns->nodes[i]->values[j][0] = '\0';
              }
            }
          }
        }
      }
    } else {
      if (strcmp(ns->root, tag) == 0) {
        ADD_TAG(current, tag, ns);
      }
    }
  }

  if (current.length == 0) {
    return 0;
  } else {
    fprintf(stderr, "Open and closing tags did not match.");
    exit(1);
  }
}

int main()
{
  char *input = "../data/pubmed21n0001.xml.gz";
  /* char *input = "../data/pubmed21n1001.xml.gz"; */
  char *parsed = "../cache/parsed.txt";
  char *root = "PubmedArticleSet";
  char *cache_dir = "../cache/";

  char *names[] = {
    "PMID",
    "Year",
    "Language",
    "Author",
    "Chemical",
    "Reference"
  };

  char *xpaths[] = {
    "./PubmedArticleSet/PubmedArticle/MedlineCitation/PMID",
    "./PubmedArticleSet/PubmedArticle/MedlineCitation/Article/Journal/JournalIssue/PubDate/Year",
    "./PubmedArticleSet/PubmedArticle/MedlineCitation/Article/Language",
    "./PubmedArticleSet/PubmedArticle/MedlineCitation/Article/AuthorList/Author/{LastName,ForeName}",
    "./PubmedArticleSet/PubmedArticle/MedlineCitation/ChemicalList/Chemical/NameOfSubstance/@UI",
    "./PubmedArticleSet/PubmedArticle/PubmedData/ReferenceList/Reference/ArticleIdList/ArticleId/[@IdType='pubmed']"
  };
  int key_idx = 0;

  int n_nodes = (sizeof(xpaths) / sizeof(*xpaths));
  node_set *ns = construct_node_set(root, xpaths, n_nodes, names, key_idx,
                                    STR_MAX, cache_dir);

  FILE *progress_ptr;
  if (!(progress_ptr = fopen(parsed, "a"))) {
    fprintf(stderr, "Failed to open parsed file.");
    exit(3);
  }

  parse_file(input, ns);
  fprintf(progress_ptr, "%s\n", input);
}
