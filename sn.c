#include <stdio.h>
#include <stdlib.h>

#include "globals.h"
#include "sn.h"

void sn_convert_to_grid_root(const struct sensor_network *sn, struct grid *g)
{
	int i;

	g->xmin = sn->xmin;
	g->xmax = sn->xmax;
	g->ymin = sn->ymin;
	g->ymax = sn->ymax;

	g->sens_ix = calloc(sn->num_s, sizeof(g->sens_ix[0]));
	for (i = 0; i < sn->num_s; i++) {
		g->sens_ix[i] = i;
		g->s += sn->sensors[i].val;
	}

	g->n = sn->num_s;

	g->cells = NULL;
	g->Nu = 0;
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
		die("Invalid count of sensors");

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

void grd_compute_real(const struct sensor_network *sn, struct grid *g)
{
	int i = 0;

	g->s = 0;
	for (i = 0; i < g->n; i++)
		g->s += sn->sensors[g->sens_ix[i]].val;
}

void grd_compute_noisy(const struct sensor_network *sn, struct grid *g,
		double epsilon, double beta, struct drand48_data *buffer)
{
	g->n_star = laplace_mechanism(g->n, beta * epsilon, 1, buffer);
	g->s_star = laplace_mechanism(g->n, (1 - beta * epsilon), sn->M, buffer);
}

void grd_split_cells(const struct sensor_network *sn, struct grid *g)
{
	g->cells = calloc(g->Nu * g->Nu, sizeof(g->cells[0]));
	if (!g->cells)
		die("Invalid cell size");
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
