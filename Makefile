LDLIBS = -lX11 -lXtst -lXi
CFLAGS = -Wall -O2

clicker: clicker.c cmdline.c
	gcc $(CFLAGS) $^ $(LDLIBS) -o $@

cmdline.c: clicker.ggo
	gengetopt < $<

.PHONY: clean

clean:
	rm -f clicker cmdline.c cmdline.h

