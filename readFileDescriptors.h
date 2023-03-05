#ifndef READ_FILE_DESCRIPTORS_H
#define READ_FILE_DESCRIPTORS_H

#include <stdlib.h>
#include <sys/syscall.h>
#include <fcntl.h>
#include <dirent.h>

#include "processes.h"

extern FileDescriptorEntry *readFileDescriptor(ProcessData *process, linux_dirent *fileEntry, char *folderPath);

extern int readFileDescriptors(ProcessData *process);

#endif