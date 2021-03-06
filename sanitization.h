#ifndef _SANITIZATION_H
#define _SANITIZATION_H

struct sensor_network;
struct grid;

enum method {UG, AG, AGS};

/**
 * All sanitization methods will fill in the grid tree (depending on the
 * methods it is a tree, a simple grid (two level tree), a two layer grid
 * (three level tree). For each node in the tree all noisy values and their
 * variances are computed, including the case when such values are not defined
 * (no postprocessing is done).
 *
 * Hence, the user can just use <>_bar and var_<>_bar values, regardless of
 * the sanitization method used.
 */
void sanitize(const struct sensor_network *sn, struct grid *g,
		double epsilon, double alpha, double beta, double gamma,
		double K, int Nt, int max_depth,
		int seed, enum method method);
#endif
