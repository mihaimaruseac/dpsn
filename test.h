#ifndef _TEST_H
#define _TEST_H

struct sensor_network;
struct grid;

/** Tests without shape generation */
void test_san_leaf_only(const struct sensor_network *sn, const struct grid *g);
void test_san_cell(const struct sensor_network *sn, const struct grid *g);

#endif
