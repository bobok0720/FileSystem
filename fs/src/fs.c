#include "fs.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "block.h"
#include "log.h"

typedef struct vfile {
    char name[MAXNAME];
    short mode;
    short type;
    struct vfile *parent;
    struct vfile **children;
    int child_count;
    int child_cap;
    uchar *data;
    uint size;
} vfile;

static vfile *root = NULL;
static vfile *cwd = NULL;
static int logged_in = 0;
static int formatted = 0;

static vfile *create_vfile(const char *name, short mode, short type, vfile *parent) {
    vfile *vf = calloc(1, sizeof(vfile));
    if (!vf) return NULL;
    strncpy(vf->name, name, MAXNAME - 1);
    vf->mode = mode;
    vf->type = type;
    vf->parent = parent;
    return vf;
}

static void destroy_vfile(vfile *vf) {
    if (!vf) return;
    for (int i = 0; i < vf->child_count; i++) {
        destroy_vfile(vf->children[i]);
    }
    free(vf->children);
    free(vf->data);
    free(vf);
}

static vfile *find_child(vfile *dir, const char *name) {
    for (int i = 0; i < dir->child_count; i++) {
        if (strcmp(dir->children[i]->name, name) == 0) return dir->children[i];
    }
    return NULL;
}

static int add_child(vfile *dir, vfile *child) {
    if (dir->child_count == dir->child_cap) {
        int new_cap = dir->child_cap ? dir->child_cap * 2 : 4;
        void *tmp = realloc(dir->children, new_cap * sizeof(vfile *));
        if (!tmp) return 0;
        dir->children = tmp;
        dir->child_cap = new_cap;
    }
    dir->children[dir->child_count++] = child;
    child->parent = dir;
    return 1;
}

static void remove_child(vfile *dir, int idx) {
    destroy_vfile(dir->children[idx]);
    for (int i = idx + 1; i < dir->child_count; i++)
        dir->children[i - 1] = dir->children[i];
    dir->child_count--;
}

void sbinit() {
    uchar buf[BSIZE];
    read_block(0, buf);
    memcpy(&sb, buf, sizeof(sb));
}

int cmd_f(int ncyl, int nsec) {
    if (!logged_in) return E_NOT_LOGGED_IN;
    destroy_vfile(root);
    root = create_vfile("/", 0, T_DIR, NULL);
    if (!root) return E_ERROR;
    cwd = root;
    formatted = 1;
    (void)ncyl; // parameters unused in this mock implementation
    (void)nsec;
    return E_SUCCESS;
}

int cmd_mk(char *name, short mode) {
    if (!formatted) return E_NOT_FORMATTED;
    if (!cwd || !name || find_child(cwd, name)) return E_ERROR;
    vfile *vf = create_vfile(name, mode, T_FILE, cwd);
    if (!vf) return E_ERROR;
    if (!add_child(cwd, vf)) {
        destroy_vfile(vf);
        return E_ERROR;
    }
    return E_SUCCESS;
}

int cmd_mkdir(char *name, short mode) {
    if (!formatted) return E_NOT_FORMATTED;
    if (!cwd || !name || find_child(cwd, name)) return E_ERROR;
    vfile *vf = create_vfile(name, mode, T_DIR, cwd);
    if (!vf) return E_ERROR;
    if (!add_child(cwd, vf)) {
        destroy_vfile(vf);
        return E_ERROR;
    }
    return E_SUCCESS;
}

int cmd_rm(char *name) {
    if (!formatted) return E_NOT_FORMATTED;
    for (int i = 0; i < cwd->child_count; i++) {
        vfile *c = cwd->children[i];
        if (c->type == T_FILE && strcmp(c->name, name) == 0) {
            remove_child(cwd, i);
            return E_SUCCESS;
        }
    }
    return E_ERROR;
}

int cmd_cd(char *name) {
    if (!formatted) return E_NOT_FORMATTED;
    if (!name) return E_ERROR;
    vfile *dir = (name[0] == '/') ? root : cwd;
    char tmp[strlen(name) + 1];
    strcpy(tmp, name);
    char *token = strtok(tmp, "/");
    while (token) {
        if (strcmp(token, "..") == 0) {
            if (dir->parent) dir = dir->parent;
        } else if (strcmp(token, ".") != 0 && strlen(token) > 0) {
            vfile *next = find_child(dir, token);
            if (!next || next->type != T_DIR) return E_ERROR;
            dir = next;
        }
        token = strtok(NULL, "/");
    }
    cwd = dir;
    return E_SUCCESS;
}

int cmd_rmdir(char *name) {
    if (!formatted) return E_NOT_FORMATTED;
    for (int i = 0; i < cwd->child_count; i++) {
        vfile *c = cwd->children[i];
        if (c->type == T_DIR && strcmp(c->name, name) == 0) {
            if (c->child_count != 0)
                return E_ERROR;
            remove_child(cwd, i);
            return E_SUCCESS;
        }
    }
    return E_ERROR;
}
int cmd_ls(entry **entries, int *n) {
    if (!formatted) return E_NOT_FORMATTED;
    *n = cwd->child_count;
    *entries = calloc(*n, sizeof(entry));
    if (!*entries && *n) return E_ERROR;
    for (int i = 0; i < *n; i++) {
        (*entries)[i].type = cwd->children[i]->type;
        strncpy((*entries)[i].name, cwd->children[i]->name, MAXNAME - 1);
        (*entries)[i].name[MAXNAME - 1] = '\0';
    }
    return E_SUCCESS;
}

int cmd_cat(char *name, uchar **buf, uint *len) {
    if (!formatted) return E_NOT_FORMATTED;
    vfile *c = find_child(cwd, name);
    if (!c || c->type != T_FILE) return E_ERROR;
    *len = c->size;
    *buf = malloc(*len + 1);
    if (!*buf && *len) return E_ERROR;
    memcpy(*buf, c->data, *len);
    (*buf)[*len] = '\0';
    return E_SUCCESS;
}

int cmd_w(char *name, uint len, const char *data) {
    if (!formatted) return E_NOT_FORMATTED;
    vfile *c = find_child(cwd, name);
    if (!c || c->type != T_FILE) return E_ERROR;
    uchar *tmp = realloc(c->data, len);
    if (!tmp && len) return E_ERROR;
    c->data = tmp;
    memcpy(c->data, data, len);
    c->size = len;
    return E_SUCCESS;
}

int cmd_i(char *name, uint pos, uint len, const char *data) {
    if (!formatted) return E_NOT_FORMATTED;
    vfile *c = find_child(cwd, name);
    if (!c || c->type != T_FILE) return E_ERROR;
    if (pos > c->size) pos = c->size;
    uchar *tmp = realloc(c->data, c->size + len);
    if (!tmp && len) return E_ERROR;
    c->data = tmp;
    memmove(c->data + pos + len, c->data + pos, c->size - pos);
    memcpy(c->data + pos, data, len);
    c->size += len;
    return E_SUCCESS;
}

int cmd_d(char *name, uint pos, uint len) {
    if (!formatted) return E_NOT_FORMATTED;
    vfile *c = find_child(cwd, name);
    if (!c || c->type != T_FILE) return E_ERROR;
    if (pos > c->size) return E_ERROR;
    if (pos + len > c->size) len = c->size - pos;
    memmove(c->data + pos, c->data + pos + len, c->size - pos - len);
    c->size -= len;
    uchar *tmp = realloc(c->data, c->size);
    if (tmp || c->size == 0) c->data = tmp;
    return E_SUCCESS;
}

int cmd_login(int auid) {
    (void)auid;
    logged_in = 1;
    return E_SUCCESS;
}
