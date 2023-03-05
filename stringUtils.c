#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "processes.h"

/**
 * Print a standardized error to stderr to indicate to the user that the command arguments are incorrect.
 */
void notifyInvalidArguments()
{
    fprintf(stderr, "Error: Command has invalid formatting and could not be parsed.\n");
}

/**
 * Check if a substring exists in a string.
 * @param haystack string to search in
 * @param needle substring to search for
 * @returns true is substring needle is in haystack, false otherwise
 */
bool startsWith(const char *haystack, const char *needle)
{
    return strstr(haystack, needle) == haystack;
}

/**
 * Check whether the given string is a decimal number.
 * @param checkString String to check
 * @return Returns true if checkString only contains decimal digits, false otherwise.
 */
bool isNumber(char *checkString)
{
    int len = strnlen(checkString, GETDENTS_BUFFER_SIZE);
    for (int i = 0; i < len; i++)
    {
        if (checkString[i] < '0' || checkString[i] > '9')
        {
            return false;
        }
    }
    return true;
}

/**
 * Parse an command argument key-value pair separated by an equal sign and store its result.
 * @param result Pointer to where the value will be assigned to
 * @param argv A string representing the command string and the value (e.g. "--samples=3")
 * @returns Returns 0 if operation was successful, 1 otherwise
 */
int parseNumericalArgument(long *result, char *argv)
{
    char *splitToken = strtok(argv, "=");
    splitToken = strtok(NULL, "=");
    if (splitToken == NULL)
    {
        // failed to find a string after the = character
        notifyInvalidArguments();
        return 1;
    }
    long tempResult = atol(splitToken);
    if (tempResult == 0)
    {
        // failed to parse string to number
        notifyInvalidArguments();
        return 1;
    }
    *result = atol(splitToken);
    return 0;
}
