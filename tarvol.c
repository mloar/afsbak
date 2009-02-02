/*
 * Copyright 2000, International Business Machines Corporation and others.
 * All Rights Reserved.
 * 
 * This software has been released under the terms of the IBM Public
 * License.  For details, see the LICENSE file in the top-level source
 * directory or online at http://www.openafs.org/dl/license10.html
 */

#include <afs/afsint.h>
#include <afs/ihandle.h>
#include "lock.h"
#include <afs/vnode.h>
#include <afs/volume.h>
#include "dump.h"

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <tar.h>
#include "storage.h"

FILE *dumpfile;
int bytecount = 0;
int verbose = 0;

    afs_int32
readvalue(size)
{
    afs_int32 value, s;
    int code;
    char *ptr;

    value = 0;
    ptr = (char *)&value;

    s = sizeof(value) - size;
    if (size < 0) {
        fprintf(stderr, "Too much data in afs_int32\n");
        return 0;
    }

    code = fread(&ptr[s], 1, size, dumpfile);
    if (code != size)
        fprintf(stderr, "Code = %d; Errno = %d\n", code, errno);

    return (value);
}

    char
readchar()
{
    char value;
    int code;

    value = '\0';
    code = fread(&value, 1, 1, dumpfile);
    if (code != 1)
        fprintf(stderr, "Code = %d; Errno = %d\n", code, errno);

    return (value);
}

#define BUFSIZE 16384
char buf[BUFSIZE];

    void
readdata(buffer, size)
    char *buffer;
    afs_sfsize_t size;
{
    int code;
    afs_int32 s;

    if (!buffer) {
        while (size > 0) {
            s = (afs_int32) ((size > BUFSIZE) ? BUFSIZE : size);
            code = fread(buf, 1, s, dumpfile);
            if (code != s)
                fprintf(stderr, "Code = %d; Errno = %d\n", code, errno);
            size -= s;
        }
    } else {
        code = fread(buffer, 1, size, dumpfile);
        if (code != size) {
            if (code < 0)
                fprintf(stderr, "Code = %d; Errno = %d\n", code, errno);
            else
                fprintf(stderr, "Read %d bytes out of %lld\n", code, (afs_uintmax_t)size);
        }
        if ((code >= 0) && (code < BUFSIZE))
            buffer[size] = 0;   /* Add null char at end */
    }
}

    afs_int32
ReadDumpHeader(dh)
    struct DumpHeader *dh;     /* Defined in dump.h */
{
    int i, done;
    char tag, c;
    afs_int32 magic;

    /*  memset(&dh, 0, sizeof(dh)); */

    magic = ntohl(readvalue(4));
    dh->version = ntohl(readvalue(4));

    done = 0;
    while (!done) {
        tag = readchar();
        switch (tag) {
            case 'v':
                dh->volumeId = ntohl(readvalue(4));
                break;

            case 'n':
                for (i = 0, c = 'a'; c != '\0'; i++) {
                    dh->volumeName[i] = c = readchar();
                }
                dh->volumeName[i] = c;
                break;

            case 't':
                dh->nDumpTimes = ntohl(readvalue(2)) >> 1;
                for (i = 0; i < dh->nDumpTimes; i++) {
                    dh->dumpTimes[i].from = ntohl(readvalue(4));
                    dh->dumpTimes[i].to = ntohl(readvalue(4));
                }
                break;

            default:
                done = 1;
                break;
        }
    }

    return ((afs_int32) tag);
}

struct volumeHeader {
    afs_int32 volumeId;
    char volumeName[100];
    afs_int32 volType;
    afs_int32 uniquifier;
    afs_int32 parentVol;
    afs_int32 cloneId;
    afs_int32 maxQuota;
    afs_int32 minQuota;
    afs_int32 diskUsed;
    afs_int32 fileCount;
    afs_int32 accountNumber;
    afs_int32 owner;
    afs_int32 creationDate;
    afs_int32 accessDate;
    afs_int32 updateDate;
    afs_int32 expirationDate;
    afs_int32 backupDate;
    afs_int32 dayUseDate;
    afs_int32 dayUse;
    afs_int32 weekCount;
    afs_int32 weekUse[100];     /* weekCount of these */
    char motd[1024];
    int inService;
    int blessed;
    char message[1024];
};

    afs_int32
