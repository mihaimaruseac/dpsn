#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "globals.h"
#include "sanitization.h"
#include "sn.h"
#include "test.h"

#ifndef DEBUG_GRID_TREE
#define DEBUG_GRID_TREE 0
#endif

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
	enum method method;
	/* method arguments */
	double gamma;
	int depth;
	/* random seed */
	long int seed;
} args;

static void usage(const char *prg)
{
	fprintf(stderr, "Usage: %s ALPHA BETA K NT EPS <u GAMMA|a GAMMA|t DEPTH> DATASET [SEED]\n", prg);
	exit(EXIT_FAILURE);
}


static void parse_arguments(int argc, char **argv)
{
	int i;

	printf("Called with: argc=%d\n", argc);
	for (i = 0; i < argc; i++)
		printf("%s ", argv[i]);
	printf("\n");

	if (argc < 9 || argc > 10)
		usage(argv[0]);
	if (sscanf(argv[1], "%lf", &args.alpha) != 1 || args.alpha <= 0 || args.alpha >= 1)
		usage(argv[0]);
	if (sscanf(argv[2], "%lf", &args.beta) != 1 || args.beta <= 0 || args.beta >= 1)
		usage(argv[0]);
	if (sscanf(argv[3], "%lf", &args.K) != 1)
		usage(argv[0]);
	if (sscanf(argv[4], "%u", &args.Nt) != 1)
		usage(argv[0]);
	if (sscanf(argv[5], "%lf", &args.eps) != 1 || args.eps <= 0)
		usage(argv[0]);
	if (strlen(argv[6]) != 1)
		usage(argv[0]);

	switch (argv[6][0]) {
	case 'u': args.method = UG; break;
	case 'a': args.method = AG; break;
	case 't': args.method = AGS; break;
	default: usage(argv[0]);
	}

	if (args.method < AGS && (sscanf(argv[7], "%lf", &args.gamma) != 1 || args.gamma <= 0 || args.gamma >= 1))
		usage(argv[0]);
	else if (args.method == AGS && (sscanf(argv[7], "%d", &args.depth) != 1 || args.depth <= 2))
		usage(argv[0]);

	args.dataset = strdup(argv[8]);
	if (argc == 10) {
		if (sscanf(argv[9], "%ld", &args.seed) != 1)
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
	sanitize(&sn, &g, args.eps, args.alpha, args.beta, args.gamma,
			args.K, args.Nt, args.depth, args.seed, args.method);

	printf("Sanitization finished, grid height: %d\n", grd_height(&g));

#if DEBUG_GRID_TREE
	{
		int i, h = grd_height(&g);
		char *fname = NULL;
		FILE *f;

		for (i = 0; i <= h; i++) {
			asprintf(&fname, "debug_%05d", i);
			f = fopen(fname, "w");
			if (!f)
				perror(fname);
			else {
				grd_debug(&sn, &g, f, i);
				fclose(f);
			}
			free(fname);
		}
	}
#endif

	double thresholds[1] = {0.1};
	int threshold_cnt = 1;
	test_san_grid(&sn, &g, thresholds, threshold_cnt);

	free(args.dataset);
	sn_cleanup(&sn);
	grd_cleanup(&g);
	return 0;
}
