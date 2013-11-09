CFLAGS+= -Wall -W -Wshadow -Wwrite-strings -Wshorten-64-to-32 -std=c99
CFLAGS+= -pedantic-errors

filegen: filegen.c
	$(CC) $(CFLAGS) filegen.c -o $(.TARGET)

clean:
	rm -f filegen

all: filegen

.include <bsd.prog.mk>
