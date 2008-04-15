#include <assd.h>

#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#define NTHREADS 10

assh *asshole;
char *username;

void *
io_looper(void *arg)
{
	struct timeval before, after;
	ass_options opts;
	int i, rc;

	for (;;) {
		/* Get the default options. */
		rc = ass_options_get(asshole, &opts);
		if (rc == -1) {
			fprintf(stderr, "ERROR: Couldn't get options: %s.\n",
				ass_errmsg(asshole));
			return NULL;
		}

		/* Add more fallthrough threads. */
		opts.fallthrough_threads = 5;
		rc = ass_options_set(asshole, &opts);
		if (rc == -1) {
			fprintf(stderr, "ERROR: Couldn't set options: %s.\n",
				ass_errmsg(asshole));
			return NULL;
		}

		for (i = 0; i < 10; i++) {
			gettimeofday(&before, NULL);
			rc = ass_get(asshole, username);
			gettimeofday(&after, NULL);
			if (rc == -1) {
				if (ass_geterr(asshole) == ASS_TIMEDOUT) {
					fprintf(stderr,
						"ERROR: response timed out.\n");
					continue;
				}
				fprintf(stderr,
					"ERROR: Couldn't perform query: %s.\n",
					ass_errmsg(asshole));
				return NULL;
			}

			after.tv_sec -= before.tv_sec;
			after.tv_usec -= before.tv_usec;
			if (after.tv_usec < 0) {
				after.tv_sec--;
				after.tv_usec += 1000000;
			}
			fprintf(stderr, "Request time: %2ld.%06ld seconds.\n",
				after.tv_sec, after.tv_usec);
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

	asshole = ass_sphincter_new();
	if (asshole == NULL) {
		fprintf(stderr, "ERROR: Couldn't connect to asshole.\n");
		return 2;
	}

	for (i = 0; i < NTHREADS; i++)
		pthread_create(&tids[i], NULL, io_looper, NULL);

	for (i = 0; i < NTHREADS; i++)
		pthread_join(tids[i], NULL);

	ass_sphincter_close(asshole);
	return 0;
}
