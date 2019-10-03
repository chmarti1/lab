BIN=/usr/bin
INITXY=$(BIN)/initxy
MOVEXY=$(BIN)/movexy
MOVEX=$(BIN)/movex
MOVEY=$(BIN)/movey

movex: movex.c lconfig.h lconfig.o
	gcc lconfig.o -lLabJackM -lm movex.c -o movex
	chmod +x movex

movey: movey.c lconfig.h lconfig.o
	gcc lconfig.o -lLabJackM -lm movey.c -o movey
	chmod +x movey

movexy: movexy.c lconfig.h lconfig.o
	gcc lconfig.o -lLabJackM -lm movexy.c -o movexy
	chmod +x movexy

initxy: initxy.c lconfig.h lconfig.o
	gcc lconfig.o -lLabJackM -lm initxy.c -o initxy
	chmod +x initxy

install: initxy movexy movex movey
	cp initxy $(INITXY)
	chmod 755 $(INITXY)
	cp movexy $(MOVEXY)
	chmod 755 $(MOVEXY)
	cp movex $(MOVEX)
	chmod 755 $(MOVEX)
	cp movey $(MOVEY)
	chmod 755 $(MOVEY)

uninstall:
	rm -f $(INITXY)
	rm -f $(MOVEXY)
	rm -f $(MOVEX)
	rm -f $(MOVEY)

