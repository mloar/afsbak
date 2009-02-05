#define _FILE_OFFSET_BITS 64

#include <errno.h>
#include <unistd.h>

#include "common.h"

size_t bytecount = 0;
int acls = 0, verbose = 0;

void usage(const char *argv0)
{
    fprintf(stderr, "Usage: %s -acfv\n", argv0);
}

int
main(argc, argv)
    int argc;
    char **argv;
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
                usage(argv[0]);
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
                return 1;
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
