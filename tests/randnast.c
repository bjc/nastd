#include <nastd.h>

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

char letters[] = "abcdefghijklmnopqrstuvwxyz0123456789-_.";

int
main(int argc, char *argv[])
{
	nasth *nasthole;
	struct timeval before, after;
	nast_options opts;
	int i, rc;

	nasthole = nast_sphincter_new(NULL);
	if (nasthole == NULL) {
		fprintf(stderr, "ERROR: Couldn't connect to nasthole.\n");
		return 2;
	}

	srand(time(NULL));

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
			char uname[32];
			int i, maxlen;

			maxlen = rand() % sizeof(uname);
			for (i = 0; i < maxlen-1; i++)
				uname[i] = letters[rand() % sizeof(letters)];
			uname[maxlen] = '\0';

			gettimeofday(&before, NULL);
			rc = nast_get(nasthole, uname);
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
