/*
 * Copyright 2000, International Business Machines Corporation and others.
 * All Rights Reserved.
 *
 * This software has been released under the terms of the IBM Public
 * License.  For details, see the LICENSE file in the top-level source
 * directory or online at http://www.openafs.org/dl/license10.html
 */

/*
 * This code is based on the restorevol utility included with the OpenAFS source
 * distribution.
 *
 * Support for the tar format was added by Matthew Loar <matthew@loar.name>.
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
#include "common.h"
#include "storage.h"

static FILE *g_dumpfile, *g_tarfile;

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

    code = fread(&ptr[s], 1, size, g_dumpfile);
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
    code = fread(&value, 1, 1, g_dumpfile);
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
            code = fread(buf, 1, s, g_dumpfile);
            if (code != s)
                fprintf(stderr, "Code = %d; Errno = %d\n", code, errno);
            size -= s;
        }
    } else {
        code = fread(buffer, 1, size, g_dumpfile);
        if (code != size) {
            if (code < 0)
                fprintf(stderr, "Code = %d; Errno = %d\n", code, errno);
            else
                fprintf(stderr, "Read %d bytes out of %lld\n", code,
                        (afs_uintmax_t)size);
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

    memset(dh, 0, sizeof(dh));

    magic = ntohl(readvalue(4));
    if (magic != DUMPBEGINMAGIC)
    {
        fprintf(stderr, "input does not appear to be a vos dump\n");
        exit(1);
    }

    dh->version = ntohl(readvalue(4));
    if (dh->version != DUMPVERSION)
    {
        fprintf(stderr, "vos dump has unsupported version: %d\n", dh->version);
        exit(1);
    }

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

    memset(&vh, 0, sizeof(vh));

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
    struct acl_accessList {
        int size; /*size of this access list in bytes, including size itself */
        int version; /*to deal with upward compatibility ; <= ACL_ACLVERSION */
        int total;
        int positive;           /* number of positive entries */
        int negative;           /* number of minus entries */
        struct acl_accessEntry {
            int id;             /*internally-used ID of user or group */
            int rights;         /*mask */
        } entries[21];
        int unused;
    } acl;
    afs_sfsize_t dataSize;
};

#define MAXNAMELEN 256

