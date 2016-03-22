#ifndef _GLOBALS_H
#define _GLOBALS_H

#define die(s, ...) \
	do {\
		fprintf(stderr, "[%s: %s %d] "s"\n", __FILE__, \
				__func__, __LINE__, ##__VA_ARGS__); \
		exit(EXIT_FAILURE); \
	} while (0)

#ifndef VERBOSE_DEBUG
#define VERBOSE_DEBUG 1
#endif
#ifndef DEBUG_ALL
#define DEBUG_ALL 0
#endif

#if VERBOSE_DEBUG
#define debug(cls, fmt, ...) \
	do {\
		if (cls || DEBUG_ALL)\
			printf("[%s: %s %d] "fmt"\n", __FILE__, \
				__func__, __LINE__, ##__VA_ARGS__); \
	} while (0)
#else
#define debug(...)
#endif

#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))

struct drand48_data;

/* qsort functions for integer comparisons */
int int_cmp(const void *a, const void *b);
int int_cmp_r(const void *a, const void *b);
int double_cmp(const void *a, const void *b);
int double_cmp_r(const void *a, const void *b);

void init_rng(long int seed, struct drand48_data *buffer);

/* Laplace mechanism */
double laplace_mechanism(double x, double eps, double sens,
		struct drand48_data *buffer);

/* version of bsearch which returns the rightmost insertion index:
 * 	- if element is in array return index, like normal bsearch
 * 	- else if element could be inserted at position k return -k-1
 */
int bsearch_i(const void *key, const void *base, size_t nmemb, size_t size,
		int (*compar)(const void *, const void *));

/* divide x by y only if y is at least t (avoid noisy division by 0 */
double noisy_div(double x, double y, double t);

#endif
