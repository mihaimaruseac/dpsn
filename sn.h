#ifndef _SN_H
#define _SN_H

struct sensor {
	double x, y;
	double val;
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
	/* indices to the sensors inside the grid */
	int *sens_ix;

	/* grid cells */
	struct grid *cells;
	/* grid zise */
	int Nu;

	/* real count and sum */
	int n, s;

	/* epsilon value */
	double epsilon;

	/* noisy cound and sum */
	double n_star, s_star;
	/* weighted averages */
	double n_ave, s_ave;
	/* consistent estimates */
	double n_bar, s_bar;

	/* variances for the above 6 values, for s scaled down by M^2 */
	double var_n_star, var_s_star;
	double var_n_ave, var_s_ave;
	double var_n_bar, var_s_bar;
};

void sn_read_from_file(const char *fname, struct sensor_network *sn);
void sn_convert_to_grid_root(const struct sensor_network *sn, struct grid *g);
void sn_cleanup(const struct sensor_network *sn);

void grd_init(struct grid *g, int sp,
		double xmin, double xmax, double ymin, double ymax);
void grd_add_point(const struct sensor_network *sn, struct grid *g, int ix);
void grd_finish(struct grid *g);
void grd_compute_real(const struct sensor_network *sn, struct grid *g);
void grd_compute_noisy(const struct sensor_network *sn, struct grid *g,
		double epsilon, double beta, struct drand48_data *buffer);
void grd_split_cells(const struct sensor_network *sn, struct grid *g);
void grd_cleanup(const struct grid *g);

#endif
