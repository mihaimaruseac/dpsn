#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#include "globals.h"
#include "sanitization.h"
#include "sn.h"

static void build_tree(const struct sensor_network *sn, struct grid *g,
		double alpha, double beta, double gamma,
		double K, int Nt, int max_depth,
		struct drand48_data *randbuffer, enum method method)
{
	double epsilon, Nu, factor;
	int i;

	/* 1. compute noisy values inside the grid */
	if (method != AGS)
		epsilon = gamma * g->epsilon;
	else
		epsilon = alpha * g->epsilon;
	grd_compute_noisy(sn, g, epsilon, beta, randbuffer);

	/* 2. compute split factor */
	factor = K * beta * (1 - beta);
	switch(method) {
	case AG: factor *= alpha; /* no break, need the below too */
	case UG: factor *= 1 - gamma; break;
	case AGS: factor *= 1 - alpha; break;
	}

	/* 3. Compute split size */
	Nu = factor * g->epsilon * (g->n_star.val + g->s_star.val / sn->M);

	/* 3. recursion end */
	if (max_depth == 0 || Nu < 0 || (g->Nu = (int)sqrt(Nu)) < Nt) {
		if (method != AGS)
			g->Nu = Nt;
		else {
			struct grid *gc = grd_copy(g);
			grd_compute_noisy(sn, gc, g->epsilon - epsilon, beta, randbuffer);
			grd_average2(g, gc);
			grd_cleanup(gc);
			free(gc);
			g->Nu = 0; /* block further recursion */
		}
		return;
	}

	/* 4. split */
	grd_split_cells(sn, g);

	/* 5. recurse */
	epsilon = g->epsilon - epsilon;
	for (i  = 0; i < g->Nu * g->Nu; i++) {
		g->cells[i].epsilon = epsilon;

		if (method == UG) {
			grd_compute_noisy(sn, &g->cells[i], epsilon, beta, randbuffer);
			continue; /* no recursion on UG */
		}

		if (method == AG) {
			int j;

			grd_compute_noisy(sn, &g->cells[i], alpha * epsilon,
					beta, randbuffer);

			Nu = epsilon * K * beta * (1 - beta) * (1 - alpha) *
				(g->cells[i].n_star.val + g->cells[i].s_star.val / sn->M);
			if (Nu < 0 || (g->cells[i].Nu = (int)sqrt(Nu)) < Nt)
				g->cells[i].Nu = Nt;
			grd_split_cells(sn, &g->cells[i]);

			for (j = 0; j < g->cells[i].Nu * g->cells[i].Nu; j++)
				grd_compute_noisy(sn, &g->cells[i].cells[j],
						(1 - alpha) * epsilon,
						beta, randbuffer);
			continue; /* split both AG levels in here */
		}

		build_tree(sn, &g->cells[i], alpha, beta, gamma, K, Nt,
				max_depth-1, randbuffer, method);
	}
}

static void update_ave_copy(struct grid *g)
{
	g->n_ave = g->n_star;
	g->s_ave = g->s_star;
}

static void update_bar_copy(struct grid *g)
{
	g->n_bar = g->n_ave;
	g->s_bar = g->s_ave;
}

static void update_ave_leaves(struct grid *g)
{
	int i;

	for (i = 0; i < g->Nu * g->Nu; i++)
		update_ave_leaves(&g->cells[i]);

	if (!g->Nu)
		update_ave_copy(g);
}

static void update_tree_ave(struct grid *g)
{
	int i;

	if (!g->Nu)
		return;

	for (i = 0; i < g->Nu * g->Nu; i++)
		update_tree_ave(&g->cells[i]);

	grd_averagev(g);
}

static void update_tree_bar(struct grid *g)
{
#if 0
	int i;

	grd_averagev(g);

	for (i = 0; i < g->Nu * g->Nu; i++)
		update_tree_ave(&g->cells[i]);
#endif
}

void sanitize(const struct sensor_network *sn, struct grid *g,
		double epsilon, double alpha, double beta, double gamma,
		double K, int Nt, int max_depth,
		int seed, enum method method)
{
	struct drand48_data randbuffer;
	int i;

	init_rng(seed, &randbuffer);

	/* 1. Set epsilon of root cell */
	g->epsilon = epsilon;

	/* 2. Split cell, build tree */
	if (method != AGS) max_depth = 1; /* constant 1 */
	build_tree(sn, g, alpha, beta, gamma, K, Nt, max_depth,
			&randbuffer, method);

	/* 3. update _ave values */
	if (method != AGS) update_ave_leaves(g);
	if (method == UG) update_ave_copy(g);
	else update_tree_ave(g);
	if (method != AGS) update_ave_copy(g);

	/* 4. update _bar values */
	update_bar_copy(g);
	for (i = 0; i < g->Nu * g->Nu; i++)
		if (method == UG) update_bar_copy(&g->cells[i]);
		else update_tree_bar(&g->cells[i]);
}
