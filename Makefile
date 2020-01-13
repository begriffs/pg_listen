CFLAGS = -std=c99 -Wpedantic -D_POSIX_C_SOURCE=200809L -Wall -Wextra `pkg-config --cflags libpq`
LDFLAGS = `pkg-config --libs libpq`

default: pg_listen

pg_listen: log.o

log.o: log.c log.h
	cc $(CFLAGS) -c log.c

clean:
	rm pg_listen
	rm log.o
