#include <stdio.h>
#include <stdlib.h>

#include "globals.h"
#include "sn.h"

void sn_convert_to_grid_root(const struct sensor_network *sn, struct grid *g)
{
	int i;
	grd_init(g, sn->num_s, sn->xmin, sn->xmax, sn->ymin, sn->ymax);

	for (i = 0; i < sn->num_s; i++)
		grd_add_point(sn, g, i);

	grd_finish(g);
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

	fclose(f);
}

void sn_cleanup(const struct sensor_network *sn)
{
	free(sn->sensors);
}

void grd_init(struct grid *g, int sp,
		double xmin, double xmax, double ymin, double ymax)
{
	g->xmin = xmin;
	g->xmax = xmax;
	g->ymin = ymin;
	g->ymax = ymax;
	g->sens_ix = calloc(sp, sizeof(g->sens_ix[0]));
	if (!g->sens_ix) die("Out of memory sp=%d, sz=%lu", sp, sp * sizeof(g->sens_ix[0]));
	g->cells = NULL;
	g->Nu = 0;
	g->n = g->s = 0;
}

/* private */
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

void grd_print_cell_vals(const struct grid *g, FILE *f, int depth)
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
	fprintf(f, "%5.2lf %5.2lf %d %5.2lf %5.2lf %5.2lf %5.2lf %5.2lf %5.2lf %5.2lf %5.2lf %5.2lf %5.2lf %5.2lf %5.2lf %5.2lf\n",
			x, y, g->n, g->s,
			g->n_star.val, g->n_star.var, g->s_star.val, g->s_star.var,
			g->n_ave.val, g->n_ave.var, g->s_ave.val, g->s_ave.var,
			g->n_bar.val, g->n_bar.var, g->s_bar.val, g->s_bar.var);
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

void grd_add_point(const struct sensor_network *sn, struct grid *g, int ix)
{
	g->sens_ix[g->n++] = ix;
	g->s += sn->sensors[ix].val;
}

void grd_finish(struct grid *g)
{
	g->sens_ix = realloc(g->sens_ix, g->n * sizeof(g->sens_ix[0]));
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

	/* TODO: add negativity reduction models (see (83)):
	 *  .* -> h_t*.* -> .' + variance computation
	 *  .* ->  |.*|  -> .' + variance computation
	 */
}

void grd_split_cells(const struct sensor_network *sn, struct grid *g)
{
	double *xlimits, *ylimits, xdelta, ydelta;
	int i, j, k;

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
			grd_init(&g->cells[i * g->Nu + j], g->n,
					xlimits[i], xlimits[i+1],
					ylimits[j], ylimits[j+1]);

	/* split points */
	for (i = 0; i < g->n; i++) {
		struct sensor *s = &sn->sensors[g->sens_ix[i]];
		j = bsearch_i(&s->x, xlimits, 1 + g->Nu, sizeof(xlimits[0]), double_cmp);
		k = bsearch_i(&s->y, ylimits, 1 + g->Nu, sizeof(ylimits[0]), double_cmp);
		j = j < 0 ? -j-2 : j-1;
		k = k < 0 ? -k-2 : k-1;
		j = j < 0 ? 0 : j;
		k = k < 0 ? 0 : k;
		grd_add_point(sn, &g->cells[j * g->Nu + k], i);
	}

	for (i = 0; i < g->Nu * g->Nu; i++)
		grd_finish(&g->cells[i]);

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

void grd_cleanup(const struct grid *g)
{
	free(g->sens_ix);
	if (g->cells) {
		int i;

		for (i = 0; i < g->Nu*g->Nu; i++)
			grd_cleanup(&g->cells[i]);
		free(g->cells);
	}
}

static struct noisy_val nv_average2(struct noisy_val a, struct noisy_val b)
{
	double alpha = b.var / (a.var + b.var);
	struct noisy_val ret;

	ret.val = alpha * a.val + (1 - alpha) * b.val;
	ret.var = alpha * a.var;

	return ret;
}

struct grid* grd_copy(const struct grid *original)
{
	struct grid *g = calloc(1, sizeof(*g));
	if (!g) die("Out of memory");
	grd_init(g, 0, 0, 0, 0, 0);
	g->n = original->n;
	g->s = original->s;

	return g;
}

void grd_average2(struct grid *a, const struct grid *b)
{
	a->n_ave = nv_average2(a->n_star, b->n_star);
	a->s_ave = nv_average2(a->s_star, b->s_star);
}

void grd_averagev(struct grid *g)
{
#if COARSE_AVE
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
#else
	die("Invalid method! Don't use as of now!");
	int i;

	/* update n */
	g->n_ave.val = g->n_star.val / g->n_star.var;
	for (i = 0; i < g->Nu * g->Nu; i++)
		g->n_ave.val += g->cells[i].n_ave.val / g->cells[i].n_ave.var;
	g->n_ave.var = 1 / g->n_star.var;
	for (i = 0; i < g->Nu * g->Nu; i++)
		g->n_ave.var += 1 / g->cells[i].n_ave.var;
	g->n_ave.val /= g->n_ave.var;
	g->n_ave.var = 1 / g->n_ave.var;

	/* update s */
	g->s_ave.val = g->s_star.val / g->s_star.var;
	for (i = 0; i < g->Nu * g->Nu; i++)
		g->s_ave.val += g->cells[i].s_ave.val / g->cells[i].s_ave.var;
	g->s_ave.var = 1 / g->s_star.var;
	for (i = 0; i < g->Nu * g->Nu; i++)
		g->s_ave.var += 1 / g->cells[i].s_ave.var;
	g->s_ave.val /= g->s_ave.var;
	g->s_ave.var = 1 / g->s_ave.var;
#endif
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
