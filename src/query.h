#ifndef QUERY_H
#define QUERY_H

#include "error.h"

#include <stdbool.h>
#include <zlib.h>

typedef struct Container {
  char* buff;
  size_t buffsize;
  char const* name;
  char const* required_value;
}* container;

typedef container attribute;
typedef container value;

typedef struct Tag {
  char* value;
  size_t buff_size;
  bool is_empty;
  bool was_prev_empty;
  bool is_close;
} tag;

pp_errno tag_get(char* c, gzFile fptr, tag* t);
pp_errno value_get(char* c, gzFile fptr, value val, tag* t);
pp_errno attribute_get(char* c, gzFile fptr, attribute att, tag* t);

#endif
