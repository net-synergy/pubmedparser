#ifndef QUERY_H
#define QUERY_H

#include <stdio.h>

int get_tag(FILE *fptr, char s[], int str_max);
int get_value(FILE *fptr, char s[], int str_max);
int get_attribute(FILE *fptr, char s[], int str_max);

#endif
