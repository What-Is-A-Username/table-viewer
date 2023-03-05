#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <fcntl.h>
#include <dirent.h>

#include "stringUtils.h"
#include "processes.h"
#include "printTables.h"
#include "readFileDescriptors.h"
#include "readProcesses.h"

#define FILE_LIST_SIZE 1024
#define MAX_COMMAND_LINE_ARGUMENT_LENGTH 64

#define ARG_PER_PROCESS "--per-process"
#define ARG_SYSTEM_WIDE "--systemWide"
#define ARG_VNODES "--Vnodes"
#define ARG_COMPOSITE "--composite"
#define ARG_THRESHOLD "--threshold"
#define ARG_OUTPUT_BINARY "--output_binary"
#define ARG_OUTPUT_TXT "--output_TXT"

#define BINARY_OUT_NAME "compositeTable.bin"
#define TXT_OUT_NAME "compositeTable.txt"

/**
 * Print information of processes that have more file descriptors than the given threshold. These are "offending processes".
 * @param threshold The maximum number of file descriptors a process can have before it is printed.
 * @param processes An array of all processes to consider.
 * @param numProcesses The size of the processes array.
*/
void printOffendingProcesses(long threshold, ProcessData **processes, int numProcesses)
{
    bool foundFirstOffender = false;
    printf("## Offending processes:\n");
    for (int i = 0; i < numProcesses; i++)
    {
        if (processes[i]->size > threshold)
        {
            if (!foundFirstOffender)
            {
                foundFirstOffender = true;
            }
            else
            {
                printf(", ");
            }
            printf("%ld (%ld)", processes[i]->pid, processes[i]->size);
        }
    }
    if (!foundFirstOffender)
    {
        printf("None!");
    }
    printf("\n");
}

