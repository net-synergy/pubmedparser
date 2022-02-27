#ifndef QUERY_H
#define QUERY_H

#include <zlib.h>

int get_tag(gzFile fptr, char c, char s[], int str_max);
int get_value(gzFile fptr, char c, char s[], int str_max);
int get_attribute(gzFile fptr, char c, char s[], int str_max);

#endif
