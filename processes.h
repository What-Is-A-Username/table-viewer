#ifndef PROCESSES_H
#define PROCESSES_H

#define SYMBOLIC_LINK_BUFFER_SIZE 1024
#define GETDENTS_BUFFER_SIZE 1024

#include <sys/stat.h>

/**
 * Describes file information obtained from getdents.
 */
typedef struct linux_dirent
{
    // inode
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
    unsigned long fd;
    unsigned long inode;
    char filename[SYMBOLIC_LINK_BUFFER_SIZE];
} FileDescriptorEntry;

/**
 * Describes information in a row of the Vnodes table
 */
typedef struct ProcessData
{
    unsigned long pid;
    unsigned long inode;
    unsigned long size;
    FileDescriptorEntry **fileDescriptors;
} ProcessData;

#endif