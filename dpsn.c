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
#ifndef DEBUG_GRID_TREE_TEXT
#define DEBUG_GRID_TREE_TEXT 0
#endif
#ifndef DEBUG_FINE_GRID
#define DEBUG_FINE_GRID 1
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
	/* test threshold */
	double tthresh;
	/* resolution of final grid */
	double resolution;
	/* random seed */
	long int seed;
} args;

static void usage(const char *prg)
{
	fprintf(stderr, "Usage: %s ALPHA BETA K NT EPS <u GAMMA|a GAMMA|t DEPTH> TTHRESH RESOLUTION DATASET [SEED]\n", prg);
	exit(EXIT_FAILURE);
}


static void parse_arguments(int argc, char **argv)
{
	int i;

	printf("Called with: argc=%d\n", argc);
	for (i = 0; i < argc; i++)
		printf("%s ", argv[i]);
	printf("\n");

	if (argc < 11 || argc > 12)
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

	if (sscanf(argv[8], "%lf", &args.tthresh) != 1 || args.tthresh <= 0)
		usage(argv[0]);

	if (sscanf(argv[9], "%lf", &args.resolution) != 1 || args.resolution <= 0)
		usage(argv[0]);

	args.dataset = strdup(argv[10]);
	if (argc == 12) {
		if (sscanf(argv[11], "%ld", &args.seed) != 1)
			usage(argv[0]);
	} else
		args.seed = 42;
}

#if DEBUG_GRID_TREE || DEBUG_FINE_GRID
static FILE *get_file_pointer(const char *header, int ix)
{
	char *fname;
	FILE *f;

	if (ix >= 0)
		asprintf(&fname, "debug_%s_%02d", header, ix);
	else
		asprintf(&fname, "debug_%s", header);
	f = fopen(fname, "w");
	if (!f)
		perror(fname);

	free(fname);
	return f;
}
#endif

int main(int argc, char **argv)
{
	struct low_res_grid_cell **grid;
	struct sensor_network sn;
	int xcnt, ycnt, i, j, h;
	struct grid g;
	double r;

#if DEBUG_GRID_TREE || DEBUG_FINE_GRID
	FILE *f = NULL;
#endif

	parse_arguments(argc, argv);
	sn_read_from_file(args.dataset, &sn);
	sn_convert_to_grid_root(&sn, &g);
	sanitize(&sn, &g, args.eps, args.alpha, args.beta, args.gamma,
			args.K, args.Nt, args.depth, args.seed, args.method);

	h = grd_height(&g);
	printf("Sanitization finished, grid height: %d\n", h);

#if DEBUG_GRID_TREE
	for (i = 0; i <= h; i++) {
		if ((f = get_file_pointer("tree_grid", i))) {
			grd_debug(&sn, &g, f, i);
			fclose(f);
		}
	}
#endif
#if DEBUG_GRID_TREE_TEXT
	grd_debug0(&sn, &g);
#endif

	test_san_leaf_only(&sn, &g, args.tthresh);
	test_san_cell(&sn, &g, args.tthresh);

	grd_to_lrg(&sn, &g, args.resolution, &grid, &xcnt, &ycnt, args.tthresh);
	printf("Fine grid built, size %d x %d\n", xcnt, ycnt);

#if DEBUG_FINE_GRID
	f = get_file_pointer("uniform_grid", -1);
	lrg_debug(grid, xcnt, ycnt, args.tthresh, sn.theta, sn.M, f);
	fclose(f);
#endif

	test_san_shape(&sn, grid, xcnt, ycnt, args.tthresh);

	for (i = 1; i <= h / 2 + 1; i++) {
		printf("Testing on absolute positive votes %d\n", i);
		test_san_votes(&sn, grid, xcnt, ycnt, i);
	}

	printf("Testing on relative positive votes 0.5\n");
	test_san_rel_votes(&sn, grid, xcnt, ycnt, 0.5);
	printf("Testing on relative positive votes 0.75\n");
	test_san_rel_votes(&sn, grid, xcnt, ycnt, 0.75);

	for (i = 0; i <= h; i++)
		for (j = 0; j < 4; j++) {
			r = i + j / 4.0;
			r = 0.1 + r/2.5;
			printf("Testing on probabilistic weights, %5.2lf\n", r);
			test_san_p(&sn, grid, xcnt, ycnt, r);
		}

	free(args.dataset);
	sn_cleanup(&sn);
	grd_cleanup(&g);
	for (i = 0; i < xcnt; i++) free(grid[i]);
	free(grid);
	return 0;
}
