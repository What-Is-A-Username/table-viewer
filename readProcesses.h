#ifndef READ_PROCESSES_H
#define READ_PROCESSES_H

#include "processes.h"
#include <stddef.h>

extern void freeProcesses(ProcessData** processes, int size);

extern ProcessData *readProcess(linux_dirent *source);

extern ProcessData **fetchProcesses(int *size, long processIdSelected);

#endif