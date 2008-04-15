#include "conf.h"
#include "config.h"
#include "nastdio.h"
#include "nastipc.h"
#include "cdb.h"
#include "fqm.h"
#include "log.h"
#include "memdb.h"
#include "mysqldb.h"
#include "periodic.h"

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>

RCSID("$Id: nastd.c,v 1.8 2001/11/09 15:54:38 shmit Exp $");

char *progname;
time_t start_time;

static int
make_u_csock()
{
	struct sockaddr_un sunix;
	int sock;

	sock = socket(AF_UNIX, SOCK_STREAM, 0);
	if (sock == -1) {
		log_err("Couldn't create unix socket: %s.", strerror(errno));
		return -1;
	}

	memset(&sunix, 0, sizeof(sunix));
	snprintf(sunix.sun_path, sizeof(sunix.sun_path), config.nast_sock);
	sunix.sun_family = AF_UNIX;
	(void)unlink(sunix.sun_path);

	if (bind(sock, (struct sockaddr *)&sunix, sizeof(sunix)) == -1) {
		log_err("Couldn't bind to unix socket: %s.", strerror(errno));
		return -1;
	}
	(void)listen(sock, 10);

	return sock;
}

static int
make_i_csock()
{
	struct sockaddr_in sinet;
	int sock;

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == -1) {
		log_err("Couldn't create inet socket: %S.", strerror(errno));
		return -1;
	}

	memset(&sinet, 0, sizeof(sinet));
	sinet.sin_family = AF_INET;
	sinet.sin_addr.s_addr = INADDR_ANY;
	sinet.sin_port = htons(config.tcp_port);
	if (bind(sock, (struct sockaddr *)&sinet, sizeof(sinet)) == -1) {
		log_err("Couldn't bind to inet socket: %s.", strerror(errno));
		return -1;
	}
	(void)listen(sock, 10);

	return sock;
}

static int
do_client_connect(int sock)
{
	struct sockaddr_un saremote;
	socklen_t addrlen;
	int s;

	addrlen = sizeof(saremote);
	s = accept(sock, (struct sockaddr *)&saremote, &addrlen);
	if (s == -1) {
		log_err("Couldn't accept new connection: %s.",
			strerror(errno));
		return -1;
	}
	io_new(s);

	return 0;
}

static int
init_daemon()
{
	sigset_t sigmask;

	log_open();
	if (cdb_new())
		return -1;
	mysqldb_new();
	if (memdb_new())
		return -1;

	/*
	 * Turn off SIGPIPE. The calls to write() will return fine with
	 * it off, and this means we don't have to do any signal mojo.
	 */
	(void)sigemptyset(&sigmask);
	(void)sigaddset(&sigmask, SIGPIPE);
	(void)sigprocmask(SIG_BLOCK, &sigmask, NULL);

	/* Now we daemonise. */
	switch (fork()) {
	case 0:
		setsid();
		close(STDIN_FILENO);
		close(STDOUT_FILENO);
		break;
	case -1:
		log_err("Couldn't fork into daemon: %s.", strerror(errno));
		return -1;
	default:
		exit(0);
	}

	return 0;
}

void
usage()
{
	fprintf(stderr, "Usage: %s [options]\n"
			"\t-d directory\tSpecify directory for nast files.\n",
			progname);
	exit(1);
}

int
main(int argc, char *argv[])
{
	char ch;
	int u_csock, i_csock;

	progname = strrchr(argv[0], '/');
	if (!progname)
		progname = argv[0];
	else
		progname++;

	/* Initialise configuration values from file. */
	config_setdefaults();
	while ((ch = getopt(argc, argv, "d:")) != -1) {
		switch (ch) {
		case 'd':
			config.nast_dir = optarg;
			break;
		default:
			usage();
			/* NOTREACHED */
		}
	}
	if (config_init())
		return 1;

	if (init_daemon()) {
		log_err("Couldn't initialise nastd. Exiting.");
		return 1;
	}

	u_csock = make_u_csock();
	if (u_csock == -1)
		return 1;

	i_csock = make_i_csock();
	if (i_csock == -1)
		return 1;

	/* Create the FQM threads. */
	(void)fqm_new(10);

	/*
	 * Creat a thread that runs periodically.
	 */
	if (periodic_new() == -1) {
		log_err("Couldn't start periodic thread. Exiting.");
		return 1;
	}

	start_time = time(NULL);

	/*
	 * Main control loop. Sit on the socket and wait for a client to
	 * connect. As soon as it does, make another thread to handle it.
	 */
	for (;;) {
		fd_set read_fds;
		int rc;

		FD_ZERO(&read_fds);
		FD_SET(i_csock, &read_fds);
		FD_SET(u_csock, &read_fds);

		rc = select(10, &read_fds, NULL, NULL, NULL);
		if (rc == -1) {
			log_err("Couldn't select on sockets: %s.",
				strerror(errno));
			sleep(30);
		}

		if (FD_ISSET(i_csock, &read_fds))
			(void)do_client_connect(i_csock);

		if (FD_ISSET(u_csock, &read_fds))
			(void)do_client_connect(u_csock);
	}
	return 0;
}
