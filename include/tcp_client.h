#ifndef _TCP_CLIENT_H_
#define _TCP_CLIENT_H_

#include "tcp_buffer.h"

typedef struct tcp_client_ *tcp_client;

tcp_client client_init(const char *hostname, int port);
void client_send(tcp_client client, const char *msg, int len);
int client_recv(tcp_client client, char *buf, int max_len);
void client_destroy(tcp_client client);

#endif
