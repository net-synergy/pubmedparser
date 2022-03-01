#ifndef QUERY_H
#define QUERY_H

#include <zlib.h>

#define EMPTY_TAG -10
#define PREV_EMPTY_TAG -20
#define NO_ATTRIBUTE -30

int get_tag(gzFile fptr, char c, char s[], int str_max);
int get_value(gzFile fptr, char c, char s[], int str_max);
int get_attribute(gzFile fptr, char c, char s[], int str_max);

#endif
