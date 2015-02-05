#ifndef _SN_H
#define _SN_H

struct sensor {
	double x, y;
	double val;
};

struct sensor_network {
	int sz;
	double M;
	double theta;
	struct sensor *sensors;
};

void sn_read_from_file(const char *fname, struct sensor_network *sn);

#endif
