#include "query.h"

#include <stdio.h>
#include <stdlib.h>
#include <zlib.h>

void tag_get(gzFile fptr, tag *t)
{
  char c = gzgetc(fptr);
  while (c != '<' && c != EOF) {
    if (c == '/') {
      if ((c = gzgetc(fptr)) == '>') {
        t->was_prev_empty = true;
        return;
      }
    } else {
      c = gzgetc(fptr);
    }
  }

  c = gzgetc(fptr);
  if (c == '?') {
    t->is_empty = true;
    return;
  }

  if (c == '/') {
    t->is_close = true;
    c = gzgetc(fptr);
  } else {
    t->is_close = false;
  }

  size_t i;
  for (i = 0; c != ' ' && c != '>' && i < (t->buff_size - 1) &&
       c != EOF; i++, c = gzgetc(fptr))
    t->value[i] = c;

  if (t->value[i - 1] == '/') {
    t->is_empty = true;
    i--;
  } else {
    t->is_empty = false;
  }

  t->value[i] = '\0';

  gzungetc(c, fptr);
}

void value_get(gzFile fptr, z_off_t pos[2], tag *t)
{
  char c = gzgetc(fptr);
  while (c != '>' && c != EOF) {
    if (c == '/') {
      if ((c = gzgetc(fptr)) == '>') {
        t->was_prev_empty = true;
        return;
      }
    } else {
      c = gzgetc(fptr);
    }
  }

  pos[0] = gztell(fptr);

  int tag_level = 0;
  char look_ahead = '\0';
  while (((c = gzgetc(fptr)) != '\n') && (c != EOF)) {
    if (c == '<') {
      look_ahead = gzgetc(fptr);
      gzungetc(look_ahead, fptr);
      if (look_ahead == '/') {
        tag_level--;
      } else {
        tag_level++;

        while (((c = gzgetc(fptr)) != '>') && (c != EOF)) {
          if (c == '/') {
            if ((look_ahead = gzgetc(fptr)) == '>') {
              tag_level--;
            }
            gzungetc(look_ahead, fptr);
          }
        }
      }

      if (tag_level < 0) {
        break;
      }
    }
  }

  // gzungetc very rarely returned -1 here and all further operations would
  // return EOF. So replace with gzseek here. But don't understand the error.
  gzseek(fptr, -1, SEEK_CUR);
  pos[1] = gztell(fptr) - pos[0];
}

void attribute_get(gzFile fptr, z_off_t pos[2], tag *t)
{
  char c = gzgetc(fptr);

  while (c != '=' && c != '>' && c != EOF) {
    if (c == '/') {
      if ((c = gzgetc(fptr)) == '>') {
        t->was_prev_empty = true;
        return;
      }
    } else {
      c = gzgetc(fptr);
    }
  }

  if (c == '>') {
    // No attribute found.
    pos[0] = -1;
    pos[1] = 0;
    return;
  }

  /* Remove '=' */
  c = gzgetc(fptr);
  /* Remove leading '"' */
  c = gzgetc(fptr);

  pos[0] = gztell(fptr) - 1;
  int i;
  for (i = 0; c != ' ' && c != '"' && c != '>'; i++, (c = gzgetc(fptr)));

  pos[1] = i;
  c = gzungetc(c, fptr);
}
