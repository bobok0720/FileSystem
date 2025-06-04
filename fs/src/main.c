
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "block.h"
#include "common.h"
#include "fs.h"
#include "log.h"

// global variables
int ncyl, nsec;

#define ReplyYes()       \
    do {                 \
        printf("Yes\n"); \
        Log("Success");  \
    } while (0)
#define ReplyNo(x)      \
    do {                \
        printf("No\n"); \
        Warn(x);        \
    } while (0)

// return a negative value to exit
int handle_f(char *args) {
    if (cmd_f(ncyl, nsec) == E_SUCCESS) {
        ReplyYes();
    } else {
        ReplyNo("Failed to format");
    }
    return 0;
}

int handle_mk(char *args) {
    char *name = strtok(args, " ");
    char *mode_str = strtok(NULL, " ");
    short mode = 0;
    if (mode_str) mode = (short)strtol(mode_str, NULL, 0);
    if (name && cmd_mk(name, mode) == E_SUCCESS) {
        ReplyYes();
    } else {
        ReplyNo("Failed to create file");
    }
    return 0;
}

int handle_mkdir(char *args) {
    char *name = strtok(args, " ");
    char *mode_str = strtok(NULL, " ");
    short mode = 0;
    if (mode_str) mode = (short)strtol(mode_str, NULL, 0);
    if (name && cmd_mkdir(name, mode) == E_SUCCESS) {
        ReplyYes();
    } else {
        ReplyNo("Failed to create directory");
    }
    return 0;
}

int handle_rm(char *args) {
    char *name = strtok(args, " ");
    if (name && cmd_rm(name) == E_SUCCESS) {
        ReplyYes();
    } else {
        ReplyNo("Failed to remove file");
    }
    return 0;
}

int handle_cd(char *args) {
    char *name = strtok(args, " ");
    if (name && cmd_cd(name) == E_SUCCESS) {
        ReplyYes();
    } else {
        ReplyNo("Failed to change directory");
    }
    return 0;
}

int handle_rmdir(char *args) {
    char *name = strtok(args, " ");
    if (name && cmd_rmdir(name) == E_SUCCESS) {
        ReplyYes();
    } else {
        ReplyNo("Failed to remove directory");
    }
    return 0;
}

int handle_ls(char *args) {
    entry *entries = NULL;
    int n = 0;
    if (cmd_ls(&entries, &n) != E_SUCCESS) {
        ReplyNo("Failed to list files");
        return 0;
    }
    ReplyYes();
    free(entries);
    return 0;
}

int handle_cat(char *args) {
    char *name = strtok(args, " ");

    uchar *buf = NULL;
    uint len;
    if (name && cmd_cat(name, &buf, &len) == E_SUCCESS) {
        ReplyYes();
        printf("%s\n", buf);
        free(buf);
    } else {
        ReplyNo("Failed to read file");
    }
    return 0;
}

int handle_w(char *args) {
    char *name = strtok(args, " ");
    char *len_str = strtok(NULL, " ");
    uint len = 0;
    if (len_str) len = strtoul(len_str, NULL, 0);
    char *data = strtok(NULL, "");
    if (!data) data = "";
    if (name && cmd_w(name, len, data) == E_SUCCESS) {
        ReplyYes();
    } else {
        ReplyNo("Failed to write file");
    }
    return 0;
}

int handle_i(char *args) {
    char *name = strtok(args, " ");
    char *pos_str = strtok(NULL, " ");
    char *len_str = strtok(NULL, " ");
    uint pos = 0, len = 0;
    if (pos_str) pos = strtoul(pos_str, NULL, 0);
    if (len_str) len = strtoul(len_str, NULL, 0);
    char *data = strtok(NULL, "");
    if (!data) data = "";
    if (name && cmd_i(name, pos, len, data) == E_SUCCESS) {
        ReplyYes();
    } else {
        ReplyNo("Failed to write file");
    }
    return 0;
}

int handle_d(char *args) {
    char *name = strtok(args, " ");
    char *pos_str = strtok(NULL, " ");
    char *len_str = strtok(NULL, " ");
    uint pos = 0, len = 0;
    if (pos_str) pos = strtoul(pos_str, NULL, 0);
    if (len_str) len = strtoul(len_str, NULL, 0);
    if (name && cmd_d(name, pos, len) == E_SUCCESS) {
        ReplyYes();
    } else {
        ReplyNo("Failed to delete file");
    }
    return 0;
}

int handle_e(char *args) {
    printf("Bye!\n");
    Log("Exit");
    return -1;
}

int handle_login(char *args) {
    char *uid_str = strtok(args, " ");
    int uid = 0;
    if (uid_str) uid = atoi(uid_str);
    if (cmd_login(uid) == E_SUCCESS) {
        ReplyYes();
    } else {
        ReplyNo("Failed to login");
    }
    return 0;
}

static struct {
    const char *name;
    int (*handler)(char *);
} cmd_table[] = {{"f", handle_f},        {"mk", handle_mk},       {"mkdir", handle_mkdir}, {"rm", handle_rm},
                 {"cd", handle_cd},      {"rmdir", handle_rmdir}, {"ls", handle_ls},       {"cat", handle_cat},
                 {"w", handle_w},        {"i", handle_i},         {"d", handle_d},         {"e", handle_e},
                 {"login", handle_login}};

#define NCMD (sizeof(cmd_table) / sizeof(cmd_table[0]))

FILE *log_file;

int main(int argc, char *argv[]) {
    log_init("fs.log");

    assert(BSIZE % sizeof(dinode) == 0);

    // get disk info and store in global variables
    get_disk_info(&ncyl, &nsec);

    // read the superblock
    sbinit();

    static char buf[4096];
    while (1) {
        fgets(buf, sizeof(buf), stdin);
        if (feof(stdin)) break;
        buf[strlen(buf) - 1] = 0;
        Log("Use command: %s", buf);
        char *p = strtok(buf, " ");
        int ret = 1;
        for (int i = 0; i < NCMD; i++)
            if (p && strcmp(p, cmd_table[i].name) == 0) {
                ret = cmd_table[i].handler(p + strlen(p) + 1);
                break;
            }
        if (ret == 1) {
            Log("No such command");
            printf("No\n");
        }
        if (ret < 0) break;
    }

    log_close();
}
