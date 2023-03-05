# OS Table Viewer

This is a Linux-based tool developed with C which displays to the user information about active files, file descriptors and processes on the machine.

The tool only considers processes owned by the user. This is determined by comparing the ownership of the corresponding `/proc/<pid>/` folder 

- [Installation](#--installation)
- [Quickstart](#--Quickstart)
- [Docs](#--Docs)
- [Runtime Comparison](#--Runtime-Comparison)

## Installation

Installation is done with a single command.
```
make tableViewer
```

## Quickstart

The program can be run with no arguments. The default behavior is to show the [composite file descriptor table](#--composite). 
```
./tableViewer
```

To disable default behavior and customize the tables shown, you can select any number of the `--per-process`, `--Vnodes`, `--composite` and `--systemWide` commands.
```
# Only show the process FD table
./tableViewer --per-process
# Show both the composite and Vnode tables
./tableViewer --composite --Vnodes
```

The results can be output to plain text.
```
./tableViewer --output_TXT
```

## Docs

### --per-process

Display a "process file descriptor" table, with columns for process ID (PID) and file descriptor.

Example Input:
```
./tableViewer --per-process
```
Example Output:
```
PID     FD
===================
1990    0
1990    1
1990    2
1990    3
1990    4
1990    5
1990    6
1990    7
1990    8
1990    9
1990    10
1990    11
1990    12
1990    13
1990    14
1990    15
1990    16
1990    17
1990    18
1990    19
1990    22
1990    23
1990    26
1990    27
1990    29
2024    0
2024    1
2024    2
===================
```
### --systemWide

Display a "system-wide file descriptor" table, with columns for process ID (PID), file descriptor and filename.

Example Input:
```
./tableViewer --systemWide
```
Example Output:
```
PID     FD      Filename
===============================
1990    0       socket:[29715]  29715
1990    1       socket:[29717]  29717
1990    2       socket:[29719]  29719
1990    3       socket:[29721]  29721
1990    4       anon_inode:[eventpoll]  28548
1990    5       pipe:[28545]    28545
1990    6       pipe:[28545]    28545
1990    7       pipe:[28546]    28546
1990    8       pipe:[28546]    28546
1990    9       anon_inode:[eventfd]    28548
1990    10      anon_inode:[eventpoll]  28548
1990    11      pipe:[19989]    19989
1990    12      pipe:[19989]    19989
1990    13      anon_inode:[eventfd]    28548
1990    14      anon_inode:[eventpoll]  28548
1990    15      pipe:[28547]    28547
1990    16      pipe:[28547]    28547
1990    17      anon_inode:[eventfd]    28548
1990    18      /dev/null       3
1990    19      /home/user/server/temp/remoteagent.log      470220
1990    22      /home/user/server/temp/exthost1/exthost.log 532642
1990    23      /home/user/server/temp/exthost1/1-compiler.log    379507
1990    26      /home/user/server/temp/exthost1/2-ui.log  289174
1990    27      /home/user/server/temp/exthost1/3-errors.log    289181
1990    29      /home/user/server/temp/exthost1/4-tools.log 384044
2024    0       /dev/pts/3
2024    1       /dev/pts/3
2024    2       /dev/pts/3
===============================
```
### --Vnodes

Display a "Vnodes file descriptor" table, with columns for process ID (PID) and inode.

Example Input:
```
./tableViewer --Vnodes
```
Example Output:
```
PID     inode
===================
1990    29715
1990    29717
1990    29719
1990    29721
1990    28548
1990    28545
1990    28545
1990    28546
1990    28546
1990    28548
1990    28548
1990    19989
1990    19989
1990    28548
1990    28548
1990    28547
1990    28547
1990    28548
1990    3
1990    470220
1990    532642
1990    379507
1990    289174
1990    289181
1990    384044
2024    6
2024    6
2024    6
===================
```

### --composite

Display a "composite file descriptor" table, with columns for process ID (PID), file descriptor, filename and inode. The composite table thereby combines all data obtainable from the process file, system-wide and Vnodes file descriptor tables.

The number on the left hand side denotes its own file descriptor count within the same process, counting up from 1.

Example Input:
```
./tableViewer --composite
```
Example Output:
```
        PID     FD      filename        inode
        =======================================
1       1990    0       socket:[29715]  29715
2       1990    1       socket:[29717]  29717
3       1990    2       socket:[29719]  29719
4       1990    3       socket:[29721]  29721
5       1990    4       anon_inode:[eventpoll]  28548
6       1990    5       pipe:[28545]    28545
7       1990    6       pipe:[28545]    28545
8       1990    7       pipe:[28546]    28546
9       1990    8       pipe:[28546]    28546
10      1990    9       anon_inode:[eventfd]    28548
11      1990    10      anon_inode:[eventpoll]  28548
12      1990    11      pipe:[19989]    19989
13      1990    12      pipe:[19989]    19989
14      1990    13      anon_inode:[eventfd]    28548
15      1990    14      anon_inode:[eventpoll]  28548
16      1990    15      pipe:[28547]    28547
17      1990    16      pipe:[28547]    28547
18      1990    17      anon_inode:[eventfd]    28548
19      1990    18      /dev/null       3
20      1990    19      /home/user/server/temp/remoteagent.log      470220
21      1990    22      /home/user/server/temp/exthost1/exthost.log 532642
22      1990    23      /home/user/server/temp/exthost1/1-compiler.log    379507
23      1990    26      /home/user/server/temp/exthost1/2-ui.log  289174
24      1990    27      /home/user/server/temp/exthost1/3-errors.log    289181
25      1990    29      /home/user/server/temp/exthost1/4-tools.log 384044
1       2024    0       /dev/pts/3      6
2       2024    1       /dev/pts/3      6
3       2024    2       /dev/pts/3      6
        =======================================
```

### --threshold=X

Set the file descriptor threshold to a non-negative long integer X, and use this to filter offending processes. When set, the last output line will be a list of processes which more file descriptors than the threshold value. If `--threshold=X` is never used, then no such list is  printed.

Example Input 1:
```
./tableViewer --threshold=24
```
Example Output 1:
(composite table output is omitted)
```
...
## Offending processes:
1990 (25)
```

Example Input 2:
```
./tableViewer --threshold=25
```
Example Output 1:
(composite table output is omitted)
```
...
## Offending processes:
None!
```


### --output_TXT

Output the [composite file descriptor table](#--composite) in plain-text (ASCII characters) to a file named `compositeTable.txt`.

### --output_binary

Output process and file descriptor data needed to construct the [composite file descriptor table](#--composite) in binary to a file named `compositeTable.bin`.

Data in the binary file is written by storing all the processes one-by-one until the end of file. Each process will start with an unsigned long representing the process ID (PID) and another unsigned long representing the number of file descriptors. Then, each one of its file descriptors is stored in binary using the struct [`fileDescriptorEntry` described in processes.h](./processes.h)

## Inodes

The value displayed in the inode column will depend on the file descriptor's content.

-   for FIFO/pipes and sockets, the inode displayed is the inode number as it appears between the brackets `[<inode>]` in the filename.
-   for directories, regular files, and character devices, the inode displayed is the inode of that file path, as determined by [`lstat`](https://man7.org/linux/man-pages/man2/lstat.2.html). If `lstat` errors, then the inode reverts to the inode of the process in `/proc/<pid>`
-   the default value for all other file descriptors, the inode displayed is the inode of process itself in `/proc/<pid>`.

## Make 

The included makefile in the project directory several rules, visible when running `make help`:

```
makefile rules available:
    tableViewer:    create the ./tableViewer executable, using the makefile to direct compiling and linking.
    <file>.o        Recompile object file from c files, if necessary. This should never be used in a typical installation.
    clean:          remove all object files from the project directory.
    cleandist:      remove all object files and the executable from the project directory.
    help:           display this help message
```

## Runtime Comparison

Here, we compare the time needed to run various commands and compare the time required to perform binary and plain-text (ASCII) output.

### Printing all PIDs
To test the speed of plain-text output and binary output, the program was run with the time command in the shell using the commands below. Plain-text and binary output was each performed for 10 runs/repetitions.
```
# plain-text
time ./tableViewer --output_TXT
# binary
time ./tableViewer --output_binary
```

Binary results:
```
run     1        2        3        4        5        6        7        8        9        10
real    0m0.010s 0m0.009s 0m0.013s 0m0.008s 0m0.008s 0m0.008s 0m0.009s 0m0.010s 0m0.007s 0m0.008s
user    0m0.001s 0m0.007s 0m0.000s 0m0.000s 0m0.006s 0m0.000s 0m0.000s 0m0.000s 0m0.000s 0m0.000s
sys     0m0.008s 0m0.000s 0m0.011s 0m0.005s 0m0.001s 0m0.007s 0m0.007s 0m0.006s 0m0.006s 0m0.006s
```

Averages: real = 0.009s, user = 0.0014s, sys = 0.0057

Plain-text
```   
runs    1        2        3        4        5        6        7        8        9        10
real    0m0.011s 0m0.008s 0m0.009s 0m0.007s 0m0.010s 0m0.007s 0m0.012s 0m0.008s 0m0.010s 0m0.008s
user    0m0.009s 0m0.005s 0m0.007s 0m0.006s 0m0.009s 0m0.005s 0m0.000s 0m0.006s 0m0.000s 0m0.000s
sys     0m0.000s 0m0.000s 0m0.000s 0m0.000s 0m0.000s 0m0.000s 0m0.010s 0m0.000s 0m0.008s 0m0.007s

Averages: real = 0.0091s, user = 0.0047s, sys = 0.0025

### A single PID

