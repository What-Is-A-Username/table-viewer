#ifndef PROCESSES_H
#define PROCESSES_H

#define SYMBOLIC_LINK_BUFFER_SIZE 1024
#define GETDENTS_BUFFER_SIZE 1024
#define MAX_PROCESS_COUNT 2048

#include <sys/stat.h>

/**
 * Describes file information obtained from getdents.
 */
typedef struct linux_dirent
{
    unsigned long d_ino;
    off_t d_off;
    unsigned short d_reclen;
    char d_name[];
} linux_dirent;

/**
 * Describes information in a a row of the composite table
 */
typedef struct fileDescriptorEntry
{
    /**
     * File descriptor data
    */
    unsigned long fd;
    /**
     * Inode
    */
    unsigned long inode;
    /**
     * Filename
    */
    char* filename;
} FileDescriptorEntry;

/**
 * Describes information in a row of the Vnodes table
 */
typedef struct ProcessData
{
    /**
     * Process identifier (PID)
    */
    unsigned long pid;
    /**
     * Inode of entry within /proc/
    */
    unsigned long inode;
    /**
     * Number of file descriptors stored in **fileDescriptors
    */
    unsigned long size;
    /**
     * All file descriptors of the process
    */
    FileDescriptorEntry **fileDescriptors;
} ProcessData;

#endif