TODIR=/usr/local/bin
GASMON="$(TODIR)/gasmon"
DBURST="$(TODIR)/dburst"
DRUN="$(TODIR)/drun"

#LINK=-lljacklm -lLabJackM -lm
LINK=-lLabJackM -lm

# The LCONFIG object file
lconfig.o: lconfig.c lconfig.h
	gcc -c lconfig.c -o lconfig.o

# The Binaries...
#
monitor.bin: monitor.c ldisplay.h lconfig.o lgas.h psat.h
	gcc lconfig.o monitor.c -lljacklm $(LINK) -o monitor.bin

ivchar.bin: lconfig.o ivchar.c
	gcc lconfig.o ivchar.c $(LINK) -o ivchar.bin
	chmod +x ivchar.bin

drun.bin: lconfig.o drun.c
	gcc lconfig.o drun.c $(LINK) -o drun.bin
	chmod +x drun.bin

dburst.bin: lconfig.o dburst.c
	gcc lconfig.o dburst.c $(LINK) -o dburst.bin
	chmod +x dburst.bin

gasmon.bin: gasmon.c ldisplay.h lgas.h
	gcc -Wall gasmon.c -lljacklm -o gasmon.bin
	chmod +x gasmon.bin

clean:
	rm -f lconfig.o
	rm -f drun.bin dburst.bin gasmon.bin

install: gasmon.bin dburst.bin drun.bin
	cp -f gasmon.bin $(GASMON)
	chmod 755 $(GASMON)
	cp -f drun.bin $(DRUN)
	chmod 755 $(DRUN)
	cp -f dburst.bin $(DBURST)
	chmod 755 $(DBURST)
