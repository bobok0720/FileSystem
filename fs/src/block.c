#include "block.h"

#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "log.h"

superblock sb;

static uchar *disk_data = NULL;
static uint disk_blocks = 0;

static void ensure_disk(uint blocks) {
    if (blocks > disk_blocks) {
        uchar *new_data = realloc(disk_data, blocks * BSIZE);
        if (new_data == NULL) {
            Error("disk realloc failed");
            exit(1);
        }
        memset(new_data + disk_blocks * BSIZE, 0, (blocks - disk_blocks) * BSIZE);
        disk_data = new_data;
        disk_blocks = blocks;
    }
}

void zero_block(uint bno) {
    uchar buf[BSIZE];
    memset(buf, 0, BSIZE);
    write_block(bno, buf);
}

uint allocate_block() {
    for (uint bno = 0; bno < sb.size; bno++) {
        uchar buf[BSIZE];
        read_block(BBLOCK(bno), buf);
        int i = bno % BPB;
        int mask = 1 << (i % 8);
        if ((buf[i / 8] & mask) == 0) {
            buf[i / 8] |= mask;
            write_block(BBLOCK(bno), buf);
            zero_block(bno);
            return bno;
        }
    }
    Warn("Out of blocks");
    return 0;
}

void free_block(uint bno) {
    if (bno >= sb.size) return;
    uchar buf[BSIZE];
    read_block(BBLOCK(bno), buf);
    int i = bno % BPB;
    int mask = 1 << (i % 8);
    buf[i / 8] &= ~mask;
    write_block(BBLOCK(bno), buf);
}

void get_disk_info(int *ncyl, int *nsec) {
    if (ncyl) *ncyl = 0;
    if (nsec) *nsec = 0;
}

void read_block(int blockno, uchar *buf) {
    ensure_disk(blockno + 1);
    memcpy(buf, disk_data + blockno * BSIZE, BSIZE);
}

void write_block(int blockno, uchar *buf) {
    ensure_disk(blockno + 1);
    memcpy(disk_data + blockno * BSIZE, buf, BSIZE);
}
