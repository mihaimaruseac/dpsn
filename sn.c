#include <stdio.h>
#include <stdlib.h>

#include "globals.h"
#include "sn.h"

void sn_read_from_file(const char *fname, struct sensor_network *sn)
{
	FILE *f = fopen(fname, "r");

	if (!f)
		die("Invalid transaction filename %s", fname);

	fclose(f);
}
