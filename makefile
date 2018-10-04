#LINK=-lljacklm -lLabJackM -lm
LINK=-lLabJackM -lm

# The LCONFIG object file
lconfig.o: lconfig.c lconfig.h
	gcc -c lconfig.c -o lconfig.o

# The Binaries...
#
monitor.bin: lconfig.o monitor.c
	gcc monitor.c lconfig.o $(LINK) -o monitor.bin
	chmod +x monitor.bin

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

test: lconfig.o test.c
	gcc lconfig.o test.c $(LINK) -o test
	chmod +x test