ReadVolumeHeader(count)
    afs_int32 count;
{
    struct volumeHeader vh;
    int i, done;
    char tag, c;

    /*  memset(&vh, 0, sizeof(vh)); */

    done = 0;
    while (!done) {
        tag = readchar();
        switch (tag) {
            case 'i':
                vh.volumeId = ntohl(readvalue(4));
                break;

            case 'v':
                ntohl(readvalue(4));        /* version stamp - ignore */
                break;

            case 'n':
                for (i = 0, c = 'a'; c != '\0'; i++) {
                    vh.volumeName[i] = c = readchar();
                }
                vh.volumeName[i] = c;
                break;

            case 's':
                vh.inService = ntohl(readvalue(1));
                break;

            case 'b':
                vh.blessed = ntohl(readvalue(1));
                break;

            case 'u':
                vh.uniquifier = ntohl(readvalue(4));
                break;

            case 't':
                vh.volType = ntohl(readvalue(1));
                break;

            case 'p':
                vh.parentVol = ntohl(readvalue(4));
                break;

            case 'c':
                vh.cloneId = ntohl(readvalue(4));
                break;

            case 'q':
                vh.maxQuota = ntohl(readvalue(4));
                break;

            case 'm':
                vh.minQuota = ntohl(readvalue(4));
                break;

            case 'd':
                vh.diskUsed = ntohl(readvalue(4));
                break;

            case 'f':
                vh.fileCount = ntohl(readvalue(4));
                break;

            case 'a':
                vh.accountNumber = ntohl(readvalue(4));
                break;

            case 'o':
                vh.owner = ntohl(readvalue(4));
                break;

            case 'C':
                vh.creationDate = ntohl(readvalue(4));
                break;

            case 'A':
                vh.accessDate = ntohl(readvalue(4));
                break;

            case 'U':
                vh.updateDate = ntohl(readvalue(4));
                break;

            case 'E':
                vh.expirationDate = ntohl(readvalue(4));
                break;

            case 'B':
                vh.backupDate = ntohl(readvalue(4));
                break;

            case 'O':
                for (i = 0, c = 'a'; c != '\0'; i++) {
                    vh.message[i] = c = readchar();
                }
                vh.volumeName[i] = c;
                break;

            case 'W':
                vh.weekCount = ntohl(readvalue(2));
                for (i = 0; i < vh.weekCount; i++) {
                    vh.weekUse[i] = ntohl(readvalue(4));
                }
                break;

            case 'M':
                for (i = 0, c = 'a'; c != '\0'; i++) {
                    vh.motd[i] = c = readchar();
                }
                break;

            case 'D':
                vh.dayUseDate = ntohl(readvalue(4));
                break;

            case 'Z':
                vh.dayUse = ntohl(readvalue(4));
                break;

            default:
                done = 1;
                break;
        }
    }

    return ((afs_int32) tag);
}

struct vNode {
    afs_int32 vnode;
    afs_int32 uniquifier;
    afs_int32 type;
    afs_int32 linkCount;
    afs_int32 dataVersion;
    afs_int32 unixModTime;
    afs_int32 servModTime;
    afs_int32 author;
    afs_int32 owner;
    afs_int32 group;
    afs_int32 modebits;
    afs_int32 parent;
    char acl[192];
#ifdef notdef
    struct acl_accessList {
        int size;               /*size of this access list in bytes, including MySize itself */
        int version;            /*to deal with upward compatibility ; <= ACL_ACLVERSION */
        int total;
        int positive;           /* number of positive entries */
        int negative;           /* number of minus entries */
        struct acl_accessEntry {
            int id;             /*internally-used ID of user or group */
            int rights;         /*mask */
        } entries[100];
    } acl;
#endif
    afs_sfsize_t dataSize;
};

#define MAXNAMELEN 256

void
WriteVNodeTarHeader(const char *dir, struct vNode *vn)
{
    unsigned int i;
    unsigned int chksum = 0;
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
    } tarheader;

    memset(&tarheader, 0, sizeof(struct Tar));
    memset(tarheader.chksum, ' ', 8);
    strncpy(tarheader.prefix, dir, 167);
    if (vn->type == 1 /* file */) {
        strncpy(tarheader.name, get(dir, vn->vnode), 100);
        tarheader.typeflag = REGTYPE;
    } else if (vn->type == 2 /* directory */) {
        tarheader.typeflag = DIRTYPE;
    } else if (vn->type == 3 /* symlink or mtpt */ ) {
        strncpy(tarheader.name, get(dir, vn->vnode), 100);
        tarheader.typeflag = SYMTYPE;
        /* Read the link in, delete it, and then create it */
        readdata(buf, vn->dataSize);
        strncpy(tarheader.linkname, buf, 100);
    }

    if (verbose)
    {
        if (tarheader.name)
        {
            fprintf(stderr, "%s/%s\n", tarheader.prefix, tarheader.name);
        }
        else
        {
            fprintf(stderr, "%s/\n", tarheader.prefix);
        }
    }

#ifdef AFS_LARGEFILE_ENV
    if (vn->dataSize > 0xFFFFFFFF)
    {
        *((afs_sfsize_t*)&tarheader.size) = vn->dataSize | 0x8000000000000000LL;
    }
    else
