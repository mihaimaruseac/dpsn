#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "globals.h"
#include "sn.h"

#ifndef DEBUG_NOISE
#define DEBUG_NOISE 0
#endif
#ifndef DEBUG_GRD2LRG
#define DEBUG_GRD2LRG 1
#endif

static void grd_init(struct grid *g,
		double xmin, double xmax, double ymin, double ymax,
		struct grid *parent)
{
	g->xmin = xmin;
	g->xmax = xmax;
	g->ymin = ymin;
	g->ymax = ymax;
	g->cells = NULL;
	g->Nu = 0;
	g->n = g->s = 0;
	g->parent = parent;
}

static void grd_add_point(const struct sensor_network *sn, struct grid *g, int ix)
{
	int c = (g->xmin <= sn->sensors[ix].x) &&
		(g->ymin <= sn->sensors[ix].y) &&
		(g->xmax > sn->sensors[ix].x) &&
		(g->ymax > sn->sensors[ix].y);
	if (!c) {
		printf("~~~(%5.2lf %5.2lf) -- (%5.2lf %5.2lf)\n",
				g->xmin, g->ymin, g->xmax, g->ymax);
		printf("~~~%5.2lf %5.2lf\n", sn->sensors[ix].x, sn->sensors[ix].y);
	}
	assert(c);
	g->n++;
	g->s += sn->sensors[ix].val;
}

static void grd_print_cell_walls(const struct grid *g, FILE *f, int depth)
{
	int i;

	fprintf(f, "%5.2lf %5.2lf %5.2lf 0\n", g->xmin, g->ymin, g->xmax - g->xmin);
	fprintf(f, "%5.2lf %5.2lf 0 %5.2lf\n", g->xmin, g->ymin, g->ymax - g->ymin);
	fprintf(f, "%5.2lf %5.2lf %5.2lf 0\n", g->xmin, g->ymax, g->xmax - g->xmin);
	fprintf(f, "%5.2lf %5.2lf 0 %5.2lf\n", g->xmax, g->ymin, g->ymax - g->ymin);

	for (i = 0; i < g->Nu*g->Nu && depth > 0; i++)
		grd_print_cell_walls(&g->cells[i], f, depth - 1);
}

static void grd_print_cell_vals(const struct grid *g, FILE *f, int depth)
{
	double x, y;

	if (depth > 0){
		int i;
		for (i = 0; i < g->Nu*g->Nu && depth > 0; i++)
			grd_print_cell_vals(&g->cells[i], f, depth - 1);
		return;
	}

	if (depth < 0)
		return;

	x = (g->xmin + g->xmax)/2;
	y = (g->ymin + g->ymax)/2;
	fprintf(f, "%5.2lf %5.2lf %d %5.2lf %5.2lf %5.2lf %5.2lf %5.2lf "
			"%5.2lf %5.2lf %5.2lf %5.2lf %5.2lf %5.2lf %5.2lf "
			"%5.2lf\n", x, y, g->n, g->s,
			g->n_star.val, g->n_star.var, g->s_star.val, g->s_star.var,
			g->n_ave.val, g->n_ave.var, g->s_ave.val, g->s_ave.var,
			g->n_bar.val, g->n_bar.var, g->s_bar.val, g->s_bar.var);
}

static struct noisy_val nv_average2(struct noisy_val a, struct noisy_val b)
{
	double alpha = b.var / (a.var + b.var);
	struct noisy_val ret;

	ret.val = alpha * a.val + (1 - alpha) * b.val;
	ret.var = alpha * a.var;

	return ret;
}

static int sensor_cmp(const void *a, const void *b)
{
	const struct sensor *sa = a, *sb = b;
	int try = double_cmp(&sa->x, &sb->x);
	if (try) return try;
	return double_cmp(&sa->y, &sb->y);
}

static int overlap(const struct grid *g,
		double xmin, double xmax, double ymin, double ymax)
{
	if (xmin >= g->xmax || g->xmin >= xmax)
		return 0;
	if (ymin >= g->ymax || g->ymin >= ymax)
		return 0;

	return 1;
}

