#ifndef _SN_H
#define _SN_H

struct sensor {
	double x, y;
	double val;
};

struct sensor_network {
	double xmin, xmax, ymin, ymax;
	double M;
	double theta;
	int num_s;
	struct sensor *sensors;
};

struct grid {
	double xmin, xmax, ymin, ymax;
};

void sn_read_from_file(const char *fname, struct sensor_network *sn);
void sn_cleanup(const struct sensor_network *sn);

void sn_convert_to_grid_root(const struct sensor_network *sn, struct grid *g);

#endif
