#ifndef _SN_H
#define _SN_H

struct sensor {
	double x, y;
	double val;
};

struct noisy_val {
	double val;
	double var;
};

struct sensor_network {
	/* bounding box */
	double xmin, xmax, ymin, ymax;
	/* max value of sensors */
	double M;
	/* threshold */
	double theta;
	/* num sensors and the sensors */
	int num_s;
	struct sensor *sensors;
};

struct grid {
	/* bounding box */
	double xmin, xmax, ymin, ymax;

	/* grid cells */
	struct grid *cells;
	/* grid zise */
	int Nu;
	/* grid parent */
	const struct grid *parent;

	/* real count and sum */
	int n;
	double s;

	/* epsilon value */
	double epsilon;

	/* noisy cound and sum */
	struct noisy_val n_star, s_star;
	/* weighted averages */
	struct noisy_val n_ave, s_ave;
	/* consistent estimates */
	struct noisy_val n_bar, s_bar;
};

struct low_res_grid_cell {
	double xmin, xmax, ymin, ymax;
	struct noisy_val n_star, s_star;
	struct noisy_val n_bar, s_bar;
	double n, s;
};

void sn_read_from_file(const char *fname, struct sensor_network *sn);
void sn_convert_to_grid_root(const struct sensor_network *sn, struct grid *g);
void sn_cleanup(const struct sensor_network *sn);

void grd_debug(const struct sensor_network *sn, const struct grid *g, FILE *f,
		int depth);
void grd_debug0(const struct sensor_network *sn, const struct grid *g);
void grd_compute_noisy(const struct sensor_network *sn, struct grid *g,
		double epsilon, double beta, struct drand48_data *buffer);
void grd_split_cells(const struct sensor_network *sn, struct grid *g);
int grd_height(const struct grid *g);
double grd_size(const struct grid *g);
void grd_cleanup(const struct grid *g);
struct grid* grd_copy(const struct grid *original);
void grd_to_lrg(const struct grid *g, double res,
		struct low_res_grid_cell ***grid,
		int *xcnt, int *ycnt);

void lrg_debug(struct low_res_grid_cell **grid, int xcnt, int ycnt, double t,
		double theta, double M, FILE *f);

/**
 * Will update _ave values in a to minimize their variance as a mean of _star
 * values in both a and b.
 */
void grd_average2(struct grid *a, const struct grid *b);
/**
 * Will update _ave values in g to minimize their variance as a mean of _star
 * values in g and _ave values in its children.
 */
void grd_averagev(struct grid *g);

/**
 * Does the mean consistency step ensuring that the _bar value in a cell is
 * the sum of the _bar values of the children. Redistributes the difference
 * between cell's _bar and sum of children's _ave, equally to each child
 */
void grd_consistency(struct grid *g);

#endif
