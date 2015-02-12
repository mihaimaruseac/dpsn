#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#include "globals.h"
#include "sanitization.h"
#include "sn.h"

void method_setup(const struct sensor_network *sn, struct grid *g,
		struct drand48_data *randbuffer,
		double epsilon, int seed)
{
	init_rng(seed, randbuffer);
	g->epsilon = epsilon;
	grd_compute_real(sn, g);
}

void sanitize_ug(const struct sensor_network *sn, struct grid *g,
		double epsilon, double alpha, double beta,
		double K, int Nt, int seed)
{
	struct drand48_data randbuffer;
	double epsilon_0, epsilon_1;
	double Nu;
	int i;

	epsilon_0 = alpha * epsilon;
	epsilon_1 = epsilon - epsilon_0;
	printf("eps_0=%lf, eps_1=%lf\n", epsilon_0, epsilon_1);

	init_rng(seed, &randbuffer);
	g->epsilon = epsilon;
	grd_compute_real(sn, g);
	grd_compute_noisy(sn, g, epsilon_0, beta, &randbuffer);
	printf("n = %d, s = %d, n_star = %d, s_star = %d\n", g->n, g->s, g->n_star, g->s_star);

	Nu = epsilon_1 * K * beta * (1 - beta) * (g->n_star + g->s_star / sn->M);

	if (Nu < 0 || (g->Nu = (int)sqrt(Nu)) < Nt)
		g->Nu = Nt;
	printf("Nu = %lf, g->Nu = %d\n", Nu, g->Nu);

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

#if 0
void sanitize_ag(const struct sensor_network *sn, struct grid *g,
		double epsilon, double alpha, double beta,
		double K, int Nt, int seed)
{
	struct drand48_data randbuffer;

	method_setup(sn, g, &randbuffer, epsilon, seed);
}

void sanitize_agt(const struct sensor_network *sn, struct grid *g,
		double epsilon, double alpha, double beta,
		double K, int Nt, int seed)
{
	struct drand48_data randbuffer;

	method_setup(sn, g, &randbuffer, epsilon, seed);
}
#endif
