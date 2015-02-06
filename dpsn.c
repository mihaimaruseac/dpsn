#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sn.h"

/* Command line arguments */
static struct {
	/* the alpha and beta parameters */
	double alpha, beta;
	/* the non-uniformity parameter */
	double K;
	/* the grid threshold */
	int Nt;
	/* global value for epsilon */
	double eps;
	/* dataset filename */
	char *dataset;
	/* the method */
	enum {UG, AG, AGS} method;
	/* random seed */
	long int seed;
} args;

static void usage(const char *prg)
{
	fprintf(stderr, "Usage: %s ALPHA BETA K NT EPS <u|a|t> DATASET [SEED]\n", prg);
	exit(EXIT_FAILURE);
}


static void parse_arguments(int argc, char **argv)
{
	int i;

	printf("Called with: argc=%d\n", argc);
	for (i = 0; i < argc; i++)
		printf("%s ", argv[i]);
	printf("\n");

	if (argc < 8 || argc > 9)
		usage(argv[0]);
	if (sscanf(argv[1], "%lf", &args.alpha) != 1 || args.alpha < 0 || args.alpha >= 1)
		usage(argv[0]);
	if (sscanf(argv[2], "%lf", &args.beta) != 1 || args.beta < 0 || args.beta >= 1)
		usage(argv[0]);
	if (sscanf(argv[3], "%lf", &args.K) != 1)
		usage(argv[0]);
	if (sscanf(argv[4], "%u", &args.Nt) != 1)
		usage(argv[0]);
	if (sscanf(argv[5], "%lf", &args.eps) != 1 || args.eps < 0)
		usage(argv[0]);
	if (strlen(argv[6]) != 1)
		usage(argv[0]);

	switch (argv[6][0]) {
	case 'u': args.method = UG; break;
	case 'a': args.method = AG; break;
	case 't': args.method = AGS; break;
	default: usage(argv[0]);
	}

	args.dataset = strdup(argv[7]);
	if (argc == 9) {
		if (sscanf(argv[8], "%ld", &args.seed) != 1)
			usage(argv[0]);
	} else
		args.seed = 42;
}

int main(int argc, char **argv)
{
	struct sensor_network sn;
	struct grid g;

	parse_arguments(argc, argv);
	sn_read_from_file(args.dataset, &sn);
	sn_convert_to_grid_root(&sn, &g);

	free(args.dataset);
	sn_cleanup(&sn);
	grd_cleanup(&g);
	return 0;
}