static double answer_prob(struct noisy_val s, struct noisy_val n,
		double T, double t, double M)
{
	double vn, vs, rho, e, v, p;

	if (fabs(s.val) <= t || fabs(n.val) <= t) {
		debug(DEBUG_GRD2LRG, "         n/a");
		return 0; /* not applicable */
	}

	vn = n.var / n.val / n.val;
	vs = M * M * s.var / s.val / s.val;
	debug(DEBUG_GRD2LRG, "  prob   s: %5.2lf , %5.2lf, %5.2lf", s.val, s.var, vs);
	debug(DEBUG_GRD2LRG, "  prob   n: %5.2lf , %5.2lf, %5.2lf", n.val, n.var, vn);

	rho = s.val / n.val;
	e = rho * (1 + vn);
	v = rho * rho * (vs + vn);
	debug(DEBUG_GRD2LRG, "   rho/e/v: %5.2lf , %5.2lf, %5.2lf", rho, e, v);

	if (e < T) {
		debug(DEBUG_GRD2LRG, "         neg");
		return 0; /* negative case */
	}

	p = v / (v + (e - T) * (e - T));
	debug(DEBUG_GRD2LRG, "  prob   p: %5.2lf", p);
	return 1 - p;
}

static void answer_full(const struct grid *g, double theta, double t,
		struct low_res_grid_cell *cell, double M,
		double xmin, double xmax, double ymin, double ymax)
{
	double ag, ar, f;
	int i;

	ag = (g->xmax - g->xmin) * (g->ymax - g->ymin);
	ar = (xmax - xmin) * (ymax - ymin);

	debug(DEBUG_GRD2LRG, "  Now cell: (%5.2lf, %5.2lf) -- (%5.2lf, %5.2lf)",
			cell->xmin, cell->ymin, cell->xmax, cell->ymax);
	debug(DEBUG_GRD2LRG, "  Now    g: (%5.2lf, %5.2lf) -- (%5.2lf, %5.2lf) %d",
			g->xmin, g->ymin, g->xmax, g->ymax, g->Nu);
	debug(DEBUG_GRD2LRG, "         n: %5.2lf | %5.2lf | %5.2lf", cell->n, cell->n_star.val, cell->n_bar.val);
	debug(DEBUG_GRD2LRG, "         s: %5.2lf | %5.2lf | %5.2lf", cell->s, cell->s_star.val, cell->s_bar.val);
	debug(DEBUG_GRD2LRG, "         +: %5.2lf | %5.2lf | %5.2lf", cell->g_above, cell->g_star_above, cell->g_bar_above);
	debug(DEBUG_GRD2LRG, "         -: %5.2lf | %5.2lf | %5.2lf", cell->g_below, cell->g_star_below, cell->g_bar_below);

	/* leaf or full cell coverage */
	if ((!g->Nu) ||
			((g->xmin == xmin) && (g->xmax == xmax) &&
			 (g->ymin == ymin) && (g->ymax == ymax))) {
		f = ar / ag;
		debug(DEBUG_GRD2LRG, "Leaf, f=%lf ar=%lf ag=%lf", f, ar, ag);

		cell->n_star.val += f * g->n_star.val; cell->n_star.var += f * f * g->n_star.var;
		cell->s_star.val += f * g->s_star.val; cell->s_star.var += f * f * g->s_star.var;
		cell->n_bar.val += f * g->n_bar.val; cell->n_bar.var += f * f * g->n_bar.var;
		cell->s_bar.val += f * g->s_bar.val; cell->s_bar.var += f * f * g->s_bar.var;
		cell->n += f * g->n;
		cell->s += f * g->s;
		goto vote;
	}

	for (i = 0; i < g->Nu * g->Nu; i++) {
		if ((g->cells[i].xmin >= xmax) || (g->cells[i].xmax <= xmin)) {
			i+= g->Nu - 1;
			continue;
		}

		if (overlap(&g->cells[i], xmin, xmax, ymin, ymax)) {
			debug(DEBUG_GRD2LRG, "Overlap %d", i);
			answer_full(&g->cells[i], theta, t, cell, M,
					max(xmin, g->cells[i].xmin),
					min(xmax, g->cells[i].xmax),
					max(ymin, g->cells[i].ymin),
					min(ymax, g->cells[i].ymax));
		}
	}

vote:
	debug(DEBUG_GRD2LRG, "V n : %9.2lf | %9.2lf | %9.2lf",
			0.0 + g->n, g->n_star.val, g->n_bar.val);
	debug(DEBUG_GRD2LRG, "V s : %9.2lf | %9.2lf | %9.2lf",
			g->s, g->s_star.val, g->s_bar.val);
	debug(DEBUG_GRD2LRG, "Vrho: %9.2lf | %9.2lf | %9.2lf",
			noisy_div(g->s, g->n, t),
			noisy_div(g->s_star.val, g->n_star.val, t),
			noisy_div(g->s_bar.val, g->n_bar.val, t));

	/* voting */
	if (noisy_div(g->s, g->n, t) >= theta)
		cell->g_above++;
	else
		cell->g_below++;
	if (noisy_div(g->s_star.val, g->n_star.val, t) >= theta)
		cell->g_star_above++;
	else
		cell->g_star_below++;
	if (noisy_div(g->s_bar.val, g->n_bar.val, t) >= theta)
		cell->g_bar_above++;
	else
		cell->g_bar_below++;

	/* weighted voting */
	cell->g_p_star += answer_prob(g->s_star, g->n_star, theta, t, M);
	cell->g_p_bar += answer_prob(g->s_bar, g->n_bar, theta, t, M);

	debug(DEBUG_GRD2LRG, "  End cell: (%5.2lf, %5.2lf) -- (%5.2lf, %5.2lf)",
			cell->xmin, cell->ymin, cell->xmax, cell->ymax);
	debug(DEBUG_GRD2LRG, "  End    g: (%5.2lf, %5.2lf) -- (%5.2lf, %5.2lf) %d",
			g->xmin, g->ymin, g->xmax, g->ymax, g->Nu);
	debug(DEBUG_GRD2LRG, "         n: %5.2lf | %5.2lf | %5.2lf", cell->n, cell->n_star.val, cell->n_bar.val);
	debug(DEBUG_GRD2LRG, "         s: %5.2lf | %5.2lf | %5.2lf", cell->s, cell->s_star.val, cell->s_bar.val);
	debug(DEBUG_GRD2LRG, "         +: %5.2lf | %5.2lf | %5.2lf", cell->g_above, cell->g_star_above, cell->g_bar_above);
	debug(DEBUG_GRD2LRG, "         -: %5.2lf | %5.2lf | %5.2lf", cell->g_below, cell->g_star_below, cell->g_bar_below);
	debug(DEBUG_GRD2LRG, "   prob  p:       | %5.2lf | %5.2lf", cell->g_p_star, cell->g_p_bar);
}

