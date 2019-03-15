TODIR=/usr/local/bin
GASMON="$(TODIR)/gasmon"

#LINK=-lljacklm -lLabJackM -lm
LINK=-lm

# The Binaries...
#
gasmon.bin: gasmon.c ldisplay.h lgas.h
	gcc -Wall gasmon.c -lljacklm -o gasmon.bin
	chmod +x gasmon.bin

clean:
	rm -f *.o
	rm -f *.bin

install: gasmon.bin
	cp -f gasmon.bin $(GASMON)
	chmod 755 $(GASMON)
