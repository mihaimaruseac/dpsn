#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "globals.h"
#include "test.h"
#include "sn.h"

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
			void *);
	void (*print)(const struct san_measure_comp*);
};

static double ratios[] = {.75, .9, .95, 1, 1.05, 1.1, 1.25};
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

static void test_san(const struct sensor_network *sn, const struct grid *g,
		int full_tree, int smc_cnt,
		struct san_measure_comp *smc)
{
	int i;

	if (full_tree || !g->Nu) {
		for (i = 0; i < smc_cnt; i++)
			smc[i].update(&smc[i], sn, g);
	}

	for (i = 0; i < g->Nu * g->Nu; i++)
		test_san(sn, &g->cells[i], full_tree, smc_cnt, smc);
}

static void do_test_san_shape(const struct sensor_network *sn,
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

static void generic_update(struct san_measure_comp* self,
			const struct sensor_network *sn,
			double s, double n,
			struct noisy_val s_star, struct noisy_val n_star,
			struct noisy_val s_bar, struct noisy_val n_bar)
{
	double rho, rho_star, rho_bar;

	rho = noisy_div(s, n, 0);
	rho_star = noisy_div(s_star.val, n_star.val, self->t);
	rho_bar = noisy_div(s_bar.val, n_bar.val, self->t);

	self->sm_star.all++;
	self->sm_bar.all++;

	if (rho > sn->theta) {
		self->sm_star.first++;
		self->sm_bar.first++;
		self->sm_star.either++;
		self->sm_bar.either++;
		if (rho_star < self->ratio) self->sm_star.flip++;
		else self->sm_star.both++;
		if (rho_bar < self->ratio) self->sm_bar.flip++;
		else self->sm_bar.both++;
	} else {
		if (rho_star > self->ratio) {
			self->sm_star.flip++;
			self->sm_star.either++;
		}
		if (rho_bar > self->ratio) {
			self->sm_bar.flip++;
			self->sm_bar.either++;
		}
	}
}

static void test_san_tree_cell(struct san_measure_comp* self,
			const struct sensor_network *sn, void *arg)
{
	struct grid *g = arg;
	generic_update(self, sn, g->s, g->n,
			g->s_star, g->n_star, g->s_bar, g->n_bar);
}

static void test_san_grid_cell(struct san_measure_comp* self,
			const struct sensor_network *sn, void *arg)
{
	struct low_res_grid_cell *g = arg;
	generic_update(self, sn, g->s, g->n,
			g->s_star, g->n_star, g->s_bar, g->n_bar);
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

void test_san_leaf_only(const struct sensor_network *sn, const struct grid *g, double t)
{
	struct san_measure_comp *smcs;
	int i, smc_cnt;

	smcs = setup_smcs(sn, t, &smc_cnt);
	for (i = 0; i < smc_cnt; i++)
		smcs[i].update = test_san_tree_cell;

	test_san(sn, g, 0, ratios_cnt, smcs);
	for (i = 0; i < smc_cnt; i++)
		smcs[i].print(&smcs[i]);
	free(smcs);
}

void test_san_cell(const struct sensor_network *sn, const struct grid *g, double t)
{
	struct san_measure_comp *smcs;
	int i, smc_cnt;

	smcs = setup_smcs(sn, t, &smc_cnt);
	for (i = 0; i < smc_cnt; i++)
		smcs[i].update = test_san_tree_cell;

	test_san(sn, g, 1, ratios_cnt, smcs);
	for (i = 0; i < smc_cnt; i++)
		smcs[i].print(&smcs[i]);
	free(smcs);
}

void test_san_shape(const struct sensor_network *sn,
		struct low_res_grid_cell **grid, int xcnt, int ycnt, double t)
{
	struct san_measure_comp *smcs;
	int i, smc_cnt;

	smcs = setup_smcs(sn, t, &smc_cnt);
	for (i = 0; i < smc_cnt; i++)
		smcs[i].update = test_san_grid_cell;

	do_test_san_shape(sn, grid, xcnt, ycnt, ratios_cnt, smcs);
	for (i = 0; i < smc_cnt; i++)
		smcs[i].print(&smcs[i]);
	free(smcs);
}