static void answer(const struct sensor_network *sn, const struct grid *g,
		struct low_res_grid_cell *cell, double t)
{
	debug(DEBUG_GRD2LRG, "Start cell: (%5.2lf, %5.2lf) -- (%5.2lf, %5.2lf)",
			cell->xmin, cell->ymin, cell->xmax, cell->ymax);
	debug(DEBUG_GRD2LRG, "Start    g: (%5.2lf, %5.2lf) -- (%5.2lf, %5.2lf) %d",
			g->xmin, g->ymin, g->xmax, g->ymax, g->Nu);
	cell->n_star.val = 0; cell->n_star.var = 0;
	cell->s_star.val = 0; cell->s_star.var = 0;
	cell->n_bar.val = 0; cell->n_bar.var = 0;
	cell->s_bar.val = 0; cell->s_bar.var = 0;
	cell->n = 0; cell->s = 0;
	cell->g_star_above = 0; cell->g_star_below = 0;
	cell->g_bar_above = 0; cell->g_bar_below = 0;
	answer_full(g, sn->theta, t, cell, sn->M,
			cell->xmin, cell->xmax, cell->ymin, cell->ymax);
}

void sn_convert_to_grid_root(const struct sensor_network *sn, struct grid *g)
{
	int i;
	grd_init(g, sn->xmin, sn->xmax, sn->ymin, sn->ymax, NULL);

	for (i = 0; i < sn->num_s; i++)
		grd_add_point(sn, g, i);
}

