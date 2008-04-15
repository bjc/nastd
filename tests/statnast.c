#include <nastd.h>

#include <stdio.h>
#include <stdlib.h>

int
main(int argc, char *argv[])
{
	nasth *nasthole;
	nast_array *aa;
	int i, rc;

	if (argc != 1) {
		printf("Usage: %s\n", argv[0]);
		return 1;
	}

	nasthole = nast_sphincter_new(NULL);
	if (nasthole == NULL) {
		fprintf(stderr, "ERROR: Couldn't connect to nasthole.\n");
		return 2;
	}

	rc = nast_stats(nasthole);
	if (rc == -1) {
		fprintf(stderr, "ERROR: Couldn't get stats: %s.\n",
			nast_errmsg(nasthole));
		nast_sphincter_close(nasthole);
		return 2;
	}

	aa = nast_get_result(nasthole);
	for (i = 0; i < aa->nitems; i++)
		printf("%s\n", aa->items[i]->strdata);
	nast_free_result(aa);

	nast_sphincter_close(nasthole);
	return 0;
}
