.POSIX :

.SUFFIXES :
.SUFFIXES : .c

CFLAGS = -std=c89 -Wpedantic -D_POSIX_C_SOURCE=200112L -Wall -Wextra

include config.mk

default : pg_listen

clean :
	rm pg_listen