void sn_read_from_file(const char *fname, struct sensor_network *sn)
{
	FILE *f = fopen(fname, "r");
	int i;

	if (!f)
		die("Invalid db filename %s", fname);

	if(fscanf(f, "%lf%lf%lf%lf%lf%lf%d",
			&sn->xmin, &sn->ymin, &sn->xmax, &sn->ymax,
			&sn->M, &sn->theta, &sn->num_s) != 7)
		die("Cannot parse header");

	sn->sensors = calloc(sn->num_s, sizeof(sn->sensors[0]));
	if (!sn->sensors)
		die("Invalid count of sensors %d sz=%lu", sn->num_s, sn->num_s * sizeof(sn->sensors[0]));

	for (i = 0; i < sn->num_s; i++)
		if (fscanf(f, "%lf%lf%lf", &sn->sensors[i].x,
				&sn->sensors[i].y, &sn->sensors[i].val) != 3)
			break;

	if (i != sn->num_s) {
		printf("File truncated, read %d of %d sensors\n", i, sn->num_s);
		sn->num_s = i;
	}

	qsort(sn->sensors, sn->num_s, sizeof(sn->sensors[0]), sensor_cmp);
	fclose(f);
}

void sn_cleanup(const struct sensor_network *sn)
{
	free(sn->sensors);
}

void grd_debug(const struct sensor_network *sn, const struct grid *g, FILE *f, int depth)
{
	int i;

	/* sensors first */
	fprintf(f, "# sensors: x y val\n");
	for (i = 0; i < sn->num_s; i++)
		fprintf(f, "%5.2lf %5.2lf %5.2lf\n", sn->sensors[i].x,
				sn->sensors[i].y, sn->sensors[i].val);

	/* new region in datafile */
	fprintf(f, "\n\n");

	/* grid cell walls */
	fprintf(f, "# grid lines: x y dx dy\n");
	grd_print_cell_walls(g, f, depth);

	/* new region in datafile */
	fprintf(f, "\n\n");

	/* grid cell values */
	fprintf(f, "# cell values: x y n s n* vn* s* vs* n' vn' s' vs' n^ vn^ s^ vs^\n");
	grd_print_cell_vals(g, f, depth);
}

static void do_grd_debug0(const struct sensor_network *sn, const struct grid *g,
		int d)
{
	int i;

	printf("%*c (%6.2lf, %6.2lf) -- (%6.2lf, %6.2lf)", d, ' ',
			g->xmin, g->ymin, g->xmax, g->ymax);
	printf("| s=%9.2lf n=%8d %8.2lf", g->s, g->n, noisy_div(g->s, g->n, 0.01));
	printf("| s=%9.2lf n=%8.2lf %8.2lf", g->s_star.val, g->n_star.val, noisy_div(g->s_star.val, g->n_star.val, 0.01));
	printf("| s=%9.2lf n=%8.2lf %8.2lf", g->s_bar.val, g->n_bar.val, noisy_div(g->s_bar.val, g->n_bar.val, 0.01));
	printf("\n");
	for (i = 0; i < g->Nu * g->Nu; i++)
		do_grd_debug0(sn, &g->cells[i], d+1);
}

void grd_debug0(const struct sensor_network *sn, const struct grid *g)
{
	do_grd_debug0(sn, g, 1);
}

void grd_compute_noisy(const struct sensor_network *sn, struct grid *g,
		double epsilon, double beta, struct drand48_data *buffer)
{
	double epsilon_n, epsilon_s;

	epsilon_n = beta * epsilon;
	epsilon_s = epsilon - epsilon_n;

	debug(DEBUG_NOISE, "   epsilon: %lf, epsilon_n: %lf, epsilon_s %lf",
			epsilon, epsilon_n, epsilon_s);

	g->n_star.val = laplace_mechanism(g->n, epsilon_n, 1, buffer);
	g->s_star.val = laplace_mechanism(g->s, epsilon_s, sn->M, buffer);

	g->n_star.var = 2 / (epsilon_n * epsilon_n);
	g->s_star.var = 2 / (epsilon_s * epsilon_s);

	debug(DEBUG_NOISE, "Real: s=%10.2lf n=%8d", g->s, g->n);
	debug(DEBUG_NOISE, "Star: s=%10.2lf n=%11.2lf", g->s_star.val, g->n_star.val);
	debug(DEBUG_NOISE, "Var:  s=%10.2lf n=%11.2lf", g->s_star.var, g->n_star.var);
	debug(DEBUG_NOISE, "Magn: s=%10.2lf n=%11.2lf",
			fabs(g->s - g->s_star.val),
			fabs(g->n - g->n_star.val));
	debug(DEBUG_NOISE, "Delt: s=%10.2lf n=%11.2lf",
			sn->M * sqrt(g->s_star.var) * 1.6282, //log 10 / sqrt 2
			sqrt(g->n_star.var) * 1.6282);
}

