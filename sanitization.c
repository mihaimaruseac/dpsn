#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#include "globals.h"
#include "sanitization.h"
#include "sn.h"

/* remember that the split is always square of g->Nu */
#ifndef MAX_SPLIT_SIZE
#define MAX_SPLIT_SIZE 2
#endif

#ifndef AG_SPLIT_ALWAYS_IN_TWO
#define AG_SPLIT_ALWAYS_IN_TWO 1
#endif

/* debug tree sanitization macros */
/* build the tree, divide budget, etc. */
#ifndef DEBUG_TREE_SANITIZATION_BUILD
#define DEBUG_TREE_SANITIZATION_BUILD 0
#endif
/* weighted averaging */
#ifndef DEBUG_TREE_SANITIZATION_UP
#define DEBUG_TREE_SANITIZATION_UP 0
#endif
/* sum consistency */
#ifndef DEBUG_TREE_SANITIZATION_DOWN
#define DEBUG_TREE_SANITIZATION_DOWN 0
#endif
#define DEBUG_TREE_SANITIZATION (DEBUG_TREE_SANITIZATION_BUILD ||\
		DEBUG_TREE_SANITIZATION_UP || DEBUG_TREE_SANITIZATION_DOWN)

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

	debug(DEBUG_TREE_SANITIZATION_BUILD, "%d: g->epsilon: %lf, epsilon: %lf", grd_level(g), g->epsilon, epsilon);
	debug(DEBUG_TREE_SANITIZATION_BUILD, "   (%5.2lf, %5.2lf) -- (%5.2lf, %5.2lf)", g->xmin, g->ymin, g->xmax, g->ymax);
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
	debug(DEBUG_TREE_SANITIZATION_BUILD, "   g->n_star:%lf g->s_star/M:%lf", g->n_star.val, g->s_star.val/sn->M);
	debug(DEBUG_TREE_SANITIZATION_BUILD, "   Nu:%lf", Nu);
	debug(DEBUG_TREE_SANITIZATION_BUILD, "Nt:%d, n*:%5.2lf", Nt, g->n_star.val);

	/* 3. recursion end */
	if (method == AGS) {
		if (Nu < 0 || max_depth == 0 || g->n_star.val < Nt) {
			struct grid *gc;
again:
			gc = grd_copy(g);
			grd_compute_noisy(sn, gc, g->epsilon - epsilon, beta, randbuffer);
			grd_average2(g, gc);

			debug(DEBUG_TREE_SANITIZATION_BUILD, "C  g->n_star:%lf g->s_star/M:%lf", gc->n_star.val, gc->s_star.val/sn->M);
			debug(DEBUG_TREE_SANITIZATION_BUILD, "A  g->n_ave:%lf g->s_ave/M:%lf", g->n_ave.val, g->s_ave.val/sn->M);
			debug(DEBUG_TREE_SANITIZATION_BUILD, "Av g->n_ave:%lf g->s_ave:%lf", g->n_ave.var, g->s_ave.var);

			grd_cleanup(gc);
			free(gc);
			g->Nu = 0; /* block further recursion */
			return;
		} else
			g->Nu = (int)sqrt(Nu);

		debug(DEBUG_TREE_SANITIZATION_BUILD, "   Nu:%d area:%lf", g->Nu, grd_size(g));

		if (g->Nu < 2) goto again; /* should do a split in at least 4 cells */
		//if (grd_size(g) < MAX_SPLIT_SIZE) goto again; /* don't split if area is too small */
		g->Nu = min(g->Nu, MAX_SPLIT_SIZE);
	}
	if (method != AGS && (Nu < 0 || (g->Nu = (int)sqrt(Nu)) < Nt)) {
		g->Nu = Nt;
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
			if (Nu < 0 || (g->cells[i].Nu = (int)sqrt(Nu)) < Nt) {
#if AG_SPLIT_ALWAYS_IN_TWO
				g->cells[i].Nu = Nt;
#else
				g->cells[i].Nu = 0;
				continue;
#endif
			}
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

	debug(DEBUG_TREE_SANITIZATION_UP, "%d (%6.2lf, %6.2lf) -- (%6.2lf, %6.2lf)",
			grd_level(g), g->xmin, g->ymin, g->xmax, g->ymax);
	debug(DEBUG_TREE_SANITIZATION_UP, "Real: | s=%9.2lf n=%8d ", g->s, g->n);
	debug(DEBUG_TREE_SANITIZATION_UP, "Nois: | s=%9.2lf n=%8.2lf ", g->s_star.val, g->n_star.val);
	debug(DEBUG_TREE_SANITIZATION_UP, "Aver: | s=%9.2lf n=%8.2lf ", g->s_ave.val, g->n_ave.val);
}

static void update_tree_bar(struct grid *g)
{
	int i;

	grd_consistency(g);

	for (i = 0; i < g->Nu * g->Nu; i++)
		update_tree_bar(&g->cells[i]);
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

	debug(DEBUG_TREE_SANITIZATION_BUILD, "Tree build, sanitization up following");

	/* 3. update _ave values */
	if (method != AGS) update_ave_leaves(g);
	if (method == UG) update_ave_copy(g);
	else update_tree_ave(g);
	if (method != AGS) update_ave_copy(g);

	debug(DEBUG_TREE_SANITIZATION_UP, "Averaging done, sanitization down following");

	/* 4. update _bar values */
	update_bar_copy(g);
	if (method == UG) for (i = 0; i < g->Nu * g->Nu; i++) update_bar_copy(&g->cells[i]);
	else update_tree_bar(g);

	debug(DEBUG_TREE_SANITIZATION, "Grid-tree built and consistently created");
}
