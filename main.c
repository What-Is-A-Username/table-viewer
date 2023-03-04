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
ProcessData *readProcess(linux_dirent *source)
{
    ProcessData *result = (ProcessData *)malloc(sizeof(ProcessData));
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
        fprintf(stream, "%d\t%ld\t%ld\t%s\t%ld\n", i, process->pid, process->fileDescriptors[i]->fd, process->fileDescriptors[i]->filename, process->fileDescriptors[i]->inode);
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

#define ARG_PER_PROCESS "--per-process"
#define ARG_SYSTEM_WIDE "--systemWide"
#define ARG_VNODES "--Vnodes"
#define ARG_COMPOSITE "--composite"
#define ARG_THRESHOLD "--threshold"
#define ARG_OUTPUT_BINARY "--output_binary"
#define ARG_OUTPUT_TXT "--output_TXT"

#define BINARY_OUT_NAME "compositeTable.bin"
#define TXT_OUT_NAME "compositeTable.txt"

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
void notifyInvalidArguments()
{
    fprintf(stderr, "Error: Command has invalid formatting and could not be parsed.\n");
}

/**
 * Parse an command argument key-value pair separated by an equal sign and store its result.
 * @param result Pointer to where the value will be assigned to
 * @param argv A string representing the command string and the value (e.g. "--samples=3")
 * @returns Returns 0 if operation was successful, 1 otherwise
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

/**
 * Print information of processes that have more file descriptors than the given threshold. These are "offending processes".
 * @param threshold The maximum number of file descriptors a process can have before it is printed.
 * @param processes An array of all processes to consider.
 * @param numProcesses The size of the processes array.
*/
void printOffendingProcesses(long threshold, ProcessData **processes, int numProcesses)
{
    bool foundFirstOffender = false;
    printf("## Offending processes:\n");
    for (int i = 0; i < numProcesses; i++)
    {
        if (processes[i]->size > threshold)
        {
            if (!foundFirstOffender)
            {
                foundFirstOffender = true;
            }
            else
            {
                printf(", ");
            }
            printf("%ld (%ld)", processes[i]->pid, processes[i]->size);
        }
    }
    if (!foundFirstOffender)
    {
        printf("None!");
    }
    printf("\n");
}

/**
 * Save composite table to binary file.
 * @param processes Structs containing all processes and file descriptors to output to binary
 * @param numProcesses The total number of processes to output
 * @return Returns 0 if operation was successful, nonzero otherwise
*/
int print_composite_binary(ProcessData **processes, int numProcesses) {
    FILE* binaryStream = fopen(BINARY_OUT_NAME, "wb");
    if (binaryStream == NULL) {
        perror("Error opening to .bin output file");
        return -1;
    }
    for (size_t i = 0; i < numProcesses; i++)
    {
        fwrite(&processes[i]->pid, sizeof(processes[i]->pid), 1, binaryStream);
        fwrite(&processes[i]->size, sizeof(processes[i]->size), 1, binaryStream);
        for (size_t j = 0; j < processes[i]->size; j++)
        {
            fwrite(processes[i]->fileDescriptors[j], sizeof(FileDescriptorEntry), 1, binaryStream);
        }
    }
    fclose(binaryStream);
    return 0;
}

/**
 * Entry point of program.
*/
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
    bool showComposite = false;

    /**
     * Print offending processes, defined by having more file descriptors than the number set in threshold.
    */
    bool thresholdSet = false;

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

    // TODO: Implement --output_TXT and --output_binary to save composite table to compositeTable.txt and compositeTable.bin respectively.

    /**
     * Output composite table to text?
     */
    bool outputTxt = false;

    /**
     * Output composite table to binary?
     */
    bool outputBinary = false;

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
        else if (strncmp(argv[i], ARG_OUTPUT_TXT, MAX_COMMAND_LINE_ARGUMENT_LENGTH) == 0)
        {
            outputTxt = true;
        }
        else if (strncmp(argv[i], ARG_OUTPUT_BINARY, MAX_COMMAND_LINE_ARGUMENT_LENGTH) == 0)
        {
            outputBinary = true;
        }
        else if (startsWith(argv[i], ARG_THRESHOLD))
        {
            if (parseNumericalArgument(&threshold, argv[i]) != 0)
            {
                return 1;
            }
            thresholdSet = true;
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
                pidArgument = atol(argv[i]);
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

    int numProcessesFound;
    ProcessData **processes = fetchProcesses(&numProcessesFound, pidArgument);

    for (int i = 0; i < numProcessesFound; i++)
    {
        int status = readFileDescriptors(processes[i]);
        if (status != 0)
        {
            return -1;
        }
    }

    if (showPerProcess)
    {
        print_table(print_perProcess_header, print_perProcess_content, print_perProcess_footer, processes, numProcessesFound, stdout);
    }

    if (showSystemWide)
    {
        print_table(print_systemWide_header, print_systemWide_content, print_systemWide_footer, processes, numProcessesFound, stdout);
    }

    if (showVnodes)
    {
        print_table(print_vnodes_header, print_vnodes_content, print_vnodes_footer, processes, numProcessesFound, stdout);
    }

    // show composite table if explicitly given in arguments, or if no table arguments were given
    // TODO: Should row numbers be added to all tables, not only composite? The two composite tables in the example have varied behavior for whether or not to print row numbers.
    if (showComposite || (!showPerProcess && !showSystemWide && !showVnodes && !showComposite))
    {
        print_table(print_composite_header, print_composite_content, print_composite_footer, processes, numProcessesFound, stdout);
    }

    if (outputTxt) {
        FILE* txtStream = fopen(TXT_OUT_NAME, "w");
        if (txtStream == NULL) {
            perror("Error opening to .txt output file");
            return -1;
        }
        print_table(print_composite_header, print_composite_content, print_composite_footer, processes, numProcessesFound, txtStream);
        fclose(txtStream);
    }

    else if (outputBinary) {
        print_composite_binary(processes, numProcessesFound);
    }

    if (thresholdSet) {
        printOffendingProcesses(threshold, processes, numProcessesFound);
    }

    // Free array memory used to store processes and file descriptors
    for (int i = 0; i < numProcessesFound; i++)
    {
        for (int fd = 0; fd < processes[i]->size; fd++)
        {
            free(processes[i]->fileDescriptors[fd]);
        }
        free(processes[i]);
    }

    return 0;
}