void grd_split_cells(const struct sensor_network *sn, struct grid *g)
{
	double *xlimits, *ylimits, xdelta, ydelta;
	int i, j, c = 0, cnd;

	xdelta = (g->xmax - g->xmin) / g->Nu;
	ydelta = (g->ymax - g->ymin) / g->Nu;

	xlimits = calloc(1 + g->Nu, sizeof(xlimits[0]));
	if (!xlimits) die("Out of memory %d sz=%lu", 1 + g->Nu, (1 + g->Nu) * sizeof(xlimits[0]));
	ylimits = calloc(1 + g->Nu, sizeof(ylimits[0]));
	if (!ylimits) die("Out of memory %d sz=%lu", 1 + g->Nu, (1 + g->Nu) * sizeof(ylimits[0]));

	for (i = 0; i < g->Nu; i++) {
		xlimits[i] = g->xmin + i * xdelta;
		ylimits[i] = g->ymin + i * ydelta;
	}
	xlimits[g->Nu] = g->xmax;
	ylimits[g->Nu] = g->ymax;

	g->cells = calloc(g->Nu * g->Nu, sizeof(g->cells[0]));
	if (!g->cells) die("Out of memory Nu=%d sz(1)=%lu sz=%lu", g->Nu, sizeof(g->cells[0]), g->Nu * g->Nu * sizeof(g->cells[0]));
	for (i = 0; i < g->Nu; i++)
		for (j = 0; j < g->Nu; j++)
			grd_init(&g->cells[i * g->Nu + j],
					xlimits[i], xlimits[i+1],
					ylimits[j], ylimits[j+1], g);

	/* split points */
	for (i = 0; i < sn->num_s; i++) {
		for (j = 0; j < g->Nu * g->Nu; j++) {
			cnd = (g->cells[j].xmin <= sn->sensors[i].x) &&
			      (g->cells[j].ymin <= sn->sensors[i].y) &&
			      (g->cells[j].xmax  > sn->sensors[i].x) &&
			      (g->cells[j].ymax  > sn->sensors[i].y);
			if (cnd) {
				grd_add_point(sn, &g->cells[j], i);
				c++;
				break;
			}
		}
	}
	assert(c == g->n);

	free(xlimits);
	free(ylimits);
}

int grd_height(const struct grid *g)
{
	int i, ch, h = 0;

	if (!g->Nu)
		return 0;

	for (i = 0; i < g->Nu * g->Nu; i++) {
		ch = 1 + grd_height(&g->cells[i]);
		if (h < ch)
			h = ch;
	}

	return h;
}

int grd_level(const struct grid *g)
{
	if (!g->parent)
		return 0;
	return 1 + grd_level(g->parent);
}

double grd_size(const struct grid *g)
{
	return (g->xmax - g->xmin) * (g->ymax - g->ymin);
}

void grd_cleanup(const struct grid *g)
{
	if (g->cells) {
		int i;

		for (i = 0; i < g->Nu*g->Nu; i++)
			grd_cleanup(&g->cells[i]);
		free(g->cells);
	}
}

struct grid* grd_copy(const struct grid *original)
{
	struct grid *g = calloc(1, sizeof(*g));
	if (!g) die("Out of memory");
	grd_init(g, 0, 0, 0, 0, NULL);
	g->n = original->n;
	g->s = original->s;

	return g;
}

