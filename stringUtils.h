#ifndef STRING_UTILS_H
#define STRING_UTILS_H

#include <stdbool.h>

#define SOCKET_TOKEN "socket:["

#define SOCKET_TOKEN "socket:["
#define PIPE_TOKEN "pipe:["

extern bool startsWith(const char *haystack, const char *needle);

extern bool isNumber(char *checkString);

extern int parseNumericalArgument(long *result, char *argv); 

#endif