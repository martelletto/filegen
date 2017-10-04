CFLAGS+= -Wall -W -Wshadow -Wwrite-strings -std=c99 -pedantic-errors

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

filegen: strtonum.o filegen.o
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm -f filegen *.o

all: filegen
