// Needed for FILE*
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

extern size_t bytecount;
extern int acls, verbose;
int create(FILE *dumpfile, FILE *tarfile);
int extract(FILE *tarfile, FILE *dumpfile);

#ifdef __cplusplus
}
#endif