void
WriteVNodeTarHeader(const char *dir, struct vNode *vn)
{
    unsigned int i;
    unsigned int chksum = 0;
    const char *filename = (vn->type == vDirectory ? NULL : get(vn->vnode));
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
    if (vn->type == 1 /* file */)
    {
        strncpy(tarheader.name, get(vn->vnode), 100);
        tarheader.typeflag = REGTYPE;
    }
    else if (vn->type == 2 /* directory */)
    {
        tarheader.typeflag = DIRTYPE;
    }
    else if (vn->type == 3 /* symlink or mtpt */ )
    {
        strncpy(tarheader.name, filename, 100);
        tarheader.typeflag = SYMTYPE;
        readdata(buf, vn->dataSize);
        strncpy(tarheader.linkname, buf, 100);
    }

    if (verbose)
    {
        if (filename)
        {
            fprintf(stderr, "%s/%s\n", dir, filename);
        }
        else
        {
            fprintf(stderr, "%s/\n", dir);
        }
    }

    /*
     * In the vos dump format, the target of symlinks is the data.  In the tar
     * format, it is included in the header.  So be sure to write a zero size
     * for a symlink.
     */
    if (vn->type != 3 /* symlink or mtpt */)
    {
#ifdef AFS_LARGEFILE_ENV
        /* 11 octal digits have a maximum size of 8 GB - 1. */
        if (vn->dataSize > ((uintmax_t) 1 << (11 * 3)) - 1)
        {
            /* GNU tar (and perhaps others) support using base-256 for size */
            uintmax_t v = vn->dataSize;
            size_t i;

            tarheader.size[0] = -128; /* 0x80 signed */
            for (i = 1; i < sizeof(tarheader.size); i++)
            {
                tarheader.size[i] = v & 0xFF;
                v = v >> 8;
            }
        }
        else
#endif
        {
            snprintf(tarheader.size, 12, "%011llo", vn->dataSize);
        }
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
    fwrite(&tarheader, 1, sizeof(struct Tar), g_tarfile);
    bytecount += sizeof(struct Tar);

    if (acls && vn->type == 2 /* directory */) {
        memset(tarheader.chksum, ' ', 8);
        strncpy(tarheader.name, ".afs_acl_restore.sh", 100);
        strncpy(tarheader.mode, "0700", 8);
        tarheader.typeflag = REGTYPE;

        if (verbose)
        {
            fprintf(stderr, "%s/.afs_acl_restore.sh\n", dir);
        }

        {
            int q;

            buf[0] = 0;
            strcat(buf, "#!/bin/sh\n\n");
            if (vn->acl.positive)
            {
                strcat(buf, "fs sa `dirname $0` ");
                for (q = 0; q < vn->acl.positive && q < 21; q++)
                {
                    char id[15];
                    sprintf(id, "%d ", vn->acl.entries[q].id);
                    strcat(buf, id);
                    if (vn->acl.entries[q].rights & 1)
                    {
                        strcat(buf, "r");
                    }
                    if (vn->acl.entries[q].rights & 2)
                    {
                        strcat(buf, "w");
                    }
                    if (vn->acl.entries[q].rights & 4)
                    {
                        strcat(buf, "i");
                    }
                    if (vn->acl.entries[q].rights & 8)
                    {
                        strcat(buf, "l");
                    }
                    if (vn->acl.entries[q].rights & 16)
                    {
                        strcat(buf, "d");
                    }
                    if (vn->acl.entries[q].rights & 32)
                    {
                        strcat(buf, "k");
                    }
                    if (vn->acl.entries[q].rights & 64)
                    {
                        strcat(buf, "a");
                    }
                    strcat(buf, " ");
                }
                strcat(buf, "-clear\n");
            }
            if (vn->acl.negative)
            {
                strcat(buf, "fs sa `dirname $0` ");
                for (; q < vn->acl.total && q < 21; q++)
                {
                    char id[15];
                    sprintf(id, "%d ", vn->acl.entries[q].id);
                    strcat(buf, id);
                    if (vn->acl.entries[q].rights & 1)
                    {
                        strcat(buf, "r");
                    }
                    if (vn->acl.entries[q].rights & 2)
                    {
                        strcat(buf, "w");
                    }
                    if (vn->acl.entries[q].rights & 4)
                    {
                        strcat(buf, "i");
                    }
                    if (vn->acl.entries[q].rights & 8)
                    {
                        strcat(buf, "l");
                    }
                    if (vn->acl.entries[q].rights & 16)
                    {
                        strcat(buf, "d");
                    }
                    if (vn->acl.entries[q].rights & 32)
                    {
                        strcat(buf, "k");
                    }
                    if (vn->acl.entries[q].rights & 64)
                    {
                        strcat(buf, "a");
                    }
                    strcat(buf, " ");
                }
                strcat(buf, " -negative\n");
            }

            snprintf(tarheader.size, 12, "%011o", strlen(buf));

            chksum = 0;
            for (i = 0; i < sizeof(struct Tar); i++) {
                chksum += *((unsigned char*)(&tarheader)+i);
            }

            {
                size_t size = strlen(buf);
                snprintf(tarheader.chksum, 8, "%07o", chksum);
                fwrite(&tarheader, 1, sizeof(struct Tar), g_tarfile);
                bytecount += sizeof(struct Tar);

                fwrite(buf, 1, size, g_tarfile);
                bytecount += size;

                size = 512 - (size % 512);
                if (size != 512)
                {
                    memset(buf, 0, size);
                    fwrite(buf, 1, size, g_tarfile);
                    bytecount += size;
                }
            }
        }
    }
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
    char filename[MAXNAMELEN];
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
                vn.acl.size = ntohl(readvalue(4));
                vn.acl.version = ntohl(readvalue(4));
                vn.acl.total = ntohl(readvalue(4));
                vn.acl.positive = ntohl(readvalue(4));
                vn.acl.negative = ntohl(readvalue(4));
                for (i = 0; i < 21; i++)
                {
                    vn.acl.entries[i].id = ntohl(readvalue(4));
                    vn.acl.entries[i].rights = ntohl(readvalue(4));
                }
                vn.acl.unused = ntohl(readvalue(4));
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
                vnode = ((vn.type == 2) ? vn.vnode : vn.parent);
                if (vnode == 1)
                    strncpy(parentdir, ".", sizeof parentdir);
                else {
                    if (!get(vnode))
                    {
                        /* This is an orphan */
                        parentdir[0] = 0;
                    }
                    else
                    {
                        strncpy(parentdir, get(vnode), sizeof parentdir);
                    }
                }

                /* Don't write a header for orphans */
                if (parentdir[0])
                {
                    WriteVNodeTarHeader(parentdir, &vn);
                }

                if (vn.type == 2) {
                    /*ITSADIR*/
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

                            if (this_vn & 1) {
                                /*ADIRENTRY*/
                                snprintf(dirname, sizeof dirname, "%s/%s",
                                        parentdir, this_name);

                                /* Store the directory name associated with the
                                 * vnode number.
                                 */
                                add(this_vn, dirname);
                            }
                            /*ADIRENTRY*/
                            else {
                                /*AFILEENTRY*/

                                add(this_vn, this_name);
                            }
                            /*AFILEENTRY*/}
                    }
                    free(buffer);
                }
                /*ITSADIR*/
                else if (vn.type == 1) {
                    /*ITSAFILE*/

                    afs_sfsize_t size, s;

                    size = vn.dataSize;
                    while (size > 0) {
                        s = (afs_int32) ((size > BUFSIZE) ? BUFSIZE : size);
                        code = fread(buf, 1, s, g_dumpfile);
                        if (code > 0) {
                            /* Don't write the contents of orphaned files */
                            if (parentdir[0])
                            {
                                fwrite(buf, 1, code, g_tarfile);
                            }
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
                    if (size != 0)
                    {
                        snprintf(filename, sizeof filename, "%s/%s", parentdir,
                                get(vn.vnode));

                        fprintf(stderr, "   File %s is incomplete\n",
                                filename);
                    }
                    size = 512 - (vn.dataSize % 512);
                    if (size != 512)
                    {
                        memset(buf, 0, size);
                        fwrite(buf, 1, size, g_tarfile);
                        bytecount += size;
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
create(FILE *dumpfile, FILE *tarfile)
{
    afs_int32 type, count, vcount;
    struct DumpHeader dh;       /* Defined in dump.h */

    g_dumpfile = dumpfile;
    g_tarfile = tarfile;

    /* Read the dump header. From it we get the volume name */
    type = ntohl(readvalue(1));
    if (type != D_DUMPHEADER) {
        fprintf(stderr, "Expected DumpHeader\n");
        return -1;
    }
    type = ReadDumpHeader(&dh);

    if (verbose > 1)
    {
        fprintf(stderr, "Converting volume dump of '%s' to tar format.\n",
            dh.volumeName);
    }

    for (count = 1; type == D_VOLUMEHEADER; count++) {
        type = ReadVolumeHeader(count);
        for (vcount = 1; type == D_VNODE; vcount++)
            type = ReadVNode(vcount);
    }

    if (type != D_DUMPEND) {
        fprintf(stderr, "Expected End-of-Dump\n");
        return -1;
    }

    memset(buf, 0, 1024);
    fwrite(buf, 1, 1024, tarfile);
    bytecount += 1024;

    fprintf(stderr, "Total bytes written: %llu\n", bytecount);

    return 0;
}
