// Debugging file

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "processes.h"
#include "printTables.h"

/**
 * Read composite table from a binary file "compositeTable.bin"
 * @param numProcessesFound A pointer to an int that will store the number of processes read from file
 * @return Returns pointer to a dynamically allocated array with composite table data if successful. Returns NULL otherwise.
*/
ProcessData** read_composite_binary(int* numProcessesFound) {
    FILE* binaryStream = fopen("compositeTable.bin", "rb");
    if (binaryStream == NULL) {
        perror("Error opening to .bin output file");
        return NULL;
    }
    FileDescriptorEntry* point;
    size_t filenameLen;
    unsigned long pid = 0l;
    *numProcessesFound = 0;
    
    ProcessData** processes = (ProcessData**)malloc(sizeof(ProcessData*) * MAX_PROCESS_COUNT);

    while (fread(&pid, sizeof(unsigned long), 1, binaryStream) > 0)
    {
        ProcessData* process = (ProcessData*)malloc(sizeof(ProcessData));
        process->pid = pid;
        fread(&(process->inode), sizeof(unsigned long), 1, binaryStream); // process inode
        fread(&(process->size), sizeof(unsigned long), 1, binaryStream); // number of fds
        process->fileDescriptors = (FileDescriptorEntry**)malloc(sizeof(FileDescriptorEntry*) * process->size);
        for (size_t j = 0; j < process->size; j++)
        {
            process->fileDescriptors[j] = (FileDescriptorEntry*)malloc(sizeof(FileDescriptorEntry));
            point = process->fileDescriptors[j];
            fread(&(point->fd), sizeof(unsigned long), 1, binaryStream);
            fread(&(point->inode), sizeof(unsigned long), 1, binaryStream);
            fread(&filenameLen, sizeof(size_t), 1, binaryStream); // length of string
            point->filename = (char*)malloc(sizeof(char) * filenameLen);
            fread(point->filename, sizeof(char), filenameLen, binaryStream);
            point->filename[filenameLen] = '\0';
        }
        processes[(*numProcessesFound)++] = process;
    }
    fclose(binaryStream);
    return processes;
}

int main() {
    int num = 0;
    ProcessData** procs = read_composite_binary(&num);
    if (procs != NULL)
        print_table(print_composite_header, print_composite_content, print_composite_footer, procs, num, stdout);
}