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
  for (i = 0; (c = gzgetc(fptr)) != ' ' && c != '>' && i < (str_max - 1) &&
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
  int tag_level = 0;
  char look_ahead = '\0';
  for (i = 0; (c = gzgetc(fptr)) != '\n' && i < (str_max - 1); i++) {
    if (c == '<') {
      look_ahead = gzgetc(fptr);
      gzungetc(look_ahead, fptr);
      if (look_ahead == '/') {
        tag_level--;
      } else {
        tag_level++;
      }

      if (tag_level < 0) {
        break;
      }
    }

    s[i] = c;
  }
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
  for (i = 0; (c = gzgetc(fptr)) != ' ' && c != '"' && i < (str_max - 1) &&
       c != '>'; i++)
    s[i] = c;
  s[i] = '\0';

  return c;
}
