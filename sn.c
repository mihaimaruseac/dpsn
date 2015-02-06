#include <stdio.h>
#include <stdlib.h>

#include "globals.h"
#include "sn.h"

void sn_convert_to_grid_root(const struct sensor_network *sn, struct grid *g)
{
}

void sn_read_from_file(const char *fname, struct sensor_network *sn)
{
	FILE *f = fopen(fname, "r");
	int i;

	if (!f)
		die("Invalid db filename %s", fname);

	if(fscanf(f, "%lf%lf%lf%lf%lf%lf%d",
			&sn->xmin, &sn->ymin, &sn->xmax, &sn->ymax,
			&sn->M, &sn->theta, &sn->num_s) != 7)
		die("Cannot parse header");

	sn->sensors = calloc(sn->num_s, sizeof(sn->sensors[0]));
	if (sn->sensors == NULL)
		die("Invalid count of sensors");

	for (i = 0; i < sn->num_s; i++)
		if (fscanf(f, "%lf%lf%lf", &sn->sensors[i].x,
				&sn->sensors[i].y, &sn->sensors[i].val) != 3)
			break;

	if (i != sn->num_s) {
		printf("File truncated, read %d of %d sensors\n", i, sn->num_s);
		sn->num_s = i;
	}

	fclose(f);
}

void sn_cleanup(const struct sensor_network *sn)
{
	free(sn->sensors);
}
