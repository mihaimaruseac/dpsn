#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#include "globals.h"
#include "sanitization.h"
#include "sn.h"

#if 0
static void sanitize_ug(const struct sensor_network *sn, struct grid *g,
		double epsilon, double beta, double gamma,
		double K, int Nt,
		struct drand48_data *randbuffer)
{
	double epsilon_0, epsilon_1;
	double Nu;
	int i;

	epsilon_0 = gamma * epsilon;
	epsilon_1 = epsilon - epsilon_0;

	g->epsilon = epsilon_1;
	grd_compute_noisy(sn, g, epsilon_0, beta, randbuffer);

	Nu = epsilon_1 * K * beta * (1 - beta) * (g->n_star.val + g->s_star.val / sn->M);

	if (Nu < 0 || (g->Nu = (int)sqrt(Nu)) < Nt)
		g->Nu = Nt;

	grd_split_cells(sn, g);

	for (i  = 0; i < g->Nu * g->Nu; i++)
		grd_compute_noisy(sn, &g->cells[i], epsilon_1, beta, randbuffer);

	/* don't do any post-processing */
	g->n_bar = g->n_ave = g->n_star;
	g->s_bar = g->s_ave = g->s_star;
	for (i  = 0; i < g->Nu * g->Nu; i++) {
		g->cells[i].n_bar = g->cells[i].n_ave = g->cells[i].n_star;
		g->cells[i].s_bar = g->cells[i].s_ave = g->cells[i].s_star;
	}
}

static void sanitize_ag(const struct sensor_network *sn, struct grid *g,
		double epsilon, double alpha, double beta, double gamma,
		double K, int Nt,
		struct drand48_data *randbuffer)
{
	double epsilon_0, epsilon_1;
	double Nu;
	int i, j;

	epsilon_0 = gamma * epsilon;
	epsilon_1 = epsilon - epsilon_0;

	g->epsilon = epsilon_1;
	grd_compute_noisy(sn, g, epsilon_0, beta, randbuffer);

	Nu = epsilon_1 * K * beta * (1 - beta) * alpha * (g->n_star.val + g->s_star.val / sn->M);

	if (Nu < 0 || (g->Nu = (int)sqrt(Nu)) < Nt)
		g->Nu = Nt;

	grd_split_cells(sn, g);

	for (i  = 0; i < g->Nu * g->Nu; i++) {
		g->cells[i].epsilon = epsilon_1;
		grd_compute_noisy(sn, &g->cells[i], alpha * epsilon_1, beta, randbuffer);
		Nu = epsilon_1 * K * beta * (1 - beta) * (1 - alpha) * (g->cells[i].n_star.val + g->cells[i].s_star.val / sn->M);
		if (Nu < 0 || (g->cells[i].Nu = (int)sqrt(Nu)) < Nt)
			g->cells[i].Nu = Nt;
		grd_split_cells(sn, &g->cells[i]);

		for (j = 0; j < g->cells[i].Nu * g->cells[i].Nu; j++)
			grd_compute_noisy(sn, &g->cells[i].cells[j], (1 - alpha) * epsilon_1, beta, randbuffer);

		/* TODO: postprocessing up */
	}

	/* TODO: postprocessing down */
}

static void sanitize_agt(const struct sensor_network *sn, struct grid *g,
		double epsilon, double alpha, double beta,
		double K, int Nt, int max_depth,
		struct drand48_data *randbuffer)
{
	g->epsilon = epsilon;
}
#endif

static void build_tree(const struct sensor_network *sn, struct grid *g,
		double alpha, double beta,
		double K, int Nt, int max_depth,
		struct drand48_data *randbuffer)
{
	double epsilon, Nu, factor;
	int i;

	printf("max_depth = %d, epsilon = %5.2lf\n", max_depth, g->epsilon);

	/* 1. compute noisy values inside the grid */
	epsilon = alpha * g->epsilon;
	printf("\tUsing epsilon = %5.2lf to compute counts\n", epsilon);
	grd_compute_noisy(sn, g, epsilon, beta, randbuffer);

	/* 2. compute split */
	factor = K * beta * (1 - beta) * (1 - alpha); // TODO: compute for the UG, AG and extract as param
	printf("\tSplitting factor: %5.2lf (%5.2lf %5.2lf %5.2lf)\n", factor, K, alpha, beta);
	Nu = factor * g->epsilon * (g->n_star.val + g->s_star.val / sn->M);
	printf("\tSplitting params: n: %5.2lf s: %5.2lf Nu: %5.2lf\n",
			g->n_star.val, g->s_star.val, Nu);

	/* 3. recursion end */
	if (max_depth == 0 || Nu < 0 || (g->Nu = (int)sqrt(Nu)) < Nt) {
		g->Nu = 0; // TODO: all 3 methods, UG,AG have Nt but still need work
		return;
	}
	printf("\tWill split into: %d\n", g->Nu);

	/* 4. split and recurse */
	grd_split_cells(sn, g);
	epsilon = g->epsilon - epsilon;
	for (i  = 0; i < g->Nu * g->Nu; i++) {
		g->cells[i].epsilon = epsilon;
		build_tree(sn, &g->cells[i], alpha, beta, K, Nt,
				max_depth-1, &randbuffer);
	}
}

/**
 * Steps:
 *  1. Set g->epsilon = epsilon to be used from the cell downwards (epsilon
 *     for AGS, TODO for others)
 *  2. Split grid downwards (TODO: fix UG, AG)
 */
void sanitize(const struct sensor_network *sn, struct grid *g,
		double epsilon, double alpha, double beta, double gamma,
		double K, int Nt, int max_depth,
		int seed, enum method method)
{
	struct drand48_data randbuffer;
	init_rng(seed, &randbuffer);

	/* 1. Set epsilon of root cell */
	g->epsilon = epsilon;

	/* 2. Split cell, build tree */
	// TODO: below is only for tree
	build_tree(sn, g, alpha, beta, K, Nt, max_depth, &randbuffer);

#if 0
	switch (method) {
	case UG:  sanitize_ug(sn, g, epsilon, beta, gamma,
				  K, Nt, &randbuffer);
		  break;
	case AG:  sanitize_ag(sn, g, epsilon, alpha, beta,
				  gamma, K, Nt, &randbuffer);
		  break;
	case AGS: sanitize_agt(sn, g, epsilon, alpha, beta,
				  K, Nt, max_depth, &randbuffer);
		  break;
	}
#endif
}
