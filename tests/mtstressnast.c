#include <nastd.h>

#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#define NTHREADS 10

nasth *nasthole;
char *username;

void *
io_looper(void *arg)
{
	struct timeval before, after;
	int i, rc;

	for (;;) {
		for (i = 0; i < 10; i++) {
			gettimeofday(&before, NULL);
			rc = nast_get(nasthole, username);
			gettimeofday(&after, NULL);
			if (rc == -1) {
				if (nast_geterr(nasthole) == NAST_TIMEDOUT) {
					fprintf(stderr,
						"ERROR: response timed out.\n");
					continue;
				}
				fprintf(stderr,
					"ERROR: Couldn't perform query: %s.\n",
					nast_errmsg(nasthole));
				return NULL;
			}

			after.tv_sec -= before.tv_sec;
			after.tv_usec -= before.tv_usec;
			if (after.tv_usec < 0) {
				after.tv_sec--;
				after.tv_usec += 1000000;
			}
			fprintf(stderr, "Request time: %2ld.%06ld seconds.\n",
				after.tv_sec, (unsigned long)after.tv_usec);
		}
	}
}

int
main(int argc, char *argv[])
{
	pthread_t tids[NTHREADS];
	int i;

	if (argc != 2) {
		printf("Usage: %s query\n", argv[0]);
		return 1;
	}
	username = argv[1];

	nasthole = nast_sphincter_new(NULL);
	if (nasthole == NULL) {
		fprintf(stderr, "ERROR: Couldn't connect to nasthole.\n");
		return 2;
	}

	for (i = 0; i < NTHREADS; i++)
		pthread_create(&tids[i], NULL, io_looper, NULL);

	for (i = 0; i < NTHREADS; i++)
		pthread_join(tids[i], NULL);

	nast_sphincter_close(nasthole);
	return 0;
}
