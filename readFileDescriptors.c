#include <unistd.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <fcntl.h>
#include <dirent.h>
#include <string.h>
#include <stdio.h>

#include "processes.h"
#include "stringUtils.h"

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
    // TODO: This returns the wrong inode
    // For sockets and pipes, parse the inode from the string type:[inode]
    char *socketToken = "socket:[";
    char *pipeToken = "pipe:[";
    int startIndex = -1;
    if (strncmp(newRow->filename, pipeToken, strlen(pipeToken)) == 0)
    {
        startIndex = strlen(pipeToken);
    }
    else if (strncmp(newRow->filename, socketToken, strlen(socketToken)) == 0)
    {
        startIndex = strlen(socketToken);
    }
    if (startIndex > -1)
    {
        int len = strnlen(newRow->filename, SYMBOLIC_LINK_BUFFER_SIZE);
        int i = startIndex;
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
    else
    {
        newRow->inode = process->inode;
    }
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
            // retrieve the file type, according to /proc/ docs
            // char d_type = *(entBuffer + i + fileEntry->d_reclen - 1);
            // // get subdirectories
            // char *fileType = getType(d_type);
            // printf("fds: %ld %s %s\n", fileEntry->d_ino, fileType, fileEntry->d_name);
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
    return 0;
}
