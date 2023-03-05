#ifndef PRINT_TABLES_H
#define PRINT_TABLES_H

#include <stdio.h>
#include "processes.h"

extern void print_systemWide_header(FILE *stream);

extern void print_systemWide_footer(FILE *stream);

extern void print_systemWide_content(ProcessData *process, FILE *stream);

extern void print_perProcess_header(FILE *stream);

extern void print_perProcess_footer(FILE *stream);

extern void print_perProcess_content(ProcessData *process, FILE *stream);

extern void print_vnodes_header(FILE *stream);

extern void print_vnodes_footer(FILE *stream);

extern void print_vnodes_content(ProcessData *process, FILE *stream);

extern void print_composite_header(FILE *stream);

extern void print_composite_footer(FILE *stream);

extern void print_composite_content(ProcessData *process, FILE *stream);

extern void print_table(void (*print_header)(FILE *),
                        void (*print_content)(ProcessData *, FILE *),
                        void (*print_footer)(FILE *),
                        ProcessData **processes,
                        size_t numProcesses,
                        FILE *stream);

extern int print_composite_binary(char* fileName, ProcessData **processes, int numProcesses);

#endif