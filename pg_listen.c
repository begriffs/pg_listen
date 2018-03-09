#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libpq-fe.h>
#include <poll.h>

void		listen_forever(PGconn *, char *);
void		clean_and_die(PGconn *);

int
main(int argc, char **argv)
{
	PGconn	   *conn;
	char	   *chan;

	if (argc != 3)
	{
		fprintf(stderr, "USAGE: %s db-url channel\n", argv[0]);
		return 1;
	}

	conn = PQconnectdb(argv[1]);
	if (PQstatus(conn) != CONNECTION_OK)
	{
		fputs(PQerrorMessage(conn), stderr);
		clean_and_die(conn);
	}

	chan = PQescapeIdentifier(conn, argv[2], 512);
	if (chan == NULL)
	{
		fputs(PQerrorMessage(conn), stderr);
		clean_and_die(conn);
	}
	listen_forever(conn, chan);

	/* should never get here */
	PQfreemem(chan);
	PQfinish(conn);
	return 0;
}

void
listen_forever(PGconn *conn, char *chan)
{
	PGnotify   *notify;
	PGresult   *res;
	int			nready,
				sock,
				chansz;
	char	   *cmd;
	struct pollfd pfd[1];

	chansz = strlen(chan);
	cmd = malloc((7 + chansz) * sizeof(char));
	if (cmd == NULL)
	{
		fputs("Failed to allocate memory for sql", stderr);
		clean_and_die(conn);
	}
	snprintf(cmd, 7 + chansz, "LISTEN %s", chan);

	res = PQexec(conn, cmd);
	if (PQresultStatus(res) != PGRES_COMMAND_OK)
	{
		fprintf(stderr, "LISTEN command failed: %s", PQerrorMessage(conn));
		PQclear(res);
		clean_and_die(conn);
	}
	PQclear(res);

	while (1)
	{
		sock = PQsocket(conn);

		if (sock < 0)
		{
			fprintf(stderr, "Failed to get libpq socket\n");
			clean_and_die(conn);
		}

		/* wait for input that may have been caused */
		/* by a NOTIFY event */
		pfd[0].fd = sock;
		pfd[0].events = POLLIN;
		nready = poll(pfd, 1, -1);

		PQconsumeInput(conn);
		while ((notify = PQnotifies(conn)) != NULL)
		{
			if (strcmp(notify->relname, chan) == 0)
			{
				printf("Got it\n");
			}
			PQfreemem(notify);
		}
	}
}

void
clean_and_die(PGconn *conn)
{
	PQfinish(conn);
	exit(1);
}
