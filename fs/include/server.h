#ifndef FS_SERVER_H
#define FS_SERVER_H

#include "tcp_utils.h"

void on_connection(int id);
int on_recv(int id, tcp_buffer *wb, char *msg, int len);
void cleanup(int id);

#endif // FS_SERVER_H
