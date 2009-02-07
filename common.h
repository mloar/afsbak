// Needed for FILE*
#include <stdio.h>
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
