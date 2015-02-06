#ifndef _SANITIZATION_H
#define _SANITIZATION_H

struct sensor_network;
struct grid;

void sanitize_ug(const struct sensor_network *sn, struct grid *g,
		double epsilon, double alpha, double beta,
		double K, int Nt, int seed);
void sanitize_ag(const struct sensor_network *sn, struct grid *g,
		double epsilon, double alpha, double beta,
		double K, int Nt, int seed);
void sanitize_agt(const struct sensor_network *sn, struct grid *g,
		double epsilon, double alpha, double beta,
		double K, int Nt, int seed);

#endif

