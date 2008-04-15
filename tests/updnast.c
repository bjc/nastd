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

	aa = nast_array_new();
	if (aa == NULL) {
		fprintf(stderr, "ERROR: Couldn't create array.\n");
		return 3;
	}

	for (i = optind+1; i < argc; i++)
		nast_array_add(aa, strlen(argv[i]), argv[i]);

	rc = nast_upd(nasthole, argv[optind], aa);
	nast_array_delete(aa);

	if (rc == -1) {
		fprintf(stderr, "ERROR: Couldn't perform update: %s.\n",
			nast_errmsg(nasthole));
		nast_sphincter_close(nasthole);
		return 2;
	}

	aa = nast_get_result(nasthole);
	if (aa->nitems == 0 || (aa->nitems == 1 &&
				aa->items[0]->strlen == 0))
		printf("Update for %s succeeded.\n", argv[optind]);
	else {
		for (i = 0; i < aa->nitems; i++)
			printf("Result[%d]: `%s'\n", i, aa->items[i]->strdata);
	}
	nast_free_result(aa);

	nast_sphincter_close(nasthole);
	return 0;
}