#endif
    {
        snprintf(tarheader.size, 12, "%011o", (unsigned int)vn->dataSize);
    }
    snprintf(tarheader.mode, 8, "%07o", vn->modebits);
    snprintf(tarheader.uid, 8, "%07o",  vn->owner);
    snprintf(tarheader.gid, 8, "%07o", vn->group);
    snprintf(tarheader.mtime, 12, "%011o", vn->unixModTime);
    strncpy(tarheader.magic, TMAGIC, TMAGLEN);
    strncpy(tarheader.version, TVERSION, TVERSLEN);

    for (i = 0; i < sizeof(struct Tar); i++) {
        chksum += *((unsigned char*)(&tarheader)+i);
    }

    snprintf(tarheader.chksum, 8, "%07o", chksum);
    write(1, &tarheader, sizeof(struct Tar));
    bytecount += sizeof(struct Tar);
}

    afs_int32
ReadVNode(count)
    afs_int32 count;
{
    struct vNode vn;
    int code, i, done;
    char tag;
    char dirname[MAXNAMELEN];
    char parentdir[MAXNAMELEN];
    char filename[MAXNAMELEN], fname[MAXNAMELEN];
    afs_int32 vnode;

    memset(&vn, 0, sizeof(vn));

    vn.vnode = ntohl(readvalue(4));
    vn.uniquifier = ntohl(readvalue(4));

    done = 0;
    while (!done) {
        tag = readchar();
        switch (tag) {
            case 't':
                vn.type = ntohl(readvalue(1));
                break;

            case 'l':
                vn.linkCount = ntohl(readvalue(2));
                break;

            case 'v':
                vn.dataVersion = ntohl(readvalue(4));
                break;

            case 'm':
                vn.unixModTime = ntohl(readvalue(4));
                break;

            case 's':
                vn.servModTime = ntohl(readvalue(4));
                break;

            case 'a':
                vn.author = ntohl(readvalue(4));
                break;

            case 'o':
                vn.owner = ntohl(readvalue(4));
                break;

            case 'g':
                vn.group = ntohl(readvalue(4));
                break;

            case 'b':
                vn.modebits = ntohl(readvalue(2));
                break;

            case 'p':
                vn.parent = ntohl(readvalue(4));
                break;

            case 'A':
                readdata(vn.acl, (afs_sfsize_t)192);      /* Skip ACL data */
                break;

#ifdef AFS_LARGEFILE_ENV
            case 'h':
                {
                    afs_uint32 hi, lo;
                    hi = ntohl(readvalue(4));
                    lo = ntohl(readvalue(4));
                    FillInt64(vn.dataSize, hi, lo);
                }
                goto common_vnode;
#endif

            case 'f':
                vn.dataSize = ntohl(readvalue(4));

#ifdef AFS_LARGEFILE_ENV
common_vnode:
#endif
                /* parentdir is the name of this dir's vnode-file-link
                 * or this file's parent vnode-file-link.
                 * "./AFSDir-<#>". It's a symbolic link to its real dir.
                 * The parent dir and symbolic link to it must exist.
                 */
                vnode = ((vn.type == 2) ? vn.vnode : vn.parent);
                if (vnode == 1)
                    strncpy(parentdir, ".", sizeof parentdir);
                else {
                    strncpy(parentdir, get(".", vnode), sizeof parentdir);
                }

                WriteVNodeTarHeader(parentdir, &vn);

                if (vn.type == 2) {
                    /*ITSADIR*/
                    /* We read the directory entries. If the entry is a
                     * directory, the subdir is created and the root dir
                     * will contain a link to it. If its a file, we only
                     * create a symlink in the dir to the file name.
                     */
                    char *buffer;
                    unsigned short j;
                    afs_int32 this_vn;
                    char *this_name;

                    struct DirEntry {
                        char flag;
                        char length;
                        unsigned short next;
                        struct MKFid {
                            afs_int32 vnode;
                            afs_int32 vunique;
                        } fid;
                        char name[20];
                    };

                    struct Pageheader {
                        unsigned short pgcount;
                        unsigned short tag;
                        char freecount;
                        char freebitmap[8];
                        char padding[19];
                    };

                    struct DirHeader {
                        struct Pageheader header;
                        char alloMap[128];
                        unsigned short hashTable[128];
                    };

                    struct Page0 {
                        struct DirHeader header;
                        struct DirEntry entry[1];
                    } *page0;


                    buffer = NULL;
                    buffer = (char *)malloc(vn.dataSize);

                    readdata(buffer, vn.dataSize);
                    page0 = (struct Page0 *)buffer;

                    /* Step through each bucket in the hash table, i,
                     * and follow each element in the hash chain, j.
                     * This gives us each entry of the dir.
                     */
                    for (i = 0; i < 128; i++) {
                        for (j = ntohs(page0->header.hashTable[i]); j;
                                j = ntohs(page0->entry[j].next)) {
                            j -= 13;
                            this_vn = ntohl(page0->entry[j].fid.vnode);
                            this_name = page0->entry[j].name;

                            if ((strcmp(this_name, ".") == 0)
                                    || (strcmp(this_name, "..") == 0))
                                continue;   /* Skip these */

                            /* For a directory entry, create it. Then create the
                             * link (from the rootdir) to this directory.
                             */
                            if (this_vn & 1) {
                                /*ADIRENTRY*/
                                /* dirname is the directory to create.
                                 * vflink is what will link to it. 
                                 */
                                snprintf(dirname, sizeof dirname, "%s/%s",
                                        parentdir, this_name);

                                add(".", this_vn, dirname);
                            }
                            /*ADIRENTRY*/
                            /* For a file entry, we remember the name of the file
                             * by creating a link within the directory. Restoring
                             * the file will later remove the link.
                             */
                            else {
                                /*AFILEENTRY*/

                                add(parentdir, this_vn, this_name);
                            }
                            /*AFILEENTRY*/}
                    }
                    free(buffer);
                }
                /*ITSADIR*/
                else if (vn.type == 1) {
                    /*ITSAFILE*/
                    /* A file vnode. So create it into the desired directory. A
                     * link should exist in the directory naming the file.
                     */
                    afs_sfsize_t size, s;

                    snprintf(filename, sizeof filename, "%s/%s", parentdir,
                            get(parentdir, vn.vnode));

                    size = vn.dataSize;
                    while (size > 0) {
                        s = (afs_int32) ((size > BUFSIZE) ? BUFSIZE : size);
                        code = fread(buf, 1, s, dumpfile);
                        if (code > 0) {
                            (void)write(1, buf, code);
                            bytecount += code;
                            size -= code;
                        }
                        if (code != s) {
                            if (code < 0)
                                fprintf(stderr, "Code = %d; Errno = %d\n", code,
                                        errno);
                            else {
                                char tmp[100];
                                (void)snprintf(tmp, sizeof tmp,
                                        "Read %llu bytes out of %llu",
                                        (afs_uintmax_t) (vn.dataSize -
                                            size),
                                        (afs_uintmax_t) vn.dataSize);
                                fprintf(stderr, "%s\n", tmp);
                            }
                            break;
                        }
                    }
                    if (size != 0) {
                        fprintf(stderr, "   File %s (%s) is incomplete\n",
                                filename, fname);
                    }
                    size = 512 - (vn.dataSize % 512);
                    while (size--) {
                        char a = 0;
                        write(1, &a, 1);
                        bytecount++;
                    }
                }
                /*ITSAFILE*/
                else if (vn.type == 3) {
                    /*ITSASYMLINK*/
                }
                /*ITSASYMLINK*/
                else {
                    fprintf(stderr, "Unknown Vnode block\n");
                }
                break;

            default:
                done = 1;
                break;
        }
    }

    return ((afs_int32) tag);
}

