#LINK=-lljacklm -lLabJackM -lm
LINK=-lLabJackM -lm

# The LCONFIG object file
lconfig.o: lconfig.c lconfig.h
	gcc -c lconfig.c

# The Binaries...
#
test.bin: lconfig.o test.c
	gcc test.c lconfig.o $(LINK) -o test.bin

ivchar.bin: lconfig.o ivchar.c
	gcc lconfig.o ivchar.c $(LINK) -o ivchar.bin
	chmod +x ivchar.bin

drun.bin: lconfig.o drun.c
	gcc lconfig.o drun.c $(LINK) -o drun.bin

