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
		double alpha, double beta, double gamma,
		double K, int Nt, int max_depth,
		struct drand48_data *randbuffer, enum method method)
{
	double epsilon, Nu, factor;
	int i;

	printf("max_depth = %d, epsilon = %5.2lf\n", max_depth, g->epsilon);

	/* 1. compute noisy values inside the grid */
	if (method < AGS)
		epsilon = gamma * g->epsilon;
	else
		epsilon = alpha * g->epsilon;
	printf("\tUsing epsilon = %5.2lf to compute counts\n", epsilon);
	grd_compute_noisy(sn, g, epsilon, beta, randbuffer);

	/* 2. compute split factor */
	factor = K * beta * (1 - beta);
	switch(method) {
	case AG: factor *= alpha; /* no break, need the below too */
	case UG: factor *= 1 - gamma; break;
	case AGS: factor *= 1 - alpha; break;
	}
	printf("\tSplitting factor: %5.2lf (%5.2lf %5.2lf %5.2lf)\n", factor, K, alpha, beta);

	/* 3. Compute split size */
	Nu = factor * g->epsilon * (g->n_star.val + g->s_star.val / sn->M);
	printf("\tSplitting params: n: %5.2lf s: %5.2lf Nu: %5.2lf\n",
			g->n_star.val, g->s_star.val, Nu);

	/* 3. recursion end */
	if (max_depth == 0 || Nu < 0 || (g->Nu = (int)sqrt(Nu)) < Nt) {
		g->Nu = 0; // TODO: all 3 methods, UG,AG have Nt but still need work
#if 0 //UG
		g->Nu = Nt; // split once anyway
#endif
#if 0 //AG
		g->Nu = Nt;
#endif
		return;
	}
	printf("\tWill split into: %d\n", g->Nu);

	/* 4. split */
	grd_split_cells(sn, g);

	/* 5. recurse */
	epsilon = g->epsilon - epsilon;
#if 0 // UG
	for (i  = 0; i < g->Nu * g->Nu; i++)
		grd_compute_noisy(sn, &g->cells[i], epsilon, beta, randbuffer);
#endif
	for (i  = 0; i < g->Nu * g->Nu; i++) {
		g->cells[i].epsilon = epsilon;
#if 0 // AG
		grd_compute_noisy(sn, &g->cells[i], alpha * epsilon, beta, randbuffer);
		Nu = epsilon_1 * K * beta * (1 - beta) * (1 - alpha) * (g->cells[i].n_star.val + g->cells[i].s_star.val / sn->M);
		if (Nu < 0 || (g->cells[i].Nu = (int)sqrt(Nu)) < Nt)
			g->cells[i].Nu = Nt;
		grd_split_cells(sn, &g->cells[i]);

		for (j = 0; j < g->cells[i].Nu * g->cells[i].Nu; j++)
			grd_compute_noisy(sn, &g->cells[i].cells[j], (1 - alpha) * epsilon, beta, randbuffer);
	}
#endif
		build_tree(sn, &g->cells[i], alpha, beta, gamma, K, Nt,
				max_depth-1, randbuffer, method);
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
	if (method != AGS) max_depth = 1; /* constant 1 */
	build_tree(sn, g, alpha, beta, gamma, K, Nt, max_depth,
			&randbuffer, method);

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
