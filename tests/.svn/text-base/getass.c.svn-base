#include <assd.h>

#include <stdio.h>
#include <stdlib.h>

int
main(int argc, char *argv[])
{
	assh *asshole;
	ass_array *aa;
	ass_options opts;
	int i, rc;

	if (argc != 2) {
		printf("Usage: %s query\n", argv[0]);
		return 1;
	}

	asshole = ass_sphincter_new();
	if (asshole == NULL) {
		fprintf(stderr, "ERROR: Couldn't connect to asshole.\n");
		return 2;
	}

	/* Get the default options. */
	rc = ass_options_get(asshole, &opts);
	if (rc == -1) {
		fprintf(stderr, "ERROR: Couldn't get options: %s.\n",
			ass_errmsg(asshole));
		ass_sphincter_close(asshole);
		return 2;
	}

	/* Add more fallthrough threads. */
	opts.fallthrough_threads = 5;
	rc = ass_options_set(asshole, &opts);
	if (rc == -1) {
		fprintf(stderr, "ERROR: Couldn't set options: %s.\n",
			ass_errmsg(asshole));
		ass_sphincter_close(asshole);
		return 2;
	}

	rc = ass_get(asshole, argv[1]);
	if (rc == -1) {
		fprintf(stderr, "ERROR: Couldn't perform query: %s.\n",
			ass_errmsg(asshole));
		ass_sphincter_close(asshole);
		return 2;
	}

	aa = ass_get_result(asshole);
	for (i = 0; i < aa->nitems; i++)
		printf("Result[%d]: `%s'\n", i, aa->items[i]->strdata);
	ass_free_result(aa);

	ass_sphincter_close(asshole);
	return 0;
}
