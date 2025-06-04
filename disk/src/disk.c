#include "disk.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "log.h"

// global variables
int _ncyl, _nsec, ttd;
static unsigned char *disk_data = NULL;
static size_t nblocks = 0;

int init_disk(char *filename, int ncyl, int nsec, int ttd_param) {
    (void)filename;
    _ncyl = ncyl;
    _nsec = nsec;
    ttd = ttd_param;
    nblocks = (size_t)ncyl * nsec;
    disk_data = malloc(nblocks * 512);
    if (disk_data == NULL) {
        Error("malloc failed");
        return 1;
    }
    memset(disk_data, 0, nblocks * 512);
    Log("Disk initialized: %s, %d Cylinders, %d Sectors per cylinder", filename, ncyl, nsec);
    return 0;
}

// all cmd functions return 0 on success
int cmd_i(int *ncyl, int *nsec) {
    // get the disk info
    *ncyl = _ncyl;
    *nsec = _nsec;
    return 0;
}

int cmd_r(int cyl, int sec, char *buf) {
    if (cyl >= _ncyl || sec >= _nsec || cyl < 0 || sec < 0) {
        Log("Invalid cylinder or sector");
        return 1;
    }
    size_t off = ((size_t)cyl * _nsec + sec) * 512;
    memcpy(buf, disk_data + off, 512);
    return 0;
}

int cmd_w(int cyl, int sec, int len, char *data) {
    if (cyl >= _ncyl || sec >= _nsec || cyl < 0 || sec < 0) {
        Log("Invalid cylinder or sector");
        return 1;
    }
    if (len < 0 || len > 512) {
        Log("Invalid length");
        return 1;
    }
    size_t off = ((size_t)cyl * _nsec + sec) * 512;
    memset(disk_data + off, 0, 512);
    memcpy(disk_data + off, data, len);
    return 0;
}

void close_disk() {
    free(disk_data);
    disk_data = NULL;
    nblocks = 0;
}
