#include <errno.h>
#include <libpq-fe.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "log.h"

void		listen_forever_print(PGconn *, const char *);
void		listen_forever_exec(PGconn *, const char *, const char *, char **);
void 		listen_poll(PGconn *, const char *);
int			reset_if_necessary(PGconn *);
void		clean_and_die(PGconn *);
void		begin_listen(PGconn *, const char *);

#define		BUFSZ 512

int
main(int argc, char **argv)
{
	PGconn     *conn;
	char       *chan;
	char       *prog;

	if (argc < 4)
	{
		log_info("USAGE: %s db-url channel /path/to/program [args]\n",
				argv[0]);
		return EXIT_FAILURE;
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

	prog = argv[3];
	if(*prog == '-')
	{	
		listen_forever_print(conn, chan);
	} 
	else 
	{
		listen_forever_exec(conn, chan, prog, argv+3);
	}

	/* should never get here */
	PQfreemem(chan);
	PQfinish(conn);
	return 0;
}

void
listen_forever_print(PGconn *conn, const char *chan)
{
	PGnotify   *notify;

	log_info("No executable given, sending notifications to stdout");

	begin_listen(conn, chan);

	while (1)
	{
		listen_poll(conn, chan);

		while ((notify = PQnotifies(conn)) != NULL)
		{
			log_info(
					"NOTIFY of '%s' received from backend PID %d",
					notify->relname, 
					notify->be_pid
			);

			printf("%s\n", notify->extra);

			PQfreemem(notify);
		}
	}
}

void
listen_forever_exec(PGconn *conn, const char *chan, const char *cmd, char **args)
{
	PGnotify   *notify;
	int			pipefds[2];

	begin_listen(conn, chan);

	while (1)
	{
		listen_poll(conn, chan);

		while ((notify = PQnotifies(conn)) != NULL)
		{
			log_info(
					"NOTIFY of '%s' received from backend PID %d",
					notify->relname, 
					notify->be_pid
			);
			/* we'll send NOTIFY payload through pipe to stdin */
			if (errno = 0, pipe(pipefds) < 0)
			{
				perror("pipe()");
				PQfreemem(notify);
				clean_and_die(conn);
			}

			switch (errno = 0, fork())
			{
				case -1:
					perror("fork()");
					PQfreemem(notify);
					close(pipefds[0]);
					close(pipefds[1]);
					clean_and_die(conn);
					break;
				case 0: /* Child - reads from pipe */
					/* Write end is unused */
					close(pipefds[1]);
					/* read from pipe as stdin */
					if (errno = 0, dup2(pipefds[0], STDIN_FILENO) < 0)
					{
						perror("Unable to assign stdin to pipe");
						close(pipefds[0]);
						exit(EXIT_FAILURE);
					}
					if (errno = 0, execv(cmd, args) < 0)
					{
						log_fatal("Can't run %s: %s",
						         cmd, strerror(errno));
						close(pipefds[0]);
						exit(EXIT_FAILURE);
					}
					/* should not get here */
					break;
				default: /* Parent - writes to pipe */
					close(pipefds[0]); /* Read end is unused */
					write(pipefds[1], notify->extra, strlen(notify->extra));
					close(pipefds[1]);
					break;
			}

			PQfreemem(notify);
		}
	}
}

void
listen_poll(PGconn *conn, const char *chan)
{
	int			sock;
	struct pollfd pfd[1];

	if (reset_if_necessary(conn))
		begin_listen(conn, chan);

	sock = PQsocket(conn);
	if (sock < 0)
	{
		log_fatal("Failed to get libpq socket: %s",
				PQerrorMessage(conn));
		clean_and_die(conn);
	}

	pfd[0].fd = sock;
	pfd[0].events = POLLIN;
	if (errno = 0, poll(pfd, 1, -1) < 0)
	{
		perror("poll()");
		clean_and_die(conn);
	}

	PQconsumeInput(conn);
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
			log_info("Failed.\nSleeping %d seconds.", seconds);
			sleep(seconds);
			seconds *= 2;
		}
		log_info("Reconnecting to database...");
		PQreset(conn);
	} while (PQstatus(conn) != CONNECTION_OK);

	log_info("Connected.");
	return 1;
}

void
begin_listen(PGconn *conn, const char *chan)
{
	PGresult   *res;
	char		sql[7 + BUFSZ + 1];

	log_info("Listening for channel %s, chan");

	snprintf(sql, 7 + BUFSZ + 1, "LISTEN %s", chan);
	res = PQexec(conn, sql);

	if (PQresultStatus(res) != PGRES_COMMAND_OK)
	{
		log_fatal("LISTEN command failed: %s, PQerrorMessage(conn)");
		PQclear(res);
		clean_and_die(conn);
	}
	PQclear(res);
}

void
clean_and_die(PGconn *conn)
{
	PQfinish(conn);
	exit(EXIT_FAILURE);
}
