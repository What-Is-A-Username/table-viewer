tableViewer: stringUtils.o printTables.o readFileDescriptors.o readProcesses.o main.o
	gcc main.o printTables.o readFileDescriptors.o readProcesses.o stringUtils.o -o tableViewer

%.o: %.c
	gcc -c -o $@ $< -Wall

.PHONY: clean

clean:
	rm -f printTables.o processes.o readFileDescriptors.o readProcesses.o stringUtils.o main.o

.PHONY: cleandist

cleandist:
	rm -f printTables.o processes.o readFileDescriptors.o readProcesses.o stringUtils.o main.o tableViewer

.PHONY: help

help:
	@echo "makefile rules available:"
	@echo "\ttableViewer:\tcreate the ./tableViewer executable, using the makefile to direct compiling and linking."
	@echo "\t<file>.o\tRecompile object file from c files, if necessary. This should never be used in a typical installation."
	@echo "\tclean:\t\tremove all object files from the project directory."
	@echo "\tcleandist:\tremove all object files and the executable from the project directory."
	@echo "\thelp:\t\tdisplay this help message"