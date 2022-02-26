#include "query.h"

#include <stdio.h>
#include <stdlib.h>

#define SEEK_LAST_DELIM(c, fptr) {\
    if (c == '<' || c == '>' || c == '=')			\
      fseek(fptr, -1, SEEK_CUR);				\
  }

int get_tag(FILE *fptr, char s[], int str_max)
{
  int c, i;

  while ((c = fgetc(fptr)) != '<' && c != EOF);
  for (i = 0; i < (str_max - 1) && (c = fgetc(fptr)) != ' ' && c != '>' &&
       c != EOF; i++)
    s[i] = c;

  if (i != 0) {
    s[i] = '\0';
  } else {
    s[0] = '/';
    s[1] = '\0';
  }

  SEEK_LAST_DELIM(c, fptr);
  return c;
}

int get_value(FILE *fptr, char s[], int str_max)
{
  int c, i;

  while ((c = fgetc(fptr)) != '>' && c != EOF);

  for (i = 0; i < (str_max - 1) && (c = fgetc(fptr)) != '<' && c != '"' &&
       c != '\n'; i++)
    s[i] = c;
  s[i] = '\0';

  SEEK_LAST_DELIM(c, fptr);
  return c;
}

int get_attribute(FILE *fptr, char s[], int str_max)
{
  int c, i;

  while ((c = fgetc(fptr)) != '=' && c != '>' && c != EOF);

  if (c == '>') {
    fprintf(stderr,
            "Did not find attribute. \
Attributes must be listed in the order they appear \
if looking for multiple attributes");
    exit(1);
  }

  /* Remove leading '"' */
  c = fgetc(fptr);
  for (i = 0; i < (str_max - 1) && (c = fgetc(fptr)) != ' ' && c != '"' &&
       c != '>'; i++)
    s[i] = c;
  s[i] = '\0';

  SEEK_LAST_DELIM(c, fptr);
  return c;
}
