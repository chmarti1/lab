ETC=/etc/lconfig
INSTALL=/usr/local/bin
BIN=bin
INITXY=$(BIN)/initxy
MOVEXY=$(BIN)/movexy
MOVEX=$(BIN)/movex
MOVEY=$(BIN)/movey
LCONFIG=lconfig/lconfig.o

$(MOVEX): smiface.h movex.c $(LCONFIG)
	gcc $(LCONFIG) -lLabJackM -lm movex.c -o $(MOVEX)

$(MOVEY): smiface.h movey.c $(LCONFIG)
	gcc $(LCONFIG) -lLabJackM -lm movey.c -o $(MOVEY)

$(MOVEXY): smiface.h movexy.c $(LCONFIG)
	gcc $(LCONFIG) -lLabJackM -lm movexy.c -o $(MOVEXY)

$(INITXY): smiface.h initxy.c $(LCONFIG)
	gcc $(LCONFIG) -lLabJackM -lm initxy.c -o $(INITXY)

install: $(INITXY) $(MOVEXY) $(MOVEX) $(MOVEY) move.conf
	chown root:root $(BIN)/*
	chmod 755 $(BIN)/*
	cp $(BIN)/* $(INSTALL)/
	mkdir -p $(ETC)
	cp move.conf $(ETC)/move.conf
	chown root:root $(ETC)/move.conf
	chmod 644 $(ETC)/move.conf

uninstall:
	rm -f $(INSTALL)/initxy
	rm -f $(INSTALL)/movexy
	rm -f $(INSTALL)/movex
	rm -f $(INSTALL)/movey
	rm -f $(ETC)/move.conf

clean:
	rm -fv $(BIN)/*
