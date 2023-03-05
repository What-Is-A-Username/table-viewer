# OS Table Viewer

This is a Linux-based tool developed with C which displays to the user information about active files, file descriptors and processes on the machine.

## Installation

Installation is done with a single command.
```
make tableViewer
```

## Docs

### --per-process

Display a "process file descriptor" table, with columns for process ID (PID) and file descriptor.

### --systemWide

Display a "system-wide file descriptor" table, with columns for process ID (PID), file descriptor and filename.

### --Vnodes

Display a "Vnodes file descriptor" table, with columns for process ID (PID) and inode.

### --composite

Display a "composite file descriptor" table, with columns for process ID (PID), file descriptor, filename and inode. The composite table thereby combines all data obtainable from the process file, system-wide and Vnodes file descriptor tables.

### --threshold=X

Set the file descriptor threshold to a non-negative long integer X, and use this to filter offending processes. When set, the last output line will be a list of processes which more file descriptors than the threshold value. If `--threshold=X` is never used, then no such list is  printed.

### --output_TXT

Output the [composite file descriptor table](#--composite) in plain-text (ASCII characters) to a file named `compositeTable.txt`.

### --output_binary

Output process and file descriptor data needed to construct the [composite file descriptor table](#--composite) in binary to a file named `compositeTable.bin`.

## Make 

Object files 


