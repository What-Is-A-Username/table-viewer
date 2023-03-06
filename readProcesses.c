#include <stdlib.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <fcntl.h>
#include <dirent.h>
#include <stdio.h>
#include <sys/stat.h>

#include "processes.h"
#include "stringUtils.h"

/**
 * Free memory used to store process and FD data
 * @param processes An array of process data to free
 * @param size The number of elements within processes to free, starting at index 0.
*/
void freeProcesses(ProcessData** processes, int size) {
    // Free array memory used to store processes and file descriptors
    if (processes == NULL) return;
    for (int i = 0; i < size; i++)
    {
        if (processes[i] == NULL) continue;
        if (processes[i]->fileDescriptors != NULL)
        for (int fd = 0; fd < processes[i]->size; fd++)
        {
            if (processes[i]->fileDescriptors[fd] == NULL) continue;
            if (processes[i]->fileDescriptors[fd]->filename != NULL) {
                free(processes[i]->fileDescriptors[fd]->filename);
            }
            free(processes[i]->fileDescriptors[fd]);
        }
        free(processes[i]);
    }
}

/**
 * Populate a new row with inode and pid data given the information from getdents.
 * @param source A pointer to the information retrieved by getdents
 * @return If successful, a dynamically-allocated processData struct with inode and PID populated from source. NULL otherwise.
 */
ProcessData *readProcess(linux_dirent *source)
{
    ProcessData *result = (ProcessData *)malloc(sizeof(ProcessData));
    if (result == NULL) {
        return NULL;
    }
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

    // buffer to store filename of process file
    char processFilename[GETDENTS_BUFFER_SIZE];

    // table with pid and filename data
    int arraySize = processIdSelected >= 0 ? 1 : MAX_PROCESS_COUNT;
    ProcessData **processes = (ProcessData **)malloc(sizeof(ProcessData *) * arraySize);

    // open file descriptor to /proc/
    int procDirFd = open("/proc/", O_RDONLY | O_DIRECTORY);

    struct stat stats;

    // get real user's uid for the user calling the tool
    uid_t currentUid = getuid();

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

                // make path to file, and get stats
                snprintf(processFilename, GETDENTS_BUFFER_SIZE, "/proc/%s", dirEntry->d_name);
                if (lstat(processFilename, &stats) == -1) {
                    freeProcesses(processes, arraySize);
                    fprintf(stderr, "Failed to read stats of file %s", processFilename);
                    return NULL;
                } 

                // skip entries not belonging to current user
                if (stats.st_uid != currentUid) {
                    i += dirEntry->d_reclen;
                    continue;
                }

                // consider only files with numerical name
                if (isNumber(dirEntry->d_name))
                {
                    // if searching for a specific PID, ignore all others
                    if (processIdSelected < 0 || strtol(dirEntry->d_name, NULL, 10) == processIdSelected)
                    {
                        ProcessData* process = readProcess(dirEntry);
                        if (process == NULL) {
                            freeProcesses(processes, arraySize);
                            fprintf(stderr, "Failed to read data for process %s", dirEntry->d_name);
                            return NULL;
                        }
                        processes[(*size)++] = process;
                    }
                }
                i += dirEntry->d_reclen;
            }
            numEntries = syscall(SYS_getdents, procDirFd, getdentsBuffer, GETDENTS_BUFFER_SIZE);
        }
    }
    return processes;
}
