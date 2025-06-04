#include "inode.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "block.h"
#include "log.h"

// simple in-memory inode table used for testing
#define MAX_INODES 1024
static dinode dinode_table[MAX_INODES];
static uchar *inode_data[MAX_INODES];
static int inode_used[MAX_INODES];

inode *iget(uint inum) {
    if (inum == 0 || inum >= MAX_INODES || !inode_used[inum]) return NULL;

    inode *ip = malloc(sizeof(inode));
    if (!ip) return NULL;

    ip->inum = inum;
    ip->type = dinode_table[inum].type;
    ip->size = dinode_table[inum].size;
    ip->blocks = dinode_table[inum].blocks;
    memcpy(ip->addrs, dinode_table[inum].addrs, sizeof(ip->addrs));
    ip->data = inode_data[inum];
    return ip;
}

void iput(inode *ip) { free(ip); }

inode *ialloc(short type) {
    for (uint i = 1; i < MAX_INODES; i++) {
        if (!inode_used[i]) {
            inode_used[i] = 1;
            memset(&dinode_table[i], 0, sizeof(dinode));
            dinode_table[i].type = type;
            inode_data[i] = NULL;
            return iget(i);
        }
    }
    Error("ialloc: no inodes");
    return NULL;
}

void iupdate(inode *ip) {
    if (!ip || ip->inum == 0 || ip->inum >= MAX_INODES || !inode_used[ip->inum])
        return;
    dinode_table[ip->inum].type = ip->type;
    dinode_table[ip->inum].size = ip->size;
    dinode_table[ip->inum].blocks = ip->blocks;
    memcpy(dinode_table[ip->inum].addrs, ip->addrs, sizeof(ip->addrs));
    inode_data[ip->inum] = ip->data;
}

int readi(inode *ip, uchar *dst, uint off, uint n) {
    if (!ip || off > ip->size) return 0;
    if (off + n > ip->size) n = ip->size - off;
    if (n == 0) return 0;
    if (!ip->data) return 0;
    memcpy(dst, ip->data + off, n);
    return n;
}

int writei(inode *ip, uchar *src, uint off, uint n) {
    if (!ip) return -1;
    uint end = off + n;
    uint cur_blocks = (ip->size + BSIZE - 1) / BSIZE;
    uint new_blocks = (end + BSIZE - 1) / BSIZE;

    if (new_blocks > cur_blocks || ip->data == NULL) {
        uchar *new_data = realloc(ip->data, new_blocks * BSIZE);
        if (!new_data) return -1;
        if (new_blocks > cur_blocks)
            memset(new_data + cur_blocks * BSIZE, 0,
                   (new_blocks - cur_blocks) * BSIZE);
        ip->data = new_data;
    }

    if (off > ip->size)
        memset(ip->data + ip->size, 0, off - ip->size);

    memcpy(ip->data + off, src, n);
    if (end > ip->size) ip->size = end;
    ip->blocks = new_blocks;
    iupdate(ip);
    return n;
}
