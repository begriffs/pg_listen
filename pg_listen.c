#include <libpq-fe.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void		listen_forever(PGconn *, char *, char *);
int			reset_if_necessary(PGconn *);
void		clean_and_die(PGconn *);
void		begin_listen(PGconn *, char *);

const int	BUFSZ = 512;

int
main(int argc, char **argv)
{
	PGconn	   *conn;
	char	   *chan;

	if (argc != 4)
	{
		fprintf(stderr, "USAGE: %s db-url channel shell-command\n", argv[0]);
		return 1;
	}

	conn = PQconnectdb(argv[1]);
	if (PQstatus(conn) != CONNECTION_OK)
	{
		fputs(PQerrorMessage(conn), stderr);
		clean_and_die(conn);
	}

	chan = PQescapeIdentifier(conn, argv[2], BUFSZ);
	if (chan == NULL)
	{
		fputs(PQerrorMessage(conn), stderr);
		clean_and_die(conn);
	}
	listen_forever(conn, chan, argv[3]);

	/* should never get here */
	PQfreemem(chan);
	PQfinish(conn);
	return 0;
}

void
listen_forever(PGconn *conn, char *chan, char *cmd)
{
	PGnotify   *notify;
	int			sock;
	pid_t		pid;
	struct pollfd pfd[1];

	printf("Listening for channel %s\n", chan);
	begin_listen(conn, chan);

	while (1)
	{
		if (reset_if_necessary(conn))
			begin_listen(conn, chan);

		sock = PQsocket(conn);
		if (sock < 0)
		{
			fprintf(stderr, "Failed to get libpq socket\n");
			clean_and_die(conn);
		}

		pfd[0].fd = sock;
		pfd[0].events = POLLIN;
		if (poll(pfd, 1, -1) < 0)
		{
			fprintf(stderr, "Poll() error\n");
			clean_and_die(conn);
		}

		PQconsumeInput(conn);
		while ((notify = PQnotifies(conn)) != NULL)
		{
			pid = fork();
			if (pid == 0)
				exit(system(cmd));

			PQfreemem(notify);
		}
	}
}

int
reset_if_necessary(PGconn *conn)
{
	unsigned int seconds = 0;

	if (PQstatus(conn) == CONNECTION_OK)
		return 0;

	do
	{
		if (seconds == 0)
			seconds = 1;
		else
		{
			printf("Failed.\nSleeping %d seconds.\n", seconds);
			sleep(seconds);
			seconds *= 2;
		}
		printf("Reconnecting to database...");
		PQreset(conn);
	} while (PQstatus(conn) != CONNECTION_OK);

	printf("Connected.\n");
	return 1;
}

void
begin_listen(PGconn *conn, char *chan)
{
	PGresult   *res;
	char		sql[7 + BUFSZ + 1];

	snprintf(sql, 7 + BUFSZ + 1, "LISTEN %s", chan);
	res = PQexec(conn, sql);

	if (PQresultStatus(res) != PGRES_COMMAND_OK)
	{
		fprintf(stderr, "LISTEN command failed: %s", PQerrorMessage(conn));
		PQclear(res);
		clean_and_die(conn);
	}
	PQclear(res);
}

void
clean_and_die(PGconn *conn)
{
	PQfinish(conn);
	exit(1);
}
