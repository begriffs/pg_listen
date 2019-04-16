CC = clang
CFLAGS = -Wall -Wextra `pkg-config --cflags libpq`
LDFLAGS = `pkg-config --libs libpq`

default: pg_listen

clean:
	rm pg_listen
