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

#define MAX_COMMAND_LINE_ARGUMENT_LENGTH 64

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
typedef struct dataRow
{
    unsigned long pid;
    unsigned long fd;
    unsigned long inode;
    char filename[SYMBOLIC_LINK_BUFFER_SIZE];
} dataRow;

/**
 * Describes information in a row of the Vnodes table 
*/
typedef struct processData
{
    unsigned long pid;
    unsigned long inode;
} processData;

/**
 * Check whether the given string is a decimal number.
 * @param checkString String to check
 * @return Returns true if checkString only contains decimal digits, false otherwise.
*/
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

/**
 * Populate a new row with inode and pid data given the information from getdents.
 * @param source A pointer to the information retrieved by getdents
 * @return A dynamically-allocated processData struct with inode and PID populated from source.
*/
processData *populateRow(linux_dirent *source)
{
    processData *result = (processData *)malloc(sizeof(dataRow));
    result->inode = source->d_ino;
    result->pid = strtoul(source->d_name, NULL, 10);
    return result;
};

// TODO: Remove this function if no longer in use
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

    // buffer to parse inode from pipes and sockets
    char inodeString[SYMBOLIC_LINK_BUFFER_SIZE];
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
                aggregate[nextIndex++] = newRow;
            }
            i += fileEntry->d_reclen;
        }
        numEntries = syscall(SYS_getdents, procDirFd, entBuffer, GETDENTS_BUFFER_SIZE);
    }
    return nextIndex - startingSize;
}

/**
 * Print the Vnodes file descriptor table
 * @param rows Information for each row to print
 * @param size The number of elements in rows to print
*/
void print_processes(processData **rows, int size)
{
    printf("PID\tinode\n");
    printf("===================");
    for (int i = 0; i < size; i++)
    {
        printf("%ld\t%ld\n", rows[i]->pid, rows[i]->inode);
    }
    return;
}

/**
 * Print the composite table 
 * @param rows Information for each row to print
 * @param size The number of elements in rows to print
*/
void print_fds(dataRow **rows, int size)
{
    printf("\tPID\tFD\tfilename\tinode\n");
    printf("\t==============================");
    for (int i = 0; i < size; i++)
    {
        printf("%d\t%ld\t%ld\t%s\t%ld\n", i, rows[i]->pid, rows[i]->fd, rows[i]->filename, rows[i]->inode);
    }
    return;
}

#define ARG_PER_PROCESS "--per-process"
#define ARG_SYSTEM_WIDE "--systemWide"
#define ARG_VNODES "--Vnodes"
#define ARG_COMPOSITE "--composite"
#define ARG_THRESHOLD "--threshold"

/**
 * Check if a substring exists in a string.
 * @param haystack string to search in
 * @param needle substring to search for
 * @returns true is substring needle is in haystack, false otherwise
 */
bool startsWith(const char *haystack, const char *needle)
{
    return strstr(haystack, needle) == haystack;
}

/**
 * Print a standardized error to stderr to indicate to the user that the command arguments are incorrect.
*/
void notifyInvalidArguments() {
    fprintf(stderr, "Error: Command has invalid formatting and could not be parsed.\n");
}

/**
 * Parse an command argument key-value pair separated by an equal sign and store its result.
 * @param result Pointer to where the value will be assigned to
 * @param argv A string representing the command string and the value (e.g. "--samples=3")
 * @returns 0 if operation was successful, 1 otherwise
 */
int parseNumericalArgument(long *result, char *argv)
{
    char *splitToken = strtok(argv, "=");
    splitToken = strtok(NULL, "=");
    if (splitToken == NULL)
    {
        // failed to find a string after the = character
        notifyInvalidArguments();
        return 1;
    }
    long tempResult = atol(splitToken);
    if (tempResult == 0)
    {
        // failed to parse string to number
        notifyInvalidArguments();
        return 1;
    }
    *result = atol(splitToken);
    return 0;
}

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
    bool showComposite = true;

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
        else if (startsWith(argv[i], ARG_THRESHOLD))
        {
            if (parseNumericalArgument(&threshold, argv[i]) != 0)
            {
                return 1;
            }
        }
        else
        {
            if (pidSet)
            {
                fprintf(stderr, "Error: Invalid number of positional arguments given.\n");
                return 1;
            }
            else
            {
                pidArgument = atoi(argv[i]);
                if (pidArgument == 0)
                {
                    fprintf(stderr, "Error: Invalid positional argument given.\n");
                    return 1;
                }
                pidSet = true;
            }
            // parse positional argument
        }
    }

    printf("Arguments parsed: %s: %d, %s: %d, %s: %d, %s: %d, %s: %ld, %s: %ld\n", ARG_PER_PROCESS, showPerProcess, ARG_SYSTEM_WIDE, showSystemWide, ARG_VNODES, showVnodes, ARG_COMPOSITE, showComposite, ARG_THRESHOLD, threshold, "PID", pidArgument);

    return 0;

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