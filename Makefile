CFLAGS = -std=c89 -Wpedantic -D_POSIX_C_SOURCE=200809L -Wall -Wextra `pkg-config --cflags libpq`
LDFLAGS = `pkg-config --libs libpq`

default: pg_listen

clean:
	rm pg_listen
