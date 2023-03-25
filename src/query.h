#ifndef QUERY_H
#define QUERY_H

#include <stdbool.h>
#include <zlib.h>

typedef struct Tag {
  char *value;
  size_t buff_size;
  bool is_empty;
  bool was_prev_empty;
  bool is_close;
} tag;

void tag_get(gzFile fptr, tag *t);
void value_get(gzFile fptr, z_off_t pos[2], tag *t);
void attribute_get(gzFile fptr, z_off_t pos[2], tag *t);

#endif
