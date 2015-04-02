#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "globals.h"
#include "sn.h"

static void grd_init(struct grid *g,
		double xmin, double xmax, double ymin, double ymax,
		const struct grid *parent)
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

static void answer_full(const struct grid *g, int arg,
		double xmin, double xmax, double ymin, double ymax,
		struct noisy_val *n_star, struct noisy_val *s_star,
		struct noisy_val *n_bar, struct noisy_val *s_bar,
		double *n, double *s)
{
	double ag, ar, f;
	int i;

	/* leaf or full cell coverage */
	if ((!g->Nu) ||
			((g->xmin == xmin) && (g->xmax == xmax) &&
			 (g->ymin == ymin) && (g->ymax == ymax))) {
		ag = (g->xmax - g->xmin) * (g->ymax - g->ymin);
		ar = (xmax - xmin) * (ymax - ymin);
		f = ar / ag;
		n_star->val += f * g->n_star.val; n_star->var += f * f * g->n_star.var;
		s_star->val += f * g->s_star.val; s_star->var += f * f * g->s_star.var;
		n_bar->val += f * g->n_bar.val; n_bar->var += f * f * g->n_bar.var;
		s_bar->val += f * g->s_bar.val; s_bar->var += f * f * g->s_bar.var;
		*n += f * g->n;
		*s += f * g->s;
		return;
	}

	for (i = 0; i < g->Nu * g->Nu; i++) {
		if ((g->cells[i].xmin >= xmax) || (g->cells[i].xmax <= xmin)) {
			i+= g->Nu - 1;
			continue;
		}
		// TODO: might implement a jump for y
		if (overlap(&g->cells[i], xmin, xmax, ymin, ymax))
			answer_full(&g->cells[i], arg + 5,
					max(xmin, g->cells[i].xmin),
					min(xmax, g->cells[i].xmax),
					max(ymin, g->cells[i].ymin),
					min(ymax, g->cells[i].ymax),
					n_star, s_star, n_bar, s_bar, n, s);
	}
}

static void answer(const struct grid *g,
		struct low_res_grid_cell *cell)
{
	cell->n_star.val = 0; cell->n_star.var = 0;
	cell->s_star.val = 0; cell->s_star.var = 0;
	cell->n_bar.val = 0; cell->n_bar.var = 0;
	cell->s_bar.val = 0; cell->s_bar.var = 0;
	cell->n = 0; cell->s = 0;
	answer_full(g, 1, cell->xmin, cell->xmax, cell->ymin, cell->ymax,
			&cell->n_star, &cell->s_star,
			&cell->n_bar, &cell->s_bar,
			&cell->n, &cell->s);
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
	printf("| s=%9.2lf n=%8d ", g->s, g->n);
	printf("| s=%9.2lf n=%8.2lf ", g->s_star.val, g->n_star.val);
	printf("| s=%9.2lf n=%8.2lf ", g->s_bar.val, g->n_bar.val);
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

	g->n_star.val = laplace_mechanism(g->n, epsilon_n, 1, buffer);
	g->s_star.val = laplace_mechanism(g->n, epsilon_s, sn->M, buffer);

	g->n_star.var = 2 / (epsilon_n * epsilon_n);
	g->s_star.var = 2 / (epsilon_s * epsilon_s);
}

void grd_split_cells(const struct sensor_network *sn, struct grid *g)
{
	double *xlimits, *ylimits, xdelta, ydelta;
	struct sensor tmp_s;
	int i, j, k, st, en;

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
	tmp_s.x = g->xmin; tmp_s.y = g->ymin;
	st = bsearch_i(&tmp_s, sn->sensors, sn->num_s, sizeof(sn->sensors[0]), sensor_cmp);
	tmp_s.x = g->xmax; tmp_s.y = g->ymax;
	en = bsearch_i(&tmp_s, sn->sensors, sn->num_s, sizeof(sn->sensors[0]), sensor_cmp);
	st = st < 0 ? -st-1 : st-1;
	en = en < 0 ? -en-1 : en-1;
	for (i = st; i < en; i++) {
		struct sensor *s = &sn->sensors[i];
		j = bsearch_i(&s->x, xlimits, 1 + g->Nu, sizeof(xlimits[0]), double_cmp);
		k = bsearch_i(&s->y, ylimits, 1 + g->Nu, sizeof(ylimits[0]), double_cmp);
		j = j < 0 ? -j-2 : j-1;
		k = k < 0 ? -k-2 : k-1;
		if (j < 0 || k < 0 || j >= g->Nu || k >= g->Nu)
			continue;
		grd_add_point(sn, &g->cells[j * g->Nu + k], i);
	}

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

void grd_to_lrg(const struct grid *g, double res,
		struct low_res_grid_cell ***grid,
		int *xcnt, int *ycnt)
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
			answer(g, &(*grid)[i][j]);
		}
}

struct lrg_get_args {
	double t;
	double theta;
	double M;
	double p;
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

static void lrg_print_val(struct low_res_grid_cell **grid, int xcnt, int ycnt,
		const struct lrg_get_args *a,
		FILE *f, const char *section_label,
		double (*get_field)(struct low_res_grid_cell **, int, int,
			const struct lrg_get_args *))
{
	int i, j;

	// TODO: print coordinates?
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
	// TODO: shape_star_prob, shape_bar_prob
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
