#include <nastd.h>

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

int
main(int argc, char *argv[])
{
	nasth *nasthole;
	struct timeval before, after;
	nast_options opts;
	int i, rc;

	if (argc != 2) {
		printf("Usage: %s query\n", argv[0]);
		return 1;
	}

	nasthole = nast_sphincter_new(NULL);
	if (nasthole == NULL) {
		fprintf(stderr, "ERROR: Couldn't connect to nasthole.\n");
		return 2;
	}

	for (;;) {
		/* Get the default options. */
		rc = nast_options_get(nasthole, &opts);
		if (rc == -1) {
			fprintf(stderr, "ERROR: Couldn't get options: %s.\n",
				nast_errmsg(nasthole));
			nast_sphincter_close(nasthole);
			return 2;
		}

		for (i = 0; i < 10; i++) {
			gettimeofday(&before, NULL);
			rc = nast_get(nasthole, argv[1]);
			gettimeofday(&after, NULL);
			if (rc == -1) {
				fprintf(stderr,
					"ERROR: Couldn't perform query: %s.\n",
					nast_errmsg(nasthole));
				nast_sphincter_close(nasthole);
				return 2;
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
	nast_sphincter_close(nasthole);
	return 0;
}
