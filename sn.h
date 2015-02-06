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
	/* noisy cound and sum */
	int n_star, s_star;
	/* weighted averages */
	int n_ave, s_ave;
	/* consistent estimates */
	int n_bar, s_bar;
};

void sn_read_from_file(const char *fname, struct sensor_network *sn);
void sn_cleanup(const struct sensor_network *sn);
void sn_convert_to_grid_root(const struct sensor_network *sn, struct grid *g);

void grd_cleanup(const struct grid *g);

#endif
