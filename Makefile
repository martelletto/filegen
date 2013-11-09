CFLAGS+= -Wall -W -Wshadow -Wwrite-strings -std=c99 -pedantic-errors

filegen: filegen.c
	$(CC) $(CFLAGS) filegen.c -o $(.TARGET)

clean:
	rm -f filegen

all: filegen

.include <bsd.prog.mk>
