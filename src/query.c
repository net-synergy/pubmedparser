#include <stdio.h>
#include <stdlib.h>
#include <zlib.h>

#include "query.h"
#include "error.h"

static inline void container_realloc(container c)
{
  c->buffsize *= 2;
  c->buff = realloc(c->buff, sizeof(*c->buff) * (c->buffsize + 1));
}

char tag_get(char c, gzFile fptr, tag *t)
{
  while (c != '<' && c != EOF) {
    if (c == '/') {
      if ((c = gzgetc(fptr)) == '>') {
        t->was_prev_empty = true;
        return c;
      }
    } else {
      c = gzgetc(fptr);
    }
  }

  if (c == EOF) {
    pubmedparser_error(PP_ERR_EOF, "End of file while searching for tag.\n");
  }

  c = gzgetc(fptr);
  if (c == '?') {
    t->is_empty = true;
    return c;
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

  if (c == EOF) {
    pubmedparser_error(PP_ERR_EOF, "End of file while searching for tag.\n");
  }

  if (t->value[i - 1] == '/') {
    t->is_empty = true;
    i--;
  } else {
    t->is_empty = false;
  }

  t->value[i] = '\0';

  return c;
}

static inline void trim_whitespace(char *buff, size_t len)
{
  size_t start = 0;
  size_t end = len - 1;
  while (buff[start] == ' ' && start < len) start++;
  while (buff[end] == ' ' && end > 0) end--;
  for (size_t i = start; start > 0 && i < (end + 1); i++) {
    buff[i - start] = buff[i];
  }
  end -= start;
  buff[end + 1] = '\0';
}

char value_get(char c, gzFile fptr, value val, tag *t)
{
  while (c != '>' && c != EOF) {
    if (c == '/') {
      if ((c = gzgetc(fptr)) == '>') {
        t->was_prev_empty = true;
        return c;
      }
    } else {
      c = gzgetc(fptr);
    }
  }

  if (c == EOF) {
    pubmedparser_error(PP_ERR_EOF, "End of file while searching for value.\n");
  }

  int tag_level = 0;
  char look_ahead = '\0';
  size_t count = 0;
  while ((c = gzgetc(fptr)) != EOF) {
    c = ((c == '\n') || (c == '\t')) ? ' ' : c;
    val->buff[count] = c;
    count++;
    if (count == val->buffsize) {
      container_realloc(val);
    }
    if (c == '<') {
      look_ahead = gzgetc(fptr);
      gzungetc(look_ahead, fptr);
      if (look_ahead == '/') {
        tag_level--;
      } else {
        tag_level++;

        do {
          c = gzgetc(fptr);
          val->buff[count] = c;
          count++;
          if (count == val->buffsize) {
            container_realloc(val);
          }
          if (c == '/') {
            if ((look_ahead = gzgetc(fptr)) == '>') {
              tag_level--;
            }
            gzungetc(look_ahead, fptr);
          }
        } while ((c != '>') && (c != EOF));
      }

      if (tag_level < 0) {
        break;
      }
    }
  }

  if (c == EOF) {
    pubmedparser_error(PP_ERR_EOF, "End of file while searching for value.\n");
  }

  count--; // Last c read is '<'.
  val->buff[count] = '\0';
  trim_whitespace(val->buff, count);
  return c;
}

char attribute_get(char c, gzFile fptr, attribute att, tag *t)
{
  while (c != '=' && c != '>' && c != EOF) {
    if (c == '/') {
      if ((c = gzgetc(fptr)) == '>') {
        t->was_prev_empty = true;
        return c;
      }
    } else {
      c = gzgetc(fptr);
    }
  }

  if (c == EOF) {
    pubmedparser_error(PP_ERR_EOF,
                       "End of file while searching for attribute.\n");
  }

  if (c == '>') {
    // No attribute found.
    att->buff[0] = '\0';
    return c;
  }

  /* Remove '=' */
  c = gzgetc(fptr);
  /* Remove leading '"' */
  c = gzgetc(fptr);

  size_t i;
  for (i = 0; c != ' ' && c != '"' &&
       c != '>' && c != EOF; i++) {
    if (i == att->buffsize) {
      container_realloc(att);
    }
    att->buff[i] = c;
    c = gzgetc(fptr);
  }

  if (c == EOF) {
    pubmedparser_error(PP_ERR_EOF,
                       "End of file while searching for attribute.\n");
  }

  att->buff[i++] = '\0';

  return c;
}