/**
 * Entry point of program.
*/
int main(int argc, char **argv)
{
    // TODO: Parse command line arguments
    /**
     * Display only process FD table. Corresponds with ARG_PER_PROCESS.
     */
    bool showPerProcess = false;

    /**
     * Display only the system-wide FD table. Corresponds with ARG_SYSTEM_WIDE command line argument.
     */
    bool showSystemWide = false;

    /**
     * Display only the Vnodes FD table. Corresponds with ARG_VNODES command line argument.
     */
    bool showVnodes = false;

    /**
     * Display only the composite table. Corresponds with ARG_COMPOSITE command line argument.
     */
    bool showComposite = false;

    /**
     * Print offending processes, defined by having more file descriptors than the number set in threshold.
    */
    bool thresholdSet = false;

    /**
     * A threshold set such that all processes with FDs assigned larger than this value will be flagged in output. Corresponds with ARG_THRESHOLD command line argument.
     */
    long threshold = __LONG_MAX__;

    /**
     * A particular process ID, if specified, for which information will be displayed for. Corresponds with the only positional argument allowed.
     */
    long pidArgument = -1;

    /**
     * Has a process ID been set via command line arguments?
     */
    bool pidSet = false;

    // TODO: Implement --output_TXT and --output_binary to save composite table to compositeTable.txt and compositeTable.bin respectively.

    /**
     * Output composite table to text?
     */
    bool outputTxt = false;

    /**
     * Output composite table to binary?
     */
    bool outputBinary = false;

    for (int i = 1; i < argc; i++)
    {
        if (strncmp(argv[i], ARG_PER_PROCESS, MAX_COMMAND_LINE_ARGUMENT_LENGTH) == 0)
        {
            showPerProcess = true;
        }
        else if (strncmp(argv[i], ARG_SYSTEM_WIDE, MAX_COMMAND_LINE_ARGUMENT_LENGTH) == 0)
        {
            showSystemWide = true;
        }
        else if (strncmp(argv[i], ARG_COMPOSITE, MAX_COMMAND_LINE_ARGUMENT_LENGTH) == 0)
        {
            showComposite = true;
        }
        else if (strncmp(argv[i], ARG_VNODES, MAX_COMMAND_LINE_ARGUMENT_LENGTH) == 0)
        {
            showVnodes = true;
        }
        else if (strncmp(argv[i], ARG_OUTPUT_TXT, MAX_COMMAND_LINE_ARGUMENT_LENGTH) == 0)
        {
            outputTxt = true;
        }
        else if (strncmp(argv[i], ARG_OUTPUT_BINARY, MAX_COMMAND_LINE_ARGUMENT_LENGTH) == 0)
        {
            outputBinary = true;
        }
        else if (startsWith(argv[i], ARG_THRESHOLD))
        {
            if (parseNumericalArgument(&threshold, argv[i]) != 0)
            {
                return 1;
            }
            thresholdSet = true;
        }
        else  // parse positional argument
        {
            if (pidSet)
            {
                fprintf(stderr, "Error: Invalid number of positional arguments given.\n");
                return 1;
            }
            else
            {
                pidArgument = atol(argv[i]);
                if (pidArgument == 0)
                {
                    fprintf(stderr, "Error: Invalid positional argument given.\n");
                    return 1;
                }
                pidSet = true;
            }
        }
    }

    // printf("Arguments parsed: %s: %d, %s: %d, %s: %d, %s: %d, %s: %ld, %s: %ld\n", ARG_PER_PROCESS, showPerProcess, ARG_SYSTEM_WIDE, showSystemWide, ARG_VNODES, showVnodes, ARG_COMPOSITE, showComposite, ARG_THRESHOLD, threshold, "PID", pidArgument);

    // retrieve an array of processes
    int numProcessesFound;
    ProcessData **processes = fetchProcesses(&numProcessesFound, pidArgument);
    if (processes == NULL) {
        fprintf(stderr, "Error: Could not read processes.\n");
        return 1;
    }

    // retrieve file descriptor information
    for (int i = 0; i < numProcessesFound; i++)
    {
        if (readFileDescriptors(processes[i]) != 0)
        {
            fprintf(stderr, "Error: Could not read file descriptors for process %ld.\n", processes[i]->pid);
            return -1;
        }
    }

    // print process FD table
    if (showPerProcess)
    {
        print_table(print_perProcess_header, print_perProcess_content, print_perProcess_footer, processes, numProcessesFound, stdout);
    }

    // print system-wide FD table
    if (showSystemWide)
    {
        print_table(print_systemWide_header, print_systemWide_content, print_systemWide_footer, processes, numProcessesFound, stdout);
    }

    // print Vnodes table
    if (showVnodes)
    {
        print_table(print_vnodes_header, print_vnodes_content, print_vnodes_footer, processes, numProcessesFound, stdout);
    }

    // show composite table if explicitly given in arguments, or if no table arguments were given
    // TODO: Should row numbers be added to all tables, not only composite? The two composite tables in the example have varied behavior for whether or not to print row numbers.
    if (showComposite || (!showPerProcess && !showSystemWide && !showVnodes && !showComposite))
    {
        print_table(print_composite_header, print_composite_content, print_composite_footer, processes, numProcessesFound, stdout);
    }

    // output composite table to .txt file
    if (outputTxt) {
        FILE* txtStream = fopen(TXT_OUT_NAME, "w");
        if (txtStream == NULL) {
            perror("Error: Could not open .txt output file");
            return 1;
        }
        print_table(print_composite_header, print_composite_content, print_composite_footer, processes, numProcessesFound, txtStream);
        fclose(txtStream);
    }

    // output process and file descriptor data to binary
    else if (outputBinary) {
        if (print_composite_binary(BINARY_OUT_NAME, processes, numProcessesFound) != 0) {
            fprintf(stderr, "Error: Could not output to binary.\n");
            return 1;
        }
    }

    // print offending processes
    if (thresholdSet) {
        printOffendingProcesses(threshold, processes, numProcessesFound);
    }

    freeProcesses(processes, numProcessesFound);

    return 0;
}