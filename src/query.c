#include "query.h"

#include <stdio.h>
#include <stdlib.h>

int get_tag(FILE *fptr, char c, char s[], int str_max)
{
  while (c != '<' && c != EOF)
    c = fgetc(fptr);

  int i;
  for (i = 0; i < (str_max - 1) && (c = fgetc(fptr)) != ' ' && c != '>' &&
       c != EOF; i++)
    s[i] = c;
  s[i] = '\0';

  return c;
}

int get_value(FILE *fptr, char c, char s[], int str_max)
{
  while (c != '>' && c != EOF)
  c = fgetc(fptr);

  int i;
  for (i = 0; i < (str_max - 1) && (c = fgetc(fptr)) != '<' && c != '"' &&
       c != '\n'; i++)
    s[i] = c;
  s[i] = '\0';

  return c;
}

int get_attribute(FILE *fptr, char c, char s[], int str_max)
{
  while (c != '=' && c != '>' && c != EOF)
    c = fgetc(fptr);

  if (c == '>') {
    fprintf(stderr,
            "Did not find attribute. \
Attributes must be listed in the order they appear \
if looking for multiple attributes");
    exit(1);
  }

  /* Remove leading '"' */
  c = fgetc(fptr);
  int i;
  for (i = 0; i < (str_max - 1) && (c = fgetc(fptr)) != ' ' && c != '"' &&
       c != '>'; i++)
    s[i] = c;
  s[i] = '\0';

  return c;
}
