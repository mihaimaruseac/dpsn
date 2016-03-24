#ifndef _TEST_H
#define _TEST_H

struct low_res_grid_cell;
struct sensor_network;
struct grid;

/** Tests without shape generation */
void test_san_leaf_only(const struct sensor_network *sn, const struct grid *g, double t);
void test_san_cell(const struct sensor_network *sn, const struct grid *g, double t);

/** Tests of lrg */
void test_san_shape(const struct sensor_network *sn,
		struct low_res_grid_cell **grid, int xcnt, int ycnt, double t);
void test_san_votes(const struct sensor_network *sn,
		struct low_res_grid_cell **grid, int xcnt, int ycnt, double t);
void test_san_rel_votes(const struct sensor_network *sn,
		struct low_res_grid_cell **grid, int xcnt, int ycnt, double t);

#endif
