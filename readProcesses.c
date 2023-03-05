#include <stdlib.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <fcntl.h>
#include <dirent.h>
#include <stdio.h>

#include "processes.h"
#include "stringUtils.h"

/**
 * Populate a new row with inode and pid data given the information from getdents.
 * @param source A pointer to the information retrieved by getdents
 * @return A dynamically-allocated processData struct with inode and PID populated from source.
 */
ProcessData *readProcess(linux_dirent *source)
{
    ProcessData *result = (ProcessData *)malloc(sizeof(ProcessData));
    result->inode = source->d_ino;
    result->pid = strtoul(source->d_name, NULL, 10);
    return result;
};

/**
 * Gather data on processes in an array, except for file descriptor data
 * @param size Pointer to int which will store to the number of processes found and put in the return array.
 * @param processIdSelected If set to a non-negative number, then only read process if the PID matches processIdSelected.
 * @return If successful, returns an array of pointers each pointing to data of one process. NULL otherwise.
 */
ProcessData **fetchProcesses(int *size, long processIdSelected)
{
    char getdentsBuffer[GETDENTS_BUFFER_SIZE];
    long numEntries = 0;
    *size = 0;
    struct linux_dirent *dirEntry;

    // table with pid and filename data
    int arraySize = processIdSelected >= 0 ? 1 : 1024;
    ProcessData **processes = (ProcessData **)malloc(sizeof(ProcessData *) * arraySize);

    // open file descriptor to /proc/
    // int procDirFd = open("/proc/", O_RDONLY | O_DIRECTORY);
    int procDirFd = open("/proc/", O_RDONLY | O_DIRECTORY);

    numEntries = syscall(SYS_getdents, procDirFd, getdentsBuffer, GETDENTS_BUFFER_SIZE);
    while (numEntries > 0)
    {
        if (numEntries < 0)
        {
            perror("Error calling getdents");
            return NULL;
        }
        else
        {
            for (long i = 0; i < numEntries;)
            {
                dirEntry = (struct linux_dirent *)(getdentsBuffer + i);

                // retrieve the file type, according to /proc/ docs
                // char d_type = *(getdentsBuffer + i + dirEntry->d_reclen - 1);
                // get subdirectories
                // char *fileType = getType(d_type);
                // printf("%ld %s %s\n", dirEntry->d_ino, fileType, dirEntry->d_name);

                // consider only files with numerical name
                if (isNumber(dirEntry->d_name))
                {
                    // if searching for a specific PID, ignore all others
                    if (processIdSelected < 0 || strtol(dirEntry->d_name, NULL, 10) == processIdSelected)
                    {
                        processes[(*size)++] = readProcess(dirEntry);
                    }
                }
                i += dirEntry->d_reclen;
            }
            numEntries = syscall(SYS_getdents, procDirFd, getdentsBuffer, GETDENTS_BUFFER_SIZE);
        }
    }
    return processes;
}
