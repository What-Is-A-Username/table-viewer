#ifndef STRING_UTILS_H
#define STRING_UTILS_H

#include <stdbool.h>

extern bool startsWith(const char *haystack, const char *needle);

extern bool isNumber(char *checkString);

extern int parseNumericalArgument(long *result, char *argv); 

#endif