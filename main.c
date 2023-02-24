#include <dirent.h>
#include <fcntl.h>
#include <sys/syscall.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#define GETDENTS_BUFFER_SIZE 1024
#define FILE_LIST_SIZE 1024
#define SYMBOLIC_LINK_BUFFER_SIZE 1024

typedef struct linux_dirent
{
    // inode
    unsigned long d_ino;
    off_t d_off;
    unsigned short d_reclen;
    char d_name[];
} linux_dirent;

typedef struct dataRow
{
    unsigned long pid;
    unsigned long fd;
    unsigned long inode;
    char filename[SYMBOLIC_LINK_BUFFER_SIZE];
} dataRow;

typedef struct processData
{
    unsigned long pid;
    unsigned long inode;
} processData;

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

processData *populateRow(linux_dirent *source)
{
    processData *result = (processData *)malloc(sizeof(dataRow));
    result->inode = source->d_ino;
    result->pid = strtoul(source->d_name, NULL, 10);
    return result;
};

char *getType(char d_type)
{
    switch (d_type)
    {
    case DT_DIR:
        // directory
        return "directory";
    case DT_FIFO:
        // FIFO
        return "fifo";
    case DT_SOCK:
        // socket
        return "socket";
    case DT_LNK:
        // symlink
        return "symlink";
    case DT_BLK:
        // block dev:
        return "block dev";
    case DT_CHR:
        // char dev
        return "char dev";
    default:
        // unknown
        return "unknown";
        break;
    }
}

/**
 * Given details of a PID in process, append all its of its file descriptors to aggregate and return the number of
 * file descriptors added.
 * @param process Contains a process identified by PID
 * @param aggregate The array of file descriptors to append to
 * @param startingSize The initial number of file descriptors in aggregate
 * @returns The number of file descriptors, belonging to the process, that were added.
 */
int readFileDescriptors(processData *process, dataRow **aggregate, int startingSize)
{
    int nextIndex = startingSize;
    char folderPath[GETDENTS_BUFFER_SIZE];
    char fullFdPath[GETDENTS_BUFFER_SIZE * 2];
    char fdFile[GETDENTS_BUFFER_SIZE];
    snprintf(folderPath, GETDENTS_BUFFER_SIZE, "/proc/%ld/fd/", process->pid);
    int procDirFd = open(folderPath, O_RDONLY | O_DIRECTORY);
    char entBuffer[GETDENTS_BUFFER_SIZE];
    long numEntries = syscall(SYS_getdents, procDirFd, entBuffer, GETDENTS_BUFFER_SIZE);
    linux_dirent *fileEntry;
    while (numEntries > 0)
    {
        for (long i = 0; i < numEntries;)
        {
            fileEntry = (struct linux_dirent *)(entBuffer + i);
            // retrieve the file type, according to /proc/ docs
            char d_type = *(entBuffer + i + fileEntry->d_reclen - 1);
            // get subdirectories
            char *fileType = getType(d_type);
            // printf("fds: %ld %s %s\n", fileEntry->d_ino, fileType, fileEntry->d_name);
            if (isNumber(fileEntry->d_name))
            {
                dataRow *newRow = (dataRow *)malloc(sizeof(dataRow));
                snprintf(fullFdPath, GETDENTS_BUFFER_SIZE * 2, "%s/%s", folderPath, fileEntry->d_name);
                readlink(fullFdPath, newRow->filename, SYMBOLIC_LINK_BUFFER_SIZE);
                newRow->pid = process->pid;
                newRow->fd = nextIndex;
                // TODO: This returns the wrong inode
                newRow->inode = process->inode;
                // printf("%s", newRow->filename);
                aggregate[nextIndex++] = newRow;
            }
            i += fileEntry->d_reclen;
        }
        numEntries = syscall(SYS_getdents, procDirFd, entBuffer, GETDENTS_BUFFER_SIZE);
    }
    return nextIndex - startingSize;
}

void print_processes(processData **rows, int size)
{
    printf("PID\tinode\n");
    for (int i = 0; i < size; i++)
    {
        printf("%ld\t%ld\n", rows[i]->pid, rows[i]->inode);
    }
    return;
}

void print_fds(dataRow **rows, int size)
{
    printf("PID\tFD\tfilename\tinode\n");
    for (int i = 0; i < size; i++)
    {
        printf("%ld\t%ld\t%s\t%ld\n", rows[i]->pid, rows[i]->fd, rows[i]->filename, rows[i]->inode);
    }
    return;
}

int main(int argc, char **argv)
{
    // TODO: Parse command line arguments
    char getdentsBuffer[GETDENTS_BUFFER_SIZE];
    long numEntries = 0;
    struct linux_dirent *dirEntry;

    /**
     * The number of process entries added to rows
     */
    int numProcessesFound = 0;
    // table with pid and filename data
    processData *processes[1024];

    // the number of fds added to fds
    int numFdsFound = 0;
    // table with fds
    dataRow *fds[1024];

    // open file descriptor to /proc/
    // int procDirFd = open("/proc/", O_RDONLY | O_DIRECTORY);
    int procDirFd = open("/proc/", O_RDONLY | O_DIRECTORY);

    numEntries = syscall(SYS_getdents, procDirFd, getdentsBuffer, GETDENTS_BUFFER_SIZE);
    while (numEntries > 0)
    {
        if (numEntries < 0)
        {
            perror("Error calling getdents");
            return 1;
        }
        else
        {
            for (long i = 0; i < numEntries;)
            {
                dirEntry = (struct linux_dirent *)(getdentsBuffer + i);
                // retrieve the file type, according to /proc/ docs
                char d_type = *(getdentsBuffer + i + dirEntry->d_reclen - 1);
                // get subdirectories
                char *fileType = getType(d_type);
                // printf("%ld %s %s\n", dirEntry->d_ino, fileType, dirEntry->d_name);
                if (isNumber(dirEntry->d_name))
                {
                    processes[numProcessesFound++] = populateRow(dirEntry);
                }
                i += dirEntry->d_reclen;
            }
            numEntries = syscall(SYS_getdents, procDirFd, getdentsBuffer, GETDENTS_BUFFER_SIZE);
        }
    }
    for (int i = 0; i < numProcessesFound; i++)
    {
        int newEntries = readFileDescriptors(processes[i], fds, numFdsFound);
        numFdsFound += newEntries;
    }
    print_processes(processes, numProcessesFound);
    print_fds(fds, numFdsFound);
}