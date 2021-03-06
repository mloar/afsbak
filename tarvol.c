/*
 * Written by Matthew Loar <matthew@loar.name>
 * This work is hereby placed in the public domain by its author.
 */

#define _FILE_OFFSET_BITS 64

#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

#include "common.h"

uintmax_t bytecount = 0;
int acls = 0, verbose = 0;

/* Print a usage message and exit */
static void usage(const char *arg, int status, const char *msg)
{
    if (msg) fprintf(stderr, "%s: %s\n", arg, msg);
    fprintf(stderr, "Usage: %s [options] [file]\n", arg);
    fprintf(stderr, "  -a     Add ACL restore script to archive\n");
    fprintf(stderr, "  -c     Create archive (vos dump to tar)\n");
    fprintf(stderr, "  -f     Use archive file or device ARCHIVE\n");
    fprintf(stderr, "  -h     Print this help message\n");
    fprintf(stderr, "  -v     Verbose mode (multiple for greater verbosity)\n");
    fprintf(stderr, "  -x     Extract archive (tar to vos dump) (not implemented)\n");
    exit(status);
}

int main(int argc, char **argv)
{
    int arg, operation = 0;
    const char *fileparam = NULL;
    while ((arg = getopt(argc, argv, "acf:hv")) != -1)
    {
        switch (arg)
        {
            case 'a':
                acls = 1;
                break;
            case 'h':
                usage(argv[0], 0, NULL);
                return 1;
                break;
            case 'x':
                if (operation)
                {
                    usage(argv[0], 1, "Both -c and -x were specified");
                }
                operation = 'x';
                break;
            case 'c':
                if (operation)
                {
                    usage(argv[0], 1, "Both -c and -x were specified");
                }
                operation = 'c';
                break;
            case 'v':
                verbose++;
                break;
            case 'f':
                fileparam = optarg;
                break;
            case '?':
                usage(argv[0], 1, NULL);
                break;
        }
    }

    if (!operation)
    {
        usage(argv[0], 1, "Either -c or -x is required\n");
        return 1;
    }
    else if (operation == 'c')
    {
        FILE *dumpfile = stdin, *tarfile = stdout;
        if (fileparam)
        {
            tarfile = fopen(fileparam, "w");
            if (!tarfile) {
                fprintf(stderr, "Cannot open '%s'. Code = %d\n",
                        fileparam, errno);
            }
        }

        return create(dumpfile, tarfile);
    }
    else if (operation == 'x')
    {
        usage(argv[0], 1, "-x not yet implemented\n");
        return 1;
    }

    return 0;
}