void grd_to_lrg(const struct sensor_network *sn, const struct grid *g,
		double res, struct low_res_grid_cell ***grid,
		int *xcnt, int *ycnt, double t)
{
	double xspan = g->xmax - g->xmin;
	double yspan = g->ymax - g->ymin;
	double v;
	int i, j;

	*xcnt = ceil(xspan / res);
	*ycnt = ceil(yspan / res);

	*grid = calloc(*xcnt, sizeof((*grid)[0]));
	for (i = 0; i < *xcnt; i++)
		(*grid)[i] = calloc(*ycnt, sizeof((*grid)[i][0]));

	for (i = 0; i < *xcnt; i++)
		for (j = 0; j < *ycnt; j++) {
			v = g->xmin + i * res;
			(*grid)[i][j].xmin = v;
			v += res;
			(*grid)[i][j].xmax = min(v, g->xmax);
			v = g->ymin + j * res;
			(*grid)[i][j].ymin = v;
			v += res;
			(*grid)[i][j].ymax = min(v, g->ymax);

			debug(DEBUG_GRD2LRG, "i: %3d j: %3d :: "
					"(%5.2lf, %5.2lf) -- (%5.2lf, %5.2lf)",
					i, j,
					(*grid)[i][j].xmin, (*grid)[i][j].ymin,
					(*grid)[i][j].xmax, (*grid)[i][j].ymax);

			answer(sn, g, &(*grid)[i][j], t);
		}
}

struct lrg_get_args {
	double t;
	double theta;
	double M;
	double p;
	double x;
};

static inline double lrg_get_n(struct low_res_grid_cell **grid,
		int x, int y, const struct lrg_get_args *a)
{
	(void) a;
	return grid[x][y].n;
}

static inline double lrg_get_s(struct low_res_grid_cell **grid,
		int x, int y, const struct lrg_get_args *a)
{
	(void) a;
	return grid[x][y].s;
}

static inline double lrg_get_rho(struct low_res_grid_cell **grid,
		int x, int y, const struct lrg_get_args *a)
{
	return noisy_div(grid[x][y].s, grid[x][y].n, a->t);
}

static inline double lrg_get_n_star(struct low_res_grid_cell **grid,
		int x, int y, const struct lrg_get_args *a)
{
	(void) a;
	return grid[x][y].n_star.val;
}

static inline double lrg_get_s_star(struct low_res_grid_cell **grid,
		int x, int y, const struct lrg_get_args *a)
{
	(void) a;
	return grid[x][y].s_star.val;
}

static inline double lrg_get_rho_star(struct low_res_grid_cell **grid,
		int x, int y, const struct lrg_get_args *a)
{
	return noisy_div(grid[x][y].s_star.val, grid[x][y].n_star.val, a->t);
}

static inline double lrg_get_n_bar(struct low_res_grid_cell **grid,
		int x, int y, const struct lrg_get_args *a)
{
	(void) a;
	return grid[x][y].n_bar.val;
}

static inline double lrg_get_s_bar(struct low_res_grid_cell **grid,
		int x, int y, const struct lrg_get_args *a)
{
	(void) a;
	return grid[x][y].s_bar.val;
}

static inline double lrg_get_rho_bar(struct low_res_grid_cell **grid,
		int x, int y, const struct lrg_get_args *a)
{
	return noisy_div(grid[x][y].s_bar.val, grid[x][y].n_bar.val, a->t);
}

static inline double lrg_get_shape(struct low_res_grid_cell **grid,
		int x, int y, const struct lrg_get_args *a)
{
	return noisy_div(grid[x][y].s, grid[x][y].n, a->t) >= a->theta;
}

static inline double lrg_get_shape_star(struct low_res_grid_cell **grid,
		int x, int y, const struct lrg_get_args *a)
{
	return noisy_div(grid[x][y].s_star.val, grid[x][y].n_star.val, a->t)
		>= a->theta;
}

static inline double lrg_get_shape_bar(struct low_res_grid_cell **grid,
		int x, int y, const struct lrg_get_args *a)
{
	return noisy_div(grid[x][y].s_bar.val, grid[x][y].n_bar.val, a->t)
		>= a->theta;
}

static inline double lrg_get_shape_bar_delta(struct low_res_grid_cell **grid,
		int x, int y, const struct lrg_get_args *a)
{
	double eta = grid[x][y].s_bar.val - grid[x][y].n_bar.val * a->theta;
	double var = grid[x][y].s_bar.var * a->M * a->M +
		grid[x][y].n_bar.val * a->theta * a->theta;
	double p = sqrt(var / a->p);
	if (eta < -p) return 0;
	if (eta > p) return 1;
	return 1 / (1 + exp(-5 * eta / p));
}

