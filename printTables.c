#include <stdio.h>
#include <string.h>
#include "processes.h"

/**
 * Print header for the system-wide file descriptor table
 * @param stream Stream to output plain-text to
*/
void print_systemWide_header(FILE *stream)
{
    fprintf(stream, "PID\tFD\tFilename\n");
    fprintf(stream, "===============================\n");
}

/**
 * Print footer for the system-wide file descriptor table
 * @param stream Stream to output plain-text to
*/
void print_systemWide_footer(FILE *stream)
{
    fprintf(stream, "===============================\n");
}

/**
 * Print table rows of a process for the system-wide file descriptor table
 * @param process Process to print
 * @param stream Stream to output plain-text to
*/
void print_systemWide_content(ProcessData *process, FILE *stream)
{
    for (int i = 0; i < process->size; i++)
    {
        fprintf(stream, "%ld\t%ld\t%s\n", process->pid, process->fileDescriptors[i]->fd, process->fileDescriptors[i]->filename);
    }
    return;
}

/**
 * Print header for the process file descriptor table
 * @param stream Stream to output plain-text to
*/
void print_perProcess_header(FILE *stream)
{
    fprintf(stream, "PID\tFD\n");
    fprintf(stream, "===================\n");
}

/**
 * Print footer for the process file descriptor table
 * @param stream Stream to output plain-text to
*/
void print_perProcess_footer(FILE *stream)
{
    fprintf(stream, "===================\n");
}

/**
 * Print table rows of a process for the process file descriptor table
 * @param process Process to print
 * @param stream Stream to output plain-text to
*/
void print_perProcess_content(ProcessData *process, FILE *stream)
{
    for (int i = 0; i < process->size; i++)
    {
        fprintf(stream, "%ld\t%ld\n", process->pid, process->fileDescriptors[i]->fd);
    }
    return;
}

/**
 * Print header for the Vnodes file descriptor table
 * @param stream Stream to output plain-text to
*/
void print_vnodes_header(FILE *stream)
{
    fprintf(stream, "PID\tinode\n");
    fprintf(stream, "===================\n");
}

/**
 * Print footer for the Vnodes file descriptor table
 * @param stream Stream to output plain-text to
*/
void print_vnodes_footer(FILE *stream)
{
    fprintf(stream, "===================\n");
}

/**
 * Print table rows of a process for the Vnodes file descriptor table
 * @param process Process to print
 * @param stream Stream to output plain-text to
*/
/**
 * Print the Vnodes file descriptor table
 * @param rows Information for each row to print
 * @param size The number of elements in rows to print
 */
void print_vnodes_content(ProcessData *process, FILE *stream)
{
    for (int i = 0; i < process->size; i++)
    {
        fprintf(stream, "%ld\t%ld\n", process->pid, process->fileDescriptors[i]->inode);
    }
    return;
}

/**
 * Print header for the composite table
 * @param stream Stream to output plain-text to
*/
void print_composite_header(FILE *stream)
{
    fprintf(stream, "\tPID\tFD\tfilename\tinode\n");
    fprintf(stream, "\t=======================================\n");
    return;
}

/**
 * Print footer for the composite table
 * @param stream Stream to output plain-text to
*/
void print_composite_footer(FILE *stream)
{
    fprintf(stream, "\t=======================================\n");
    return;
}

/**
 * Print the composite table for a process
 * @param process Process to print
 * @param stream Stream to output plain-text to
 */
void print_composite_content(ProcessData *process, FILE *stream)
{
    for (int i = 0; i < process->size; i++)
    {
        fprintf(stream, "%d\t%ld\t%ld\t%s\t%ld\n", i+1, process->pid, process->fileDescriptors[i]->fd, process->fileDescriptors[i]->filename, process->fileDescriptors[i]->inode);
    }
    return;
}

/**
 * Print process and file descriptor data to stream.
 * @param print_header Function used to print the table header
 * @param print_content Function used to print a process in the table
 * @param print_footer Function used to print the table footer
 * @param processes An array of all processes to consider.
 * @param numProcesses The size of the processes array.
 * @param stream Stream to output to
*/
void print_table(void (*print_header)(FILE *),
                 void (*print_content)(ProcessData *, FILE *),
                 void (*print_footer)(FILE *),
                 ProcessData **processes,
                 size_t numProcesses,
                 FILE *stream)
{
    (*print_header)(stream);
    for (size_t i = 0; i < numProcesses; i++)
    {
        (*print_content)(processes[i], stream);
    }
    (*print_footer)(stream);
}

/**
 * Save composite table to binary file.
 * @param processes Structs containing all processes and file descriptors to output to binary
 * @param numProcesses The total number of processes to output
 * @return Returns 0 if operation was successful, nonzero otherwise
*/
int print_composite_binary(char* fileName, ProcessData **processes, int numProcesses) {
    FILE* binaryStream = fopen(fileName, "wb");
    if (binaryStream == NULL) {
        perror("Error opening to .bin output file");
        return -1;
    }
    FileDescriptorEntry* point;
    size_t filenameLen;
    for (size_t i = 0; i < numProcesses; i++)
    {
        fwrite(&processes[i]->pid, sizeof(unsigned long), 1, binaryStream);
        fwrite(&processes[i]->inode, sizeof(unsigned long), 1, binaryStream);
        fwrite(&processes[i]->size, sizeof(unsigned long), 1, binaryStream); // number of fds
        for (size_t j = 0; j < processes[i]->size; j++)
        {
            point = processes[i]->fileDescriptors[j];
            filenameLen = strnlen(point->filename, SYMBOLIC_LINK_BUFFER_SIZE);
            fwrite(&(point->fd), sizeof(unsigned long), 1, binaryStream);
            fwrite(&(point->inode), sizeof(unsigned long), 1, binaryStream);
            fwrite(&filenameLen, sizeof(size_t), 1, binaryStream); // length of string
            for (size_t i = 0; i < filenameLen; i++) // filename string
            {
                fwrite(point->filename + i, sizeof(char), 1, binaryStream);
            }
        }
    }
    fclose(binaryStream);
    return 0;
}

