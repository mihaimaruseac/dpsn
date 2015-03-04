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

struct test_grid {
	double t;
	struct san_measure star, bar;
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

static void do_test_san_cell(const struct sensor_network *sn, const struct grid *g,
		struct test_grid *tgs, int cnt)
{
	double rho, rho_star, rho_bar;
	int i;

	for (i = 0; i < cnt; i++) {
		if (g->n == 0)
			rho = 0;
		else
			rho = g->s / g->n;

		if (g->n_star.val < tgs[i].t)
			rho_star = 0;
		else
			rho_star = g->s_star.val / g->n_star.val;

		if (g->n_bar.val < tgs[i].t)
			rho_bar = 0;
		else
			rho_bar = g->s_bar.val / g->n_bar.val;

		if (rho >= sn->theta) {
			tgs[i].star.first++;
			tgs[i].bar.first++;
			if (rho_star >= sn->theta)
				tgs[i].star.both++;
			else
				tgs[i].star.flip++;
			if (rho_bar >= sn->theta)
				tgs[i].bar.both++;
			else
				tgs[i].bar.flip++;
		} else {
			if (rho_star >= sn->theta)
				tgs[i].star.flip++;
			if (rho_bar >= sn->theta)
				tgs[i].bar.flip++;
		}
		tgs[i].star.either = tgs[i].star.both + tgs[i].star.flip;
		tgs[i].bar.either = tgs[i].bar.both + tgs[i].bar.flip;
		tgs[i].star.all++;
		tgs[i].bar.all++;

#if DEBUG_TEST_GRID_TREE
		printf(" %d %5.2lf %5.2lf %d", g->n, g->s, rho, rho >= sn->theta);
		printf(" %5.2lf %5.2lf %5.2lf %d", g->n_star.val, g->s_star.val, rho_star, rho_star >= sn->theta);
		printf(" %5.2lf %5.2lf %5.2lf %d\n", g->n_bar.val, g->s_bar.val, rho_bar, rho_bar >= sn->theta);
		printf("  ~~  - ------ ---- -");
		printf(" %5.2lf %5.2lf ---- -", g->n_star.var, g->s_star.var);
		printf(" %5.2lf %5.2lf ---- -\n", g->n_bar.var, g->s_bar.var);
#endif
	}
}

static void test_san(const struct sensor_network *sn, const struct grid *g,
		double t, int full_tree,
		struct test_grid *tgs, int cnt)
{
	int i, r;

	if (full_tree || !g->Nu)
		do_test_san_cell(sn, g, tgs, cnt);

	for (i = 0; i < g->Nu * g->Nu; i++)
		test_san(sn, &g->cells[i], t, full_tree, tgs, cnt);
}

void test_san_grid(const struct sensor_network *sn, const struct grid *g,
		double *t, int tsz)
{
	struct test_grid *results_full;
	struct test_grid *results;
	int i;

	results = calloc(tsz, sizeof(results[0]));
	if (!results)
		die("Cannot allocate sanity result structures!");
	results_full = calloc(tsz, sizeof(results_full[0]));
	if (!results_full)
		die("Cannot allocate sanity result structures!");

	for (i = 0; i < tsz; i++) {
		results[i].t = t[i];
		results_full[i].t = t[i];
	}

	test_san(sn, g, t[i], 0, results, tsz);
	test_san(sn, g, t[i], 1, results_full, tsz);

	for (i = 0; i < tsz; i++) {
		printf("l %5.2lf: ", t[i]);
		sm_print(&results[i].star);
		printf(" | ");
		sm_print(&results[i].bar);
		printf("\n");
		printf("t %5.2lf: ", t[i]);
		sm_print(&results_full[i].star);
		printf(" | ");
		sm_print(&results_full[i].bar);
		printf("\n");
	}


	free(results);
	free(results_full);
}
