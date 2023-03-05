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
