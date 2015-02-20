#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#include "globals.h"
#include "sanitization.h"
#include "sn.h"

void sanitize_ug(const struct sensor_network *sn, struct grid *g,
		double epsilon, double beta, double gamma,
		double K, int Nt, int seed)
{
	struct drand48_data randbuffer;
	double epsilon_0, epsilon_1;
	double Nu;
	int i;

	epsilon_0 = gamma * epsilon;
	epsilon_1 = epsilon - epsilon_0;

	init_rng(seed, &randbuffer);
	g->epsilon = epsilon_1;
	grd_compute_noisy(sn, g, epsilon_0, beta, &randbuffer);

	Nu = epsilon_1 * K * beta * (1 - beta) * (g->n_star.val + g->s_star.val / sn->M);

	if (Nu < 0 || (g->Nu = (int)sqrt(Nu)) < Nt)
		g->Nu = Nt;

	grd_split_cells(sn, g);

	for (i  = 0; i < g->Nu * g->Nu; i++)
		grd_compute_noisy(sn, &g->cells[i], epsilon_1, beta, &randbuffer);

	/* don't do any post-processing */
	g->n_bar = g->n_ave = g->n_star;
	g->s_bar = g->s_ave = g->s_star;
	for (i  = 0; i < g->Nu * g->Nu; i++) {
		g->cells[i].n_bar = g->cells[i].n_ave = g->cells[i].n_star;
		g->cells[i].s_bar = g->cells[i].s_ave = g->cells[i].s_star;
	}
}

void sanitize_ag(const struct sensor_network *sn, struct grid *g,
		double epsilon, double alpha, double beta, double gamma,
		double K, int Nt, int seed)
{
	struct drand48_data randbuffer;
	double epsilon_0, epsilon_1;
	double Nu;
	int i, j;

	epsilon_0 = gamma * epsilon;
	epsilon_1 = epsilon - epsilon_0;

	init_rng(seed, &randbuffer);
	g->epsilon = epsilon_1;
	grd_compute_noisy(sn, g, epsilon_0, beta, &randbuffer);

	Nu = epsilon_1 * K * beta * (1 - beta) * alpha * (g->n_star.val + g->s_star.val / sn->M);

	if (Nu < 0 || (g->Nu = (int)sqrt(Nu)) < Nt)
		g->Nu = Nt;

	grd_split_cells(sn, g);

	for (i  = 0; i < g->Nu * g->Nu; i++) {
		g->cells[i].epsilon = epsilon_1;
		grd_compute_noisy(sn, &g->cells[i], alpha * epsilon_1, beta, &randbuffer);
		Nu = epsilon_1 * K * beta * (1 - beta) * (1 - alpha) * (g->cells[i].n_star.val + g->cells[i].s_star.val / sn->M);
		if (Nu < 0 || (g->cells[i].Nu = (int)sqrt(Nu)) < Nt)
			g->cells[i].Nu = Nt;
		grd_split_cells(sn, &g->cells[i]);

		for (j = 0; j < g->cells[i].Nu * g->cells[i].Nu; j++)
			grd_compute_noisy(sn, &g->cells[i].cells[j], (1 - alpha) * epsilon_1, beta, &randbuffer);

		/* TODO: postprocessing up */
	}

	/* TODO: postprocessing down */
}

void sanitize_agt(const struct sensor_network *sn, struct grid *g,
		double epsilon, double alpha, double beta,
		double K, int Nt, int max_depth, int seed)
{
	struct drand48_data randbuffer;

	init_rng(seed, &randbuffer);
	g->epsilon = epsilon;
}
