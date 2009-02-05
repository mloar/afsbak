#define _FILE_OFFSET_BITS 64

#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

#include "common.h"

size_t bytecount = 0;
int acls = 0, verbose = 0;

/* Print a usage message and exit */
static void usage(const char *arg, int status, const char *msg)
{
    if (msg) fprintf(stderr, "%s: %s\n", arg, msg);
    fprintf(stderr, "Usage: %s [options] [file]\n", arg);
    fprintf(stderr, "  -a     Add ACL restore script to archive\n");
    fprintf(stderr, "  -c     Not implemented\n");
    fprintf(stderr, "  -f     Use archive file or device ARCHIVE\n");
    fprintf(stderr, "  -h     Print this help message\n");
    fprintf(stderr, "  -v     Verbose mode\n");
    fprintf(stderr, "  -x     Extract files from archive (not implemented)\n");
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
                usage(argv[0], 0, 0);
                return 1;
                break;
            case 'x':
                if (operation)
                {
                    fprintf(stderr, "Both -c and -x were specified\n");
                    return 1;
                }
                operation = 'x';
                break;
            case 'c':
                if (operation)
                {
                    fprintf(stderr, "Both -c and -x were specified\n");
                    return 1;
                }
                operation = 'c';
                break;
            case 'v':
                verbose++;
                break;
            case 'f':
                fileparam = optarg;
                break;
            default:
                usage(argv[0], 1, "Invalid option!");
        }
    }

    if (!operation)
    {
        fprintf(stderr, "Either -c or -x is required\n");
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
        fprintf(stderr, "extract not implemented yet\n");
        return 1;
    }

    return 0;
}
