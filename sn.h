#ifndef _SN_H
#define _SN_H

struct sensor {
	double x, y;
	double val;
};

struct sensor_network {
	double sz;
	double M;
	double theta;
	int num_s;
	struct sensor *sensors;
};

void sn_read_from_file(const char *fname, struct sensor_network *sn);
void sn_cleanup(const struct sensor_network *sn);

#endif
