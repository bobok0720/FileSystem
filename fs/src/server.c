#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "fs.h"
#include "log.h"
#include "server.h"
#include "tcp_utils.h"

/*
 * Command handlers
 *
 * These handlers mimic the behaviour of the ones in main.c but send the
 * response through the tcp_buffer provided by the network library.
 */

static int handle_f(tcp_buffer *wb, char *args, int len) {
    if (cmd_f(0, 0) == E_SUCCESS) {
        reply_with_yes(wb, NULL, 0);
    } else {
        reply_with_no(wb, NULL, 0);
    }
    return 0;
}

static int handle_mk(tcp_buffer *wb, char *args, int len) {
    char *name = strtok(args, " ");
    char *mode_str = strtok(NULL, " ");
    short mode = 0;
    if (mode_str) mode = (short)strtol(mode_str, NULL, 0);
    if (name && cmd_mk(name, mode) == E_SUCCESS) {
        reply_with_yes(wb, NULL, 0);
    } else {
        reply_with_no(wb, NULL, 0);
    }
    return 0;
}

static int handle_mkdir(tcp_buffer *wb, char *args, int len) {
    char *name = strtok(args, " ");
    char *mode_str = strtok(NULL, " ");
    short mode = 0;
    if (mode_str) mode = (short)strtol(mode_str, NULL, 0);
    if (name && cmd_mkdir(name, mode) == E_SUCCESS) {
        reply_with_yes(wb, NULL, 0);
    } else {
        reply_with_no(wb, NULL, 0);
    }
    return 0;
}

static int handle_rm(tcp_buffer *wb, char *args, int len) {
    char *name = strtok(args, " ");
    if (name && cmd_rm(name) == E_SUCCESS) {
        reply_with_yes(wb, NULL, 0);
    } else {
        reply_with_no(wb, NULL, 0);
    }
    return 0;
}

static int handle_cd(tcp_buffer *wb, char *args, int len) {
    char *name = strtok(args, " ");
    if (name && cmd_cd(name) == E_SUCCESS) {
        reply_with_yes(wb, NULL, 0);
    } else {
        reply_with_no(wb, NULL, 0);
    }
    return 0;
}

static int handle_rmdir(tcp_buffer *wb, char *args, int len) {
    char *name = strtok(args, " ");
    if (name && cmd_rmdir(name) == E_SUCCESS) {
        reply_with_yes(wb, NULL, 0);
    } else {
        reply_with_no(wb, NULL, 0);
    }
    return 0;
}

static int handle_ls(tcp_buffer *wb, char *args, int len) {
    entry *entries = NULL;
    int n = 0;
    if (cmd_ls(&entries, &n) == E_SUCCESS) {
        reply_with_yes(wb, NULL, 0);
    } else {
        reply_with_no(wb, NULL, 0);
    }
    free(entries);
    return 0;
}

static int handle_cat(tcp_buffer *wb, char *args, int len) {
    char *name = strtok(args, " ");

    uchar *buf = NULL;
    uint l;
    if (name && cmd_cat(name, &buf, &l) == E_SUCCESS) {
        reply_with_yes(wb, (char *)buf, l);
        free(buf);
    } else {
        reply_with_no(wb, NULL, 0);
    }
    return 0;
}

static int handle_w(tcp_buffer *wb, char *args, int len) {
    char *name = strtok(args, " ");
    char *len_str = strtok(NULL, " ");
    uint l = 0;
    if (len_str) l = strtoul(len_str, NULL, 0);
    char *data = strtok(NULL, "");
    if (!data) data = "";
    if (name && cmd_w(name, l, data) == E_SUCCESS) {
        reply_with_yes(wb, NULL, 0);
    } else {
        reply_with_no(wb, NULL, 0);
    }
    return 0;
}

static int handle_i(tcp_buffer *wb, char *args, int len) {
    char *name = strtok(args, " ");
    char *pos_str = strtok(NULL, " ");
    char *len_str = strtok(NULL, " ");
    uint pos = 0, l = 0;
    if (pos_str) pos = strtoul(pos_str, NULL, 0);
    if (len_str) l = strtoul(len_str, NULL, 0);
    char *data = strtok(NULL, "");
    if (!data) data = "";
    if (name && cmd_i(name, pos, l, data) == E_SUCCESS) {
        reply_with_yes(wb, NULL, 0);
    } else {
        reply_with_no(wb, NULL, 0);
    }
    return 0;
}

static int handle_d(tcp_buffer *wb, char *args, int len) {
    char *name = strtok(args, " ");
    char *pos_str = strtok(NULL, " ");
    char *len_str = strtok(NULL, " ");
    uint pos = 0, l = 0;
    if (pos_str) pos = strtoul(pos_str, NULL, 0);
    if (len_str) l = strtoul(len_str, NULL, 0);
    if (name && cmd_d(name, pos, l) == E_SUCCESS) {
        reply_with_yes(wb, NULL, 0);
    } else {
        reply_with_no(wb, NULL, 0);
    }
    return 0;
}

static int handle_e(tcp_buffer *wb, char *args, int len) {
    const char *msg = "Bye!";
    reply(wb, msg, strlen(msg) + 1);
    return -1;
}

static int handle_login(tcp_buffer *wb, char *args, int len) {
    char *uid_str = strtok(args, " ");
    int uid = 0;
    if (uid_str) uid = atoi(uid_str);
    if (cmd_login(uid) == E_SUCCESS) {
        reply_with_yes(wb, NULL, 0);
    } else {
        reply_with_no(wb, NULL, 0);
    }
    return 0;
}

/* command table */
static struct {
    const char *name;
    int (*handler)(tcp_buffer *, char *, int);
} cmd_table[] = {
    {"f", handle_f},      {"mk", handle_mk},  {"mkdir", handle_mkdir},
    {"rm", handle_rm},    {"cd", handle_cd},  {"rmdir", handle_rmdir},
    {"ls", handle_ls},    {"cat", handle_cat}, {"w", handle_w},
    {"i", handle_i},      {"d", handle_d},    {"e", handle_e},
    {"login", handle_login},
};

#define NCMD (sizeof(cmd_table) / sizeof(cmd_table[0]))

void on_connection(int id) {
    /* currently unused */
}

int on_recv(int id, tcp_buffer *wb, char *msg, int len) {
    char *p = strtok(msg, " \r\n");
    int ret = 1;
    for (int i = 0; i < NCMD; i++)
        if (p && strcmp(p, cmd_table[i].name) == 0) {
            ret = cmd_table[i].handler(wb, p + strlen(p) + 1,
                                      len - strlen(p) - 1);
            break;
        }
    if (ret == 1) {
        static char unk[] = "Unknown command";
        buffer_append(wb, unk, sizeof(unk));
    }
    if (ret < 0) {
        return -1;
    }
    return 0;
}

void cleanup(int id) {
    /* currently unused */
}

FILE *log_file;

int main(int argc, char *argv[]) {
    log_init("fs.log");

    tcp_server server = server_init(666, 1, on_connection, on_recv, cleanup);
    server_run(server);

    /* never reached */
    log_close();
}

