#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "globals.h"
#include "test.h"
#include "sn.h"

#ifndef DEBUG_TEST_GRID_TREE
#define DEBUG_TEST_GRID_TREE 0
#endif

struct san_measure {
	int first; /* bits of 1 in the first set (the real values) */
	int both; /* bits of 1 in the intersection */
	int either; /* bits of 1 in the union */
	int flip; /* bits of 1 in the xor */
	int all; /* all recorded samples */
};

static void sm_init(struct san_measure *sm)
{
	memset(sm, 0, sizeof(*sm));
}

static void sm_print(const struct san_measure *sm)
{
	double jaccard = (sm->both + 0.0) / sm->either;
	double flip_ratio = (sm->flip + 0.0) / sm->all;
	flip_ratio = 1 - flip_ratio;
	printf("%d %d %d %d %d %3.2lf %3.2lf", sm->first, sm->both,
			sm->either, sm->flip, sm->all, jaccard, flip_ratio);
}

/* val is 0, 1, 2 or 3 */
static void sm_parse(struct san_measure *sm, int val)
{
	sm->all++;
	if (val == 3) {
		sm->both++;
		sm->either++;
	} else if (val != 0) {
		sm->flip++;
		sm->either++;
	}
	if (val > 1)
		sm->first++;
}

static int do_generic_test_san(double *rho, double *rho_star, double *rho_bar,
		double t, double nt, double theta, double s, double n,
		const struct noisy_val *n_star, const struct noisy_val *s_star,
		const struct noisy_val *n_bar, const struct noisy_val *s_bar)
{
	int ret = 0;

	*rho = noisy_div(s, n, 0);
	*rho_star = noisy_div(s_star->val, n_star->val, t);
	*rho_bar = noisy_div(s_bar->val, n_bar->val, t);

	ret |= (*rho >= theta) << 2;
	/* TODO: might need a different threshold than theta */
	ret |= (*rho_star >= nt * theta) << 1;
	ret |= (*rho_bar >= nt * theta) << 0;

	return ret;
}

static int do_test_san_cell(const struct sensor_network *sn,
		const struct grid *g, double t, double nt)
{
	double rho, rho_star, rho_bar;
	int ret = do_generic_test_san(&rho, &rho_star, &rho_bar, t, nt,
			sn->theta, g->s, g->n,
			&g->n_star, &g->s_star, &g->n_bar, &g->s_bar);

#if DEBUG_TEST_GRID_TREE
	printf("%01x ~~ ", ret);
	printf(" %d %5.2lf %5.2lf %d", g->n, g->s, rho, rho >= sn->theta);
	printf(" %5.2lf %5.2lf %5.2lf %d", g->n_star.val, g->s_star.val, rho_star, rho_star >= sn->theta);
	printf(" %5.2lf %5.2lf %5.2lf %d\n", g->n_bar.val, g->s_bar.val, rho_bar, rho_bar >= sn->theta);
	printf("  ~~  - ------ ---- -");
	printf(" %5.2lf %5.2lf ---- -", g->n_star.var, g->s_star.var);
	printf(" %5.2lf %5.2lf ---- -\n", g->n_bar.var, g->s_bar.var);
#endif

	return ret;
}

static void test_san(const struct sensor_network *sn, const struct grid *g,
		double t, double nt, int full_tree,
		struct san_measure *star, struct san_measure *bar)
{
	int i, r;

	if (full_tree || !g->Nu) {
		r = do_test_san_cell(sn, g, t, nt);
		sm_parse(star, (r & 0x06) >> 1);
		sm_parse(bar, ((r & 0x04) >> 1) | (r & 0x01));
	}

	for (i = 0; i < g->Nu * g->Nu; i++)
		test_san(sn, &g->cells[i], t, nt, full_tree, star, bar);
}

static void do_test_san(const struct sensor_network *sn, const struct grid *g,
		double t, double nt, int full_tree)
{
	struct san_measure star, bar;

	printf("Threshold: %5.2lf | Values: ", nt * sn->theta);
	sm_init(&star);
	sm_init(&bar);
	test_san(sn, g, t, nt, full_tree, &star, &bar);
	sm_print(&star);
	printf(" | ");
	sm_print(&bar);
	printf("\n");
}

static void do_test_san_shape(struct low_res_grid_cell **grid,
		int xcnt, int ycnt, double t, double nt, double theta)
{
	double rho, rho_star, rho_bar;
	struct san_measure star, bar;
	int i, j, r;

	printf("Threshold: %5.2lf | Values: ", nt * theta);
	sm_init(&star);
	sm_init(&bar);

	for (i = 0; i < xcnt; i++)
		for (j = 0; j < ycnt; j++) {
			r = do_generic_test_san(&rho, &rho_star, &rho_bar, t,
					nt, theta, grid[i][j].s, grid[i][j].n,
					&grid[i][j].n_star, &grid[i][j].s_star,
					&grid[i][j].n_bar, &grid[i][j].s_bar);
			sm_parse(&star, (r & 0x06) >> 1);
			sm_parse(&bar, ((r & 0x04) >> 1) | (r & 0x01));
		}

	sm_print(&star);
	printf(" | ");
	sm_print(&bar);
	printf("\n");
}

void test_san_leaf_only(const struct sensor_network *sn, const struct grid *g, double t)
{
	do_test_san(sn, g, t,  .75, 0);
	do_test_san(sn, g, t,   .9, 0);
	do_test_san(sn, g, t,  .95, 0);
	do_test_san(sn, g, t,    1, 0);
	do_test_san(sn, g, t, 1.05, 0);
	do_test_san(sn, g, t, 1.10, 0);
	do_test_san(sn, g, t, 1.25, 0);
}

void test_san_cell(const struct sensor_network *sn, const struct grid *g, double t)
{
	do_test_san(sn, g, t,  .75, 1);
	do_test_san(sn, g, t,   .9, 1);
	do_test_san(sn, g, t,  .95, 1);
	do_test_san(sn, g, t,    1, 1);
	do_test_san(sn, g, t, 1.05, 1);
	do_test_san(sn, g, t, 1.10, 1);
	do_test_san(sn, g, t, 1.25, 1);
}

void test_san_shape(struct low_res_grid_cell **grid, int xcnt, int ycnt,
		double t, double theta, double M)
{
	do_test_san_shape(grid, xcnt, ycnt, t,  .75, theta);
	do_test_san_shape(grid, xcnt, ycnt, t,   .9, theta);
	do_test_san_shape(grid, xcnt, ycnt, t,  .95, theta);
	do_test_san_shape(grid, xcnt, ycnt, t,    1, theta);
	do_test_san_shape(grid, xcnt, ycnt, t, 1.05, theta);
	do_test_san_shape(grid, xcnt, ycnt, t, 1.10, theta);
	do_test_san_shape(grid, xcnt, ycnt, t, 1.25, theta);
}
