#ifndef _TEST_H
#define _TEST_H

struct sensor_network;
struct grid;

/** Tests without shape generation */
void test_san_grid(const struct sensor_network *sn, const struct grid *g, double t);

#endif
