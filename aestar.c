/*
 *  Copyright (c) 2009 Matthew Loar.
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to
 *  deal with the Software without restriction, including without limitation the
 *  rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 *  sell copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimers.
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimers in the documentation
 *    and/or other materials provided with the distribution.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 *  WITH THE SOFTWARE.
 *
 *  Abstract:
 *      Encrypts or decrypts the file data in a tar archive using the aespipe
 *      utility.
 *
 */

#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <tar.h>

static int verbose = 0;

struct Tar
{
    char name[100];
    char mode[8];
    char uid[8];
    char gid[8];
    char size[12];
    char mtime[12];
    char chksum[8];
    char typeflag;
    char linkname[100];
    char magic[6];
    char version[2];
    char uname[32];
    char gname[32];
    char devmajor[8];
    char devminor[8];
    char prefix[167];
};

int
ReadTarHeader(struct Tar *tar)
{
    size_t i;
    unsigned int chksum, mychksum = 0;
    if (fread(tar, 1, sizeof(struct Tar), stdin) != sizeof(struct Tar))
    {
        if (feof(stdin))
        {
            fprintf(stderr, "end of file\n");
            return 1;
        }
        fprintf(stderr, "Could not read tar header\n");
        return 1;
    }

    sscanf(tar->chksum, "%07o", &chksum);
    for (i = 0; i < sizeof(struct Tar); i++)
    {
        mychksum += *(((unsigned char*)tar) + i);
    }

    if (!mychksum)
    {
        if (verbose > 1)
        {
            fprintf(stderr, "end of archive\n");
        }
        return 1;
    }

    for (i = 0; i < 8; i++)
    {
        mychksum -= *(unsigned char*)&tar->chksum[i];
    }
    mychksum += ' '*8;

    if (tar->magic[0] == 'a')
        mychksum += 'u' - 'a';

    if (mychksum != chksum)
    {
        fprintf(stderr, "checksums do not match\n");
        return 1;
    }

    if (verbose)
    {
        fprintf(stderr, "%s\n", tar->name);
    }
    return 0;
}

/* Print a usage message and exit */
static void usage(const char *arg, int status, const char *msg)
{
    if (msg) fprintf(stderr, "%s: %s\n", arg, msg);
    fprintf(stderr, "Usage: %s [options] -k file\n", arg);
    fprintf(stderr, "  -d           Decrypt (default is encrypt)\n");
    fprintf(stderr, "  -h           Print this help message\n");
    fprintf(stderr, "  -k file      Use passphrase in file\n");
    fprintf(stderr, "  -v           Verbose (multiple for more verbosity)\n");
    exit(status);
}

int
main(int argc, char* argv[])
{
    int arg, decrypt = 0;
    char cmd[MAXPATHLEN + 20];
    const char *fileparam = NULL;
    while ((arg = getopt(argc, argv, "dhk:v")) != -1)
    {
        switch (arg)
        {
            case 'd':
                decrypt = 1;
                break;
            case 'h':
                usage(argv[0], 0, NULL);
                break;
            case 'k':
                fileparam = optarg;
                break;
            case 'v':
                verbose++;
                break;
            case '?':
                usage(argv[0], 1, NULL);
                break;
            default:
                usage(argv[0], 1, "option parsing failed");
                break;
        }
    }

    if (!fileparam)
    {
        usage(argv[0], 1, "passphrase file must be specified");
    }

    {
        int ret = snprintf(cmd, MAXPATHLEN + 20, "aespipe %s -P %s",
                decrypt ? "-d" : "", fileparam);

        if (ret < 0 || ret >= MAXPATHLEN + 20)
        {
            fprintf(stderr, "Could not format command string\n");
            return 1;
        }
    }


    struct Tar tar;
    while (!feof(stdin) && !ReadTarHeader(&tar))
    {
        uintmax_t size = 0LL;
        char buf[512];
        FILE *aespipe;

        if (tar.size[0] & -128)
        {
            int i;
            /* GNU size extension */
            for (i = 1; i < sizeof(tar.size); i++)
            {
                size = size << 8;
                size += tar.size[i];
            }
        }
        else
        {
            /* Make sure size is NULL-terminated */
            strncpy(buf, tar.size, sizeof(tar.size));
            buf[sizeof(tar.size)] = 0;

            sscanf(buf, "%11llo", &size);
        }
        if (verbose > 1)
        {
            fprintf(stderr, "%s %11llu bytes of data\n",
                    decrypt ? "decrypting" : "encrypting", size);
        }

        /*
         * Our output will look like a tar file, but data will be lost if a tar
         * program is used to extract its contents.  So we change the magic,
         * which also breaks the checksum, which should prevent any half-sane
         * tar program from trying to extract the contents.
         */
        if (decrypt)
            tar.magic[0] = 'u';
        else
            tar.magic[0] = 'a';

        fwrite(&tar, 1, sizeof(struct Tar), stdout);
        fflush(stdout);
        if (size)
        {
            aespipe = popen(cmd, "w");

            while (size)
            {
                /*
                 * CBC mode requires 512-byte blocks.  Luckily, the tar format
                 * also uses 512-byte blocks.  We leave the real file size in
                 * the header, so while the result may look like a tar file,
                 * using tar to extract the contents would result in data loss.
                 */
                if (fread(buf, 1, 512, stdin) != 512)
                {
                    if (feof(stdin))
                    {
                        fprintf(stderr, "encountered end-of-file\n");
                        return 1;
                    }

                    perror("fread");
                    return 1;
                }

                fwrite(buf, 1, 512, aespipe);

                if (size > 512LL) size -= 512LL; else size = 0LL;
            }

            if (pclose(aespipe) != 0)
            {
                fprintf(stderr, "aespipe failed\n");
                return 1;
            }
        }
    }

    unsigned char zeros[1024];
    memset(zeros, 0, 1024);
    fwrite(zeros, 1, 1024, stdout);
    return 0;
}
