/*
 * Written by Matthew Loar <matthew@loar.name>
 * This work is hereby placed in the public domain by its author.
 */

/* Needed for FILE* */
#include <stdio.h>
/* Needed for uintmax_t */
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

extern uintmax_t bytecount;
extern int acls, verbose;
int create(FILE *dumpfile, FILE *tarfile);
int extract(FILE *tarfile, FILE *dumpfile);

#ifdef __cplusplus
}
#endif
