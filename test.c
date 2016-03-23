#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "globals.h"
#include "test.h"
#include "sn.h"

#ifndef DEBUG_SMC_COMP_TC
#define DEBUG_SMC_COMP_TC 0
#endif
#ifndef DEBUG_SMC_COMP_GC
#define DEBUG_SMC_COMP_GC 0
#endif
#ifndef DEBUG_SMC_COMP_VA
#define DEBUG_SMC_COMP_VA 0
#endif
#ifndef DEBUG_SMC_COMP_VR
#define DEBUG_SMC_COMP_VR 0
#endif

struct san_measure {
	int first; /* bits of 1 in the first set (the real values) */
	int both; /* bits of 1 in the intersection */
	int either; /* bits of 1 in the union */
	int flip; /* bits of 1 in the xor */
	int all; /* all recorded samples */
};

struct san_measure_comp {
	struct san_measure sm_star;
	struct san_measure sm_bar;
	double ratio;
	double t; /* threshold for testing against 0 */
	void (*update)(struct san_measure_comp*,
			const struct sensor_network *,
			const void *);
	void (*print)(const struct san_measure_comp*);
};

static double ratios[] = {1}; //.75, .9, .95, 1, 1.05, 1.1, 1.25};
static const double ratios_cnt = sizeof(ratios) / sizeof(ratios[0]);

static void sm_print(const struct san_measure *sm)
{
	double jaccard = (sm->both + 0.0) / sm->either;
	double flip_ratio = (sm->flip + 0.0) / sm->all;
	flip_ratio = 1 - flip_ratio;
	printf("%d %d %d %d %d %3.2lf %3.2lf", sm->first, sm->both,
			sm->either, sm->flip, sm->all, jaccard, flip_ratio);
}

static void test_san_print(const struct san_measure_comp *self)
{
	printf("Threshold: %5.2lf | Values: ", self->ratio);
	sm_print(&self->sm_star);
	printf(" | ");
	sm_print(&self->sm_bar);
	printf("\n");
}

static void generic_update(struct san_measure_comp* self, double weight,
		double real, double star, double bar,
		double real_t, double private_t)
{
	self->sm_star.all += weight;
	self->sm_bar.all += weight;

	if (real >= real_t) {
		self->sm_star.first += weight;
		self->sm_bar.first += weight;
		self->sm_star.either += weight;
		self->sm_bar.either += weight;
		if (star < private_t) self->sm_star.flip += weight;
		else self->sm_star.both += weight;
		if (bar < private_t) self->sm_bar.flip += weight;
		else self->sm_bar.both += weight;
	} else {
		if (star >= private_t) {
			self->sm_star.flip += weight;
			self->sm_star.either += weight;
		}
		if (bar >= private_t) {
			self->sm_bar.flip += weight;
			self->sm_bar.either += weight;
		}
	}
}

static void test_san_tree_cell(struct san_measure_comp* self,
			const struct sensor_network *sn, const void *arg)
{
	const struct grid *g = arg;
#if DEBUG_SMC_COMP_TC
	printf(" g: (%5.2lf, %5.2lf) -- (%5.2lf, %5.2lf) %5.2lf\n",
			g->xmin, g->ymin, g->xmax, g->ymax, self->t);
	printf("Before: ");
	self->print(self);
#endif
	generic_update(self, 1,
			noisy_div(g->s, g->n, self->t),
			noisy_div(g->s_star.val, g->n_star.val, self->t),
			noisy_div(g->s_bar.val, g->n_bar.val, self->t),
			sn->theta, self->ratio);
#if DEBUG_SMC_COMP_TC
	printf("After : ");
	self->print(self);
#endif
}

static void test_san_grid_cell(struct san_measure_comp* self,
			const struct sensor_network *sn, const void *arg)
{
	const struct low_res_grid_cell *g = arg;
#if DEBUG_SMC_COMP_GC
	printf(" g: (%5.2lf, %5.2lf) -- (%5.2lf, %5.2lf) %5.2lf\n",
			g->xmin, g->ymin, g->xmax, g->ymax, self->t);
	printf("args: %5.2lf %5.2lf %5.2lf %5.2lf %5.2lf\n",
			noisy_div(g->s, g->n, self->t),
			noisy_div(g->s_star.val, g->n_star.val, self->t),
			noisy_div(g->s_bar.val, g->n_bar.val, self->t),
			sn->theta, self->ratio);
	printf("Before: ");
	self->print(self);
#endif
	generic_update(self, 1,
			noisy_div(g->s, g->n, self->t),
			noisy_div(g->s_star.val, g->n_star.val, self->t),
			noisy_div(g->s_bar.val, g->n_bar.val, self->t),
			sn->theta, self->ratio);
#if DEBUG_SMC_COMP_GC
	printf("After : ");
	self->print(self);
#endif
}

static void test_san_grid_votes_above(struct san_measure_comp* self,
			const struct sensor_network *sn, const void *arg)
{
	const struct low_res_grid_cell *g = arg;
	(void)sn;
#if DEBUG_SMC_COMP_VA
	printf(" g: (%5.2lf, %5.2lf) -- (%5.2lf, %5.2lf) %5.2lf\n",
			g->xmin, g->ymin, g->xmax, g->ymax, self->t);
	printf("args: %5.2lf %5.2lf %5.2lf %5.2lf %5.2lf\n",
			g->g_above, g->g_star_above, g->g_bar_above,
			self->t, self->t);
	printf("Before: ");
	self->print(self);
#endif
	generic_update(self, 1,
			g->g_above, g->g_star_above, g->g_bar_above,
			self->t, self->t);
#if DEBUG_SMC_COMP_VA
	printf("After : ");
	self->print(self);
#endif
}