int
main(argc, argv)
    int argc;
    char **argv;
{
    int code = 0, c;
    afs_int32 type, count, vcount;
    struct DumpHeader dh;       /* Defined in dump.h */

    int arg;
    dumpfile = (FILE *) stdin;      /* use stdin */
    while ((arg = getopt(argc, argv, "xcvf:")) != -1)
    {
        switch (arg)
        {
            case 'h':
                break;
            case 'x':
                fprintf(stderr, "Extract not implemented yet\n");
                return 1;
                break;
            case 'c':
                break;
            case 'v':
                verbose++;
                break;
            case 'f':
                dumpfile = fopen(optarg, "r");
                if (!dumpfile) {
                    fprintf(stderr, "Cannot open '%s'. Code = %d\n",
                            optarg, errno);
                }
                break;
            default:
                return 1;
        }
    }

    /* Read the dump header. From it we get the volume name */
    type = ntohl(readvalue(1));
    if (type != 1) {
        fprintf(stderr, "Expected DumpHeader\n");
        code = -1;
        goto cleanup;
    }
    type = ReadDumpHeader(&dh);

    fprintf(stderr, "Converting volume dump of '%s' to tar format.\n",
            dh.volumeName);

    for (count = 1; type == 2; count++) {
        type = ReadVolumeHeader(count);
        for (vcount = 1; type == 3; vcount++)
            type = ReadVNode(vcount);
    }

    if (type != 4) {
        fprintf(stderr, "Expected End-of-Dump\n");
        code = -1;
        goto cleanup;
    }

cleanup:
    c = 1024;
    while (c--) {
        char a = 0;
        write(1, &a, 1);
        bytecount++;
    }

    fprintf(stderr, "Total bytes written: %d\n", bytecount);

    return (code);
}
