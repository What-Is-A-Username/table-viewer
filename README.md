# OS Table Viewer

This is a Linux-based tool developed with C which displays to the user information about active files, file descriptors and processes on the machine.

The tool only considers processes owned by the user. This is determined by comparing the ownership of the corresponding `/proc/<pid>/` folder to the real user ID of the caller ([getuid()](https://man7.org/linux/man-pages/man2/getuid.2.html))

- [Installation](#installation)
- [Quickstart](#quickstart)
- [Docs](#docs)
- [Makefile](#make)
- [Runtime Comparison](#runtime-comparison)

## Installation

Download or clone the repository:
```
git clone https://github.com/What-Is-A-Username/table-viewer.git
cd table-viewer
```

Installation is done with a single command using the Makefile.
```
make tableViewer
```

Now it can be run:
```
./tableViewer
```

There is also a second program included (`binRead`) which provides an example of how to read the binary output. However, since this falls outside the original project specification, I make no guarantees for its functionality and expect this to be ignored for testing.
```
make binRead
./binRead
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

You can set the program to flag processes possessing a high number of file descriptors.
```
./tableViewer --threshold=10
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
-   for directories, regular files, block devices, and character devices, the inode displayed is the inode of that file path, as determined by [`lstat`](https://man7.org/linux/man-pages/man2/lstat.2.html). If `lstat` errors, then the inode reverts to the inode of the process in `/proc/<pid>`
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

### All processes
To test the speed of plain-text output and binary output, the program was run with the time command in the shell using the commands below. Plain-text and binary output was each performed for 10 runs/repetitions.

**All results are expressed in seconds.** Additionally, the file size is expressed in bytes, as read from `ls -l`.

We find that the total time (the "real" time) needed to run the program was identical and the total kernel time (the "sys" time) was also identical. The only difference in times was between "user" times, with the binary output being slightly faster (0.0012 seconds versus 0.002 seconds, on average). 

```
# plain-text
time ./tableViewer --output_TXT
# binary
time ./tableViewer --output_binary
```

Plain-text results:
```   
run #   1     2     3     4     5     6     7     8     9     10
real	0.048 0.052 0.049 0.048 0.044 0.044 0.048 0.051 0.047 0.048
user	0.005 0.003 0.004 0.000 0.000 0.000 0.000 0.008 0.000 0.000
sys	0.006 0.010 0.008 0.012 0.009 0.010 0.011 0.005 0.011 0.012

average real = 0.0479
average user = 0.002
average sys = 0.0094
file size = 10996
```

Binary results:
```
run #   1     2     3     4     5     6     7     8     9     10
real 	0.046 0.045 0.046 0.045 0.044 0.046 0.043 0.048 0.054 0.050
user	0.003 0.005 0.000 0.000 0.000 0.000 0.000 0.000 0.004 0.000
sys	0.009 0.005 0.010 0.010 0.009 0.012 0.008 0.011 0.009 0.011

average real = 0.0467
average user = 0.0012
average sys = 0.0094
file size = 12714
```

### A single process

The commands were modified to time the speed when only one process was considered. The process used was specifically chosen because of its moderate number of file descriptors (10), which was neither too high or too low compared to all other processes running on the machine.

Again, each format was repeated for 10 repetitions to gather a sufficient sample size.

```
# plain-text
time ./tableViewer 26732 --output_TXT
# binary
time ./tableViewer 26732 --output_binary
```

Plain-text results:
```
real	0.012 0.011 0.012 0.013 0.011 0.013 0.011 0.013 0.014 0.011
user 	0.000 0.000 0.000 0.000 0.000 0.000 0.004 0.000 0.000 0.000
sys	0.005 0.005 0.004 0.005 0.004 0.005 0.000 0.005 0.005 0.005

real = 0.0121
user = 0.0004
sys = 0.0043
file size: 426
```

Binary results: 
```
real	0.011 0.013 0.014 0.012 0.011 0.011 0.012 0.009 0.010 0.012
user	0.000 0.000 0.000 0.000 0.004 0.000 0.002 0.000 0.000 0.005
sys	0.005 0.005 0.006 0.005 0.000 0.004 0.003 0.003 0.005 0.000

average real = 0.0115
average user = 0.0011
average sys = 0.0036
file size: 413
```

### Conclusions

We find that the "real" and "sys" times were highly similar when printing to binary and plain-text files, both when multiple processes were considered and when a single process was considered.  We also find that binary and plain-text file sizes were highly similar when printing a single process (413 vs 426). When printing multiple processes, the binary file was actually longer than the plain-text (12714 vs 10996).

If we consider the "sys" times, then the time taken by the system to write the binary and plain-text outputs to the filesystem was identical when printing all processes. This can be explained by two reasons. Firstly, a large portion of the output is actually alphabetic output (filenames), which is stored in practically identical ways in both plain text and binary files. These strings would not be printed appreciably differently between binary and plain-text output. Secondly, a major benefit of binary only arises when we store numbers greater than 4-digits long (because space for one 32-bit int = 4 8-bit chars). It is inefficient for numbers less than 4-digits long. Given the rare occurrence of large numbers in the data, this advantage to binary never materialized in a significant way. In fact, based on the fact that the binary file was longer than the plain-text file (12714 vs 10996), the storage of smaller numbers as integers likely contributed to making the binary format less space efficient.

If we consider "user" times, we see that binary output is faster than plain-text (0.0012 vs 0.002 seconds) when printing all processes. Since "user" time refers to CPU time used in running the code, I believe that this difference comes down to the implementation-specific details of the code. Printing of the table to plain-text requires multiple calls to print_table (one for each process), which may be computationally intensive due to the fact that the call stack is being frequently modified with new calls. This possible explanation is also consistent with the fact that binary "user" time was actually much longer than the plain-text "user" time (0.011 vs 0.0004); printing a single process' data significantly reduces the number of calls required to print the table to plain-text.

Finally, the "real" time, which reflects the total wall time spent from start to finish, indicates there is negligible difference between binary and text outputs from a practical standpoint. The "real" time includes time when the system chooses to compute other processes by blocking this program's process. Given the relatively small size of the output files being handled, speed differences between format may have been masked by natural fluctuations in CPU and memory utilization due to other users and system background processes.

In conclusion, it was found that binary and plain-text formats for the tested output were highly similar in their execution time. I suggest that implementation-specific details can be a significant determinant of how the "user" time changes in relation to the number of processes printed. If a much larger composite table was tested here (thousands of rows, multiple megabytes of data, etc.), then we can more accurately make an assessment of execution time and file size differences between plain-text and binary formats.