static void test_san_grid_votes(struct san_measure_comp* self,
			const struct sensor_network *sn, const void *arg)
{
	const struct low_res_grid_cell *g = arg;
	(void)sn;
#if DEBUG_SMC_COMP_VR
	printf(" g: (%5.2lf, %5.2lf) -- (%5.2lf, %5.2lf) %5.2lf\n",
			g->xmin, g->ymin, g->xmax, g->ymax, self->t);
	printf("args: %5.2lf %5.2lf %5.2lf %5.2lf %5.2lf\n",
			g->g_above / (g->g_above + g->g_below),
			g->g_star_above / (g->g_star_above + g->g_star_below),
			g->g_bar_above / (g->g_bar_above + g->g_bar_below),
			self->t, self->t);
	printf("Before: ");
	self->print(self);
#endif
	generic_update(self, 1,
			g->g_above / (g->g_above + g->g_below),
			g->g_star_above / (g->g_star_above + g->g_star_below),
			g->g_bar_above / (g->g_bar_above + g->g_bar_below),
			self->t, self->t);
#if DEBUG_SMC_COMP_VR
	printf("After : ");
	self->print(self);
#endif
}

static void test_san_tree(const struct sensor_network *sn, const struct grid *g,
		int full_tree, int smc_cnt,
		struct san_measure_comp *smc)
{
	int i;

	if (full_tree || !g->Nu) {
		for (i = 0; i < smc_cnt; i++)
			smc[i].update(&smc[i], sn, g);
	}

	for (i = 0; i < g->Nu * g->Nu; i++)
		test_san_tree(sn, &g->cells[i], full_tree, smc_cnt, smc);
}

static void test_san_low_res(const struct sensor_network *sn,
		struct low_res_grid_cell **grid,
		int xcnt, int ycnt, int smc_cnt,
		struct san_measure_comp *smc)
{
	int i, j, k;

	for (i = 0; i < xcnt; i++)
		for (j = 0; j < ycnt; j++)
			for (k = 0; k < smc_cnt; k++)
				smc[k].update(&smc[k], sn, &grid[i][j]);
}

static struct san_measure_comp *setup_smcs(const struct sensor_network *sn,
		double t, int *smc_cnt)
{
	struct san_measure_comp *smcs = calloc(ratios_cnt, sizeof(smcs[0]));
	int i;

	*smc_cnt = ratios_cnt;

	/* setup */
	for (i = 0; i < ratios_cnt; i++) {
		smcs[i].ratio = ratios[i] * sn->theta;
		smcs[i].print = test_san_print;
		smcs[i].t = t;
	}

	return smcs;
}

static void test_result_cleanup(struct san_measure_comp *smcs, int smc_cnt)
{
	int i;

	for (i = 0; i < smc_cnt; i++)
		smcs[i].print(&smcs[i]);
	free(smcs);
}

static void test_san_grid(const struct sensor_network *sn,
		const struct grid *g, int ft, double t)
{
	struct san_measure_comp *smcs;
	int i, smc_cnt;

	smcs = setup_smcs(sn, t, &smc_cnt);
	for (i = 0; i < smc_cnt; i++)
		smcs[i].update = test_san_tree_cell;

	test_san_tree(sn, g, ft, smc_cnt, smcs);
	test_result_cleanup(smcs, smc_cnt);
}

static void test_san_lrg(const struct sensor_network *sn,
		struct low_res_grid_cell **grid, int xcnt, int ycnt, double t,
		void (*update)(struct san_measure_comp*,
			const struct sensor_network *,
			const void *))
{
	struct san_measure_comp *smcs;
	int i, smc_cnt;

	smcs = setup_smcs(sn, t, &smc_cnt);
	for (i = 0; i < smc_cnt; i++)
		smcs[i].update = update;

	test_san_low_res(sn, grid, xcnt, ycnt, ratios_cnt, smcs);
	test_result_cleanup(smcs, smc_cnt);
}

void test_san_leaf_only(const struct sensor_network *sn, const struct grid *g, double t)
{
	test_san_grid(sn, g, 0, t);
}

void test_san_cell(const struct sensor_network *sn, const struct grid *g, double t)
{
	test_san_grid(sn, g, 1, t);
}

void test_san_shape(const struct sensor_network *sn,
		struct low_res_grid_cell **grid, int xcnt, int ycnt, double t)
{
	test_san_lrg(sn, grid, xcnt, ycnt, t, test_san_grid_cell);
}

void test_san_votes(const struct sensor_network *sn,
		struct low_res_grid_cell **grid, int xcnt, int ycnt, double t)
{
	test_san_lrg(sn, grid, xcnt, ycnt, t, test_san_grid_votes_above);
}

void test_san_rel_votes(const struct sensor_network *sn,
		struct low_res_grid_cell **grid, int xcnt, int ycnt, double t)
{
	test_san_lrg(sn, grid, xcnt, ycnt, t, test_san_grid_votes);
}
