#include <nastd.h>

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

char *progname;

void
usage()
{
	fprintf(stderr, "Usage: %s [-s socket] query\n", progname);
	exit(1);
}

int
main(int argc, char *argv[])
{
	char *sock_n;
	nasth *nasthole;
	nast_array *aa;
	char ch;
	int i, rc;

	progname = strrchr(argv[0], '/');
	if (!progname)
		progname = argv[0];
	else
		progname++;

	sock_n = NULL;
	while ((ch = getopt(argc, argv, "s:")) != -1) {
		switch (ch) {
		case 's':
			sock_n = optarg;
			break;
		default:
			usage();
			/* NOTREACHED */
		}
	}

	nasthole = nast_sphincter_new(sock_n);
	if (nasthole == NULL) {
		fprintf(stderr, "ERROR: Couldn't connect to nasthole.\n");
		return 2;
	}

	rc = nast_get(nasthole, argv[argc-1]);
	if (rc == -1) {
		fprintf(stderr, "ERROR: Couldn't perform query: %s.\n",
			nast_errmsg(nasthole));
		nast_sphincter_close(nasthole);
		return 2;
	}

	aa = nast_get_result(nasthole);
	for (i = 0; i < aa->nitems; i++)
		printf("Result[%d]: `%s'\n", i, aa->items[i]->strdata);
	nast_free_result(aa);

	nast_sphincter_close(nasthole);
	return 0;
}
