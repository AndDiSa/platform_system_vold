/*
 * Copyright (C) 2012 Team Eos & Chris Trotman
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>

#include <linux/kdev_t.h>
#include <linux/fs.h>

#define LOG_TAG "Vold"

#include <cutils/log.h>
#include <cutils/properties.h>

#include "Ntfs.h"

static char NTFS3G_PATH[] = "/system/bin/ntfs-3g";
static char NTFS3G_PROBE_PATH[] = "/system/bin/ntfs-3g.probe";
static char NTFSFIX_PATH[] = "/system/bin/ntfsfix";
extern "C" int logwrap(int argc, const char **argv, int background);
extern "C" int mount(const char *, const char *, const char *, unsigned long, const void *);

int Ntfs::check(const char *fsPath) {
    bool rw = true;
    if (access(NTFS3G_PROBE_PATH, X_OK)) {
        SLOGW("Skipping fs checks\n");
        return 0;
    }

    int rc = 0;
    const char *args[5];
    args[0] = NTFS3G_PROBE_PATH;
    args[1] = "-w";
    args[2] = fsPath;
    args[3] = NULL;

    rc = logwrap(3, args, 1);

    switch (rc) {
        case 0:
            SLOGI("NTFS filesystem found");
            return 0;
        case 13: // Inconsistent NTFS
        case 14: // NTFS Partition is hibernated
        case 15: // NTFS Partition was uncleanly unmounted.
            SLOGI("Inconsistent NTFS filesystem found. Trying to fix.");
            rc = 0;
            args[0] = NTFSFIX_PATH;
            args[1] = fsPath;
            args[2] = NULL;
            rc = logwrap(2, args, 1);
            if (rc == 0) return 0;
            else return 1;
        default:
            return 2;
    }

    return 0;
}

int Ntfs::doMount(const char *fsPath, const char *mountPoint,
                 bool ro, bool remount, bool executable,
                 int ownerUid, int ownerGid, int permMask, bool createLost) {
    int rc;
    unsigned long flags;
    char mountOptions[255];
    const char *args[5];

    sprintf(mountOptions,
            "-o nodev,nosuid,sync,noexec,utf8,uid=%d,gid=%d,fmask=%o,dmask=%o%s%s",
            ownerUid, ownerGid, permMask, permMask, (ro ? ",ro" : ""),
            (remount ? ",remount" : ""));

    args[0] = NTFS3G_PATH;
    args[1] = mountOptions;
    args[2] = fsPath;
    args[3] = mountPoint;
    args[4] = NULL;

    rc = logwrap(4, args, 1);

    if (rc && errno == EROFS) {
        SLOGE("%s appears to be a read only filesystem - retrying mount RO", fsPath);
        sprintf(mountOptions,
                "-o nodev,nosuid,sync,noexec,utf8,uid=%d,gid=%d,fmask=%o,dmask=%o,ro%s",
                ownerUid, ownerGid, permMask, permMask, (remount ? ",remount" : ""));

        args[0] = NTFS3G_PATH;
        args[1] = mountOptions;
        args[2] = fsPath;
        args[3] = mountPoint;
        args[4] = NULL;

        rc = logwrap(4, args, 1);
    }

    return rc;
}

int Ntfs::format(const char *fsPath, unsigned int numSectors) {
    return -1; // Formatting Ntfs is not supported.
}