static inline double lrg_get_n_star_var(struct low_res_grid_cell **grid,
		int x, int y, const struct lrg_get_args *a)
{
	(void) a;
	return grid[x][y].n_star.var;
}

static inline double lrg_get_s_star_var(struct low_res_grid_cell **grid,
		int x, int y, const struct lrg_get_args *a)
{
	(void) a;
	return grid[x][y].s_star.var;
}

static inline double lrg_get_star_vote_above(struct low_res_grid_cell **grid,
		int x, int y, const struct lrg_get_args *a)
{
	(void) a;
	return grid[x][y].g_star_above;
}

static inline double lrg_get_star_vote_below(struct low_res_grid_cell **grid,
		int x, int y, const struct lrg_get_args *a)
{
	(void) a;
	return grid[x][y].g_star_below;
}

static inline double lrg_get_bar_vote_above(struct low_res_grid_cell **grid,
		int x, int y, const struct lrg_get_args *a)
{
	(void) a;
	return grid[x][y].g_bar_above;
}

static inline double lrg_get_bar_vote_below(struct low_res_grid_cell **grid,
		int x, int y, const struct lrg_get_args *a)
{
	(void) a;
	return grid[x][y].g_bar_below;
}

static inline double lrg_get_star_real_vote(struct low_res_grid_cell **grid,
		int x, int y, const struct lrg_get_args *a)
{
	double test;

	test = grid[x][y].g_star_above - grid[x][y].g_star_below;
	if (test >= -a->x) return test + 2 * a->x;
	return test - 2 * a->x;
}

static inline double lrg_get_bar_real_vote(struct low_res_grid_cell **grid,
		int x, int y, const struct lrg_get_args *a)
{
	double test;

	test = grid[x][y].g_bar_above - grid[x][y].g_bar_below;
	if (test >= -a->x) return test + 2 * a->x;
	return test - 2 * a->x;
}

static inline double lrg_get_star_vote(struct low_res_grid_cell **grid,
		int x, int y, const struct lrg_get_args *a)
{
	(void) a;
	double v = grid[x][y].g_star_above / (grid[x][y].g_star_above + grid[x][y].g_star_below);
	if (v >= 0.5) return v;
	return 0;
}

static inline double lrg_get_bar_vote(struct low_res_grid_cell **grid,
		int x, int y, const struct lrg_get_args *a)
{
	(void) a;
	double v = grid[x][y].g_bar_above / (grid[x][y].g_bar_above + grid[x][y].g_bar_below);
	if (v >= 0.5) return v;
	return 0;
}

static void lrg_print_val(struct low_res_grid_cell **grid, int xcnt, int ycnt,
		const struct lrg_get_args *a,
		FILE *f, const char *section_label,
		double (*get_field)(struct low_res_grid_cell **, int, int,
			const struct lrg_get_args *))
{
	int i, j;

	fprintf(f, "# %s\n", section_label);
	for (j = 0; j < ycnt; j++) {
		for (i = 0; i < xcnt; i++)
			fprintf(f, "%5.2f ", get_field(grid, i, j, a));
		fprintf(f, "\n");
	}
	fprintf(f, "\n");
}

