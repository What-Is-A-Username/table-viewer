#ifndef READ_PROCESSES_H
#define READ_PROCESSES_H

#include "processes.h"

extern ProcessData *readProcess(linux_dirent *source);

extern ProcessData **fetchProcesses(int *size, long processIdSelected);

#endif