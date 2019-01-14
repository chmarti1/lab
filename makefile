#LINK=-lljacklm -lLabJackM -lm
LINK=-lLabJackM -lm

# The LCONFIG object file
lconfig.o: lconfig.c lconfig.h
	gcc -c lconfig.c -o lconfig.o

# The Binaries...
#
monitor.bin: monitor.c ldisplay.h lconfig.o lgas.h psat.h
	gcc lconfig.o monitor.c -lljacklm $(LINK) -o monitor.bin

test.bin: lconfig.o test.c
	gcc test.c lconfig.o $(LINK) -o test.bin

ivchar.bin: lconfig.o ivchar.c
	gcc lconfig.o ivchar.c $(LINK) -o ivchar.bin
	chmod +x ivchar.bin

drun: lconfig.o drun.c
	gcc lconfig.o drun.c $(LINK) -o drun
	chmod +x drun

dburst: lconfig.o dburst.c
	gcc lconfig.o dburst.c $(LINK) -o dburst
	chmod +x dburst

gasmon: gasmon.c ldisplay.h lgas.h
	gcc -Wall gasmon.c -lljacklm -o gasmon
	chmod +x gasmon

test: lconfig.o test.c
	gcc lconfig.o test.c $(LINK) -o test
	chmod +x test
