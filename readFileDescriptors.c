#include <unistd.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <fcntl.h>
#include <dirent.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>

#include "processes.h"
#include "stringUtils.h"

/**
 * Extract file descriptor information
 * @param process Data of process to which this file descriptor belongs
 * @param fileEntry File information of the file descriptor file to be read, as retrieved by getdents
 * @param folderPath Absolute path of parent folder containing the file descriptor
 * @return Data about the file descriptor.
 */
FileDescriptorEntry *readFileDescriptor(ProcessData *process, linux_dirent *fileEntry, char *folderPath)
{
    // temp variable to read file descriptor file name
    char fullFdPath[GETDENTS_BUFFER_SIZE * 2];
    // temp variable to store inode string
    char inodeString[SYMBOLIC_LINK_BUFFER_SIZE];

    FileDescriptorEntry *newRow = (FileDescriptorEntry *)malloc(sizeof(FileDescriptorEntry));
    snprintf(fullFdPath, GETDENTS_BUFFER_SIZE * 2, "%s/%s", folderPath, fileEntry->d_name);
    readlink(fullFdPath, newRow->filename, SYMBOLIC_LINK_BUFFER_SIZE);
    newRow->fd = strtol(fileEntry->d_name, NULL, 10);

    // default inode value
    newRow->inode = process->inode;

    // TODO: This returns the wrong inode
    // For sockets and pipes, parse the inode from the string type:[inode]
    int startIndex = -1;

    // sockets
    if (strncmp(newRow->filename, SOCKET_TOKEN, strlen(SOCKET_TOKEN)) == 0)
        startIndex = strlen(SOCKET_TOKEN);
    // FIFO/pipes
    else if (strncmp(newRow->filename, PIPE_TOKEN, strlen(PIPE_TOKEN)) == 0)
        startIndex = strlen(PIPE_TOKEN);
    else {
        // get stats of the file descriptor file
        struct stat stats;
        int fd = open(fullFdPath, O_RDWR);
        if (fstat(fd, &stats) != -1) {
            struct stat inodestats;
            switch(stats.st_mode & S_IFMT)
            {
                case S_IFDIR:
                case S_IFREG:
                case S_IFCHR:
                case S_IFBLK:
                case S_IFLNK:
                    if (lstat(newRow->filename, &inodestats) != -1)
                        newRow->inode = inodestats.st_ino; // inode of file
                default:
                    break;
            }
        }
        close(fd);
    }

    // for pipes and sockets, parse the inode from the string
    if (startIndex > -1)
    {
        int len = strnlen(newRow->filename, SYMBOLIC_LINK_BUFFER_SIZE);
        int i = startIndex;
        // gather digits into a string
        for (; i < len && newRow->filename[i] != ']'; i++)
        {
            inodeString[i - startIndex] = newRow->filename[i];
        }
        if (i < SYMBOLIC_LINK_BUFFER_SIZE)
        {
            inodeString[i - startIndex] = '\0';
        }
        newRow->inode = strtoul(inodeString, NULL, 10);
    }

    // struct stat inodestats;
    // if (lstat(newRow->filename, &inodestats) == -1)
    // {
    //     printf("FD Inodes of %ld: %ld / %ld / %ld / %s / %ld \n", process->pid, fileEntry->d_ino, newRow->inode, stats.st_ino, newRow->filename, 0l);
    //     return NULL;
    // } else {
    //     printf("FD Inodes of %ld: %ld / %ld / %ld / %s / %ld \n", process->pid, fileEntry->d_ino, newRow->inode, stats.st_ino, newRow->filename, inodestats.st_ino);
    // }

    return newRow;
}

/**
 * Given a process of id ID, populate its array of file descriptors with data found
 * in /proc/{ID}/fd.
 * @param process Contains a process identified by PID
 * @returns 0 if operation was successful, nonzero otherwise
 */
int readFileDescriptors(ProcessData *process)
{
    // generate the path and open the folder to search in
    char folderPath[GETDENTS_BUFFER_SIZE];
    snprintf(folderPath, GETDENTS_BUFFER_SIZE, "/proc/%ld/fd/", process->pid);
    int procDirFd = open(folderPath, O_RDONLY | O_DIRECTORY);

    // buffer for getdents
    char entBuffer[GETDENTS_BUFFER_SIZE];

    // number of entries found in the folder
    long numEntries = syscall(SYS_getdents, procDirFd, entBuffer, GETDENTS_BUFFER_SIZE);

    process->fileDescriptors = (FileDescriptorEntry **)malloc(sizeof(FileDescriptorEntry *) * numEntries);
    // reset number of file descs added to zero
    process->size = 0;

    linux_dirent *fileEntry;
    while (numEntries > 0)
    {
        for (long i = 0; i < numEntries;)
        {
            fileEntry = (struct linux_dirent *)(entBuffer + i);

            if (isNumber(fileEntry->d_name))
            {
                // add file descriptor information to process table
                process->fileDescriptors[process->size] = readFileDescriptor(process, fileEntry, folderPath);
                process->size++;
            }
            i += fileEntry->d_reclen;
        }
        numEntries = syscall(SYS_getdents, procDirFd, entBuffer, GETDENTS_BUFFER_SIZE);
    }
    close(procDirFd);
    return 0;
}
