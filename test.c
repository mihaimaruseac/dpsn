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

static int do_test_san_cell(const struct sensor_network *sn, const struct grid *g, double t)
{
	double rho, rho_star, rho_bar;
	int ret = 0;

	if (g->n == 0)
		rho = 0;
	else
		rho = g->s / g->n;
	ret |= (rho >= sn->theta) << 2;

	if (g->n_star.val < t)
		rho_star = 0;
	else
		rho_star = g->s_star.val / g->n_star.val;
	/* TODO: might need a different threshold than theta */
	ret |= (rho_star >= sn->theta) << 1;

	if (g->n_bar.val < t)
		rho_bar = 0;
	else
		rho_bar = g->s_bar.val / g->n_bar.val;
	ret |= (rho_bar >= sn->theta) << 0;

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
		double t, int full_tree,
		struct san_measure *star, struct san_measure *bar)
{
	int i, r;

	if (full_tree || !g->Nu) {
		r = do_test_san_cell(sn, g, t);
		sm_parse(star, (r & 0x06) >> 1);
		sm_parse(bar, ((r & 0x04) >> 1) | (r & 0x01));
	}

	for (i = 0; i < g->Nu * g->Nu; i++)
		test_san(sn, &g->cells[i], t, full_tree, star, bar);
}

static void do_test_san(const struct sensor_network *sn, const struct grid *g,
		double t, int full_tree)
{
	struct san_measure star, bar;

	sm_init(&star);
	sm_init(&bar);
	printf("%5.2lf: ", t);
	test_san(sn, g, t, full_tree, &star, &bar);
	sm_print(&star);
	printf(" | ");
	sm_print(&bar);
	printf("\n");
}

void test_san_grid(const struct sensor_network *sn, const struct grid *g,
		double *t, int tsz)
{
	int i;

	for (i = 0; i < tsz; i++) {
		do_test_san(sn, g, t[i], 0);
		do_test_san(sn, g, t[i], 1);
	}
}
