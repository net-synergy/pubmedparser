#include "query.h"

#include <stdio.h>
#include <stdlib.h>
#include <zlib.h>

int get_tag(gzFile fptr, char c, char s[], int str_max)
{
  while (c != '<' && c != EOF) {
    if (c != '/') {
      c = gzgetc(fptr);
    } else {
      if ((c = gzgetc(fptr)) == '>')
	return PREV_EMPTY_TAG;
    }
  }

  int i;
  for (i = 0; i < (str_max - 1) && (c = gzgetc(fptr)) != ' ' && c != '>' &&
       c != EOF; i++)
    s[i] = c;
  s[i] = '\0';

  if (s[i - 1] == '/')
    return EMPTY_TAG;

  return c;
}

int get_value(gzFile fptr, char c, char s[], int str_max)
{
  while (c != '>' && c != EOF) {
    if (c == '/') {
      if ((c = gzgetc(fptr)) == '>')
	return EMPTY_TAG;
    } else {
      c = gzgetc(fptr);
    }
  }

  int i;
  for (i = 0; i < (str_max - 1) && (c = gzgetc(fptr)) != '<' && c != '"' &&
       c != '\n'; i++)
    s[i] = c;
  s[i] = '\0';

  return c;
}

int get_attribute(gzFile fptr, char c, char s[], int str_max)
{
  while (c != '=' && c != '>' && c != EOF) {
    if (c == '/') {
      if ((c = gzgetc(fptr)) == '>')
	return EMPTY_TAG;
    } else {
      c = gzgetc(fptr);
    }
  }

  if (c == '>') {
    return NO_ATTRIBUTE;
  }

  /* Remove leading '"' */
  c = gzgetc(fptr);
  int i;
  for (i = 0; i < (str_max - 1) && (c = gzgetc(fptr)) != ' ' && c != '"' &&
       c != '>'; i++)
    s[i] = c;
  s[i] = '\0';

  return c;
}
