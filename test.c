#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#include "globals.h"
#include "test.h"
#include "sn.h"

#ifndef TEST_THRESHOLD
#define TEST_THRESHOLD 0.5 //TODO: promote to param
#endif

#ifndef DEBUG_TEST_GRID_TREE
#define DEBUG_TEST_GRID_TREE 0
#endif

struct san_measure {
	int both; /* bits of 1 in the intersection */
	int either; /* bits of 1 in the union */
	int flip; /* bits of 1 in the xor */
	int all; /* all recorded samples */
};

static void sm_init(struct san_measure *sm)
{
	sm->both = sm->either = sm->flip = sm->all = 0;
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
}

static int do_test_san_cell(const struct sensor_network *sn, const struct grid *g)
{
	double rho, rho_star, rho_bar;
	int ret = 0;

	if (g->n == 0)
		rho = 0;
	else
		rho = g->s / g->n;
	ret |= (rho >= sn->theta) << 2;

	if (g->n_star.val < TEST_THRESHOLD)
		rho_star = 0;
	else
		rho_star = g->s_star.val / g->n_star.val;
	ret |= (rho_star >= sn->theta) << 1;

	if (g->n_bar.val < TEST_THRESHOLD)
		rho_bar = 0;
	else
		rho_bar = g->s_bar.val / g->n_bar.val;
	ret |= (rho_bar >= sn->theta) << 0;

#if DEBUG_TEST_GRID_TREE
	printf("%01x ~~ ", ret);
	printf(" %d %5.2lf %5.2lf %d", g->n, g->s, rho, rho >= sn->theta);
	printf(" %5.2lf %5.2lf %5.2lf %d", g->n_star.val, g->s_star.val, rho_star, rho_star >= sn->theta);
	printf(" %5.2lf %5.2lf %5.2lf %d\n", g->n_bar.val, g->s_bar.val, rho_bar, rho_bar >= sn->theta);
#endif

	return ret;
}

static void test_san(const struct sensor_network *sn, const struct grid *g,
		int full_tree,
		struct san_measure *star, struct san_measure *bar)
{
	int i, r;

	if (full_tree || !g->Nu) {
		r = do_test_san_cell(sn, g);
		sm_parse(star, (r & 0x6) >> 1);
		sm_parse(bar, ((r & 0x4) >> 1) || (r & 0x1));
		assert(star->all == bar->all);
	}

	for (i = 0; i < g->Nu * g->Nu; i++)
		test_san(sn, &g->cells[i], full_tree, star, bar);
}

void test_san_leaf_only(const struct sensor_network *sn, const struct grid *g)
{
	struct san_measure star, bar;

	sm_init(&star);
	sm_init(&bar);
	test_san(sn, g, 0, &star, &bar);
	printf("%d %d %d %d %3.2lf %3.2lf | %d %d %d %d %3.2lf %3.2lf\n",
			star.both, star.either, star.flip, star.all, (star.both + 0.0) / star.either, (star.flip + 0.0) / star.all,
			bar.both, bar.either, bar.flip, bar.all, (bar.both + 0.0) / bar.either, (bar.flip + 0.0) / bar.all);
}

void test_san_cell(const struct sensor_network *sn, const struct grid *g)
{
	struct san_measure star, bar;

	sm_init(&star);
	sm_init(&bar);
	test_san(sn, g, 1, &star, &bar);
	printf("%d %d %d %d %3.2lf %3.2lf | %d %d %d %d %3.2lf %3.2lf\n",
			star.both, star.either, star.flip, star.all, (star.both + 0.0) / star.either, (star.flip + 0.0) / star.all,
			bar.both, bar.either, bar.flip, bar.all, (bar.both + 0.0) / bar.either, (bar.flip + 0.0) / bar.all);
}