void lrg_debug(struct low_res_grid_cell **grid, int xcnt, int ycnt, double t,
		double theta, double M, FILE *f)
{
	struct lrg_get_args a = {
		.t = t,
		.theta = theta,
		.M = M,
		.p = 0.9,
		.x = 0,
	};
	lrg_print_val(grid, xcnt, ycnt, &a, f, "n", lrg_get_n);
	lrg_print_val(grid, xcnt, ycnt, &a, f, "s", lrg_get_s);
	lrg_print_val(grid, xcnt, ycnt, &a, f, "rho", lrg_get_rho);
	lrg_print_val(grid, xcnt, ycnt, &a, f, "n_star", lrg_get_n_star);
	lrg_print_val(grid, xcnt, ycnt, &a, f, "s_star", lrg_get_s_star);
	lrg_print_val(grid, xcnt, ycnt, &a, f, "rho_star", lrg_get_rho_star);
	lrg_print_val(grid, xcnt, ycnt, &a, f, "n_bar", lrg_get_n_bar);
	lrg_print_val(grid, xcnt, ycnt, &a, f, "s_bar", lrg_get_s_bar);
	lrg_print_val(grid, xcnt, ycnt, &a, f, "rho_bar", lrg_get_rho_bar);
	lrg_print_val(grid, xcnt, ycnt, &a, f, "shape", lrg_get_shape);
	lrg_print_val(grid, xcnt, ycnt, &a, f, "shape_star", lrg_get_shape_star);
	lrg_print_val(grid, xcnt, ycnt, &a, f, "shape_bar", lrg_get_shape_bar);
	lrg_print_val(grid, xcnt, ycnt, &a, f, "n_star_var", lrg_get_n_star_var);
	lrg_print_val(grid, xcnt, ycnt, &a, f, "s_star_var", lrg_get_s_star_var);
	lrg_print_val(grid, xcnt, ycnt, &a, f, "shape_delta", lrg_get_shape_bar_delta);
	lrg_print_val(grid, xcnt, ycnt, &a, f, "star_vote_above", lrg_get_star_vote_above);
	lrg_print_val(grid, xcnt, ycnt, &a, f, "star_vote_below", lrg_get_star_vote_below);
	lrg_print_val(grid, xcnt, ycnt, &a, f, "bar_vote_above", lrg_get_bar_vote_above);
	lrg_print_val(grid, xcnt, ycnt, &a, f, "bar_vote_below", lrg_get_bar_vote_below);
	lrg_print_val(grid, xcnt, ycnt, &a, f, "star_real_vote", lrg_get_star_real_vote);
	lrg_print_val(grid, xcnt, ycnt, &a, f, "bar_real_vote", lrg_get_bar_real_vote);
	lrg_print_val(grid, xcnt, ycnt, &a, f, "star_vote", lrg_get_star_vote);
	lrg_print_val(grid, xcnt, ycnt, &a, f, "bar_vote", lrg_get_bar_vote);
}

void grd_average2(struct grid *a, const struct grid *b)
{
	a->n_ave = nv_average2(a->n_star, b->n_star);
	a->s_ave = nv_average2(a->s_star, b->s_star);
}

void grd_averagev(struct grid *g)
{
	struct noisy_val b;
	int i;

	/* update n */
	b.val = b.var = 0;
	for (i = 0; i < g->Nu*g->Nu; i++) {
		b.val += g->cells[i].n_ave.val;
		b.var += g->cells[i].n_ave.var;
	}
	g->n_ave = nv_average2(g->n_star, b);

	/* update s */
	b.val = b.var = 0;
	for (i = 0; i < g->Nu*g->Nu; i++) {
		b.val += g->cells[i].s_ave.val;
		b.var += g->cells[i].s_ave.var;
	}
	g->s_ave = nv_average2(g->s_star, b);
}

void grd_consistency(struct grid *g)
{
	struct noisy_val children_sum, add;
	int i, Nu2;

	if (!g->Nu) return;
	Nu2 = g->Nu * g->Nu;

	/* update n */
	children_sum.val = children_sum.var = 0;
	for (i = 0; i < Nu2; i++) {
		children_sum.val += g->cells[i].n_ave.val;
		children_sum.var += g->cells[i].n_ave.var;
	}
	add.val = (g->n_bar.val - children_sum.val) / Nu2;
	add.var = (g->n_bar.var + children_sum.var) / (Nu2 * Nu2);
	for (i = 0; i < Nu2; i++) {
		g->cells[i].n_bar.val = g->cells[i].n_ave.val + add.val;
		g->cells[i].n_bar.var = g->cells[i].n_ave.var + add.var;
	}

	/* update s */
	children_sum.val = children_sum.var = 0;
	for (i = 0; i < Nu2; i++) {
		children_sum.val += g->cells[i].s_ave.val;
		children_sum.var += g->cells[i].s_ave.var;
	}
	add.val = (g->s_bar.val - children_sum.val) / Nu2;
	add.var = (g->s_bar.var + children_sum.var) / (Nu2 * Nu2);
	for (i = 0; i < Nu2; i++) {
		g->cells[i].s_bar.val = g->cells[i].s_ave.val + add.val;
		g->cells[i].s_bar.var = g->cells[i].s_ave.var + add.var;
	}
}
