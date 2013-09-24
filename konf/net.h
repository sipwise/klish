#ifndef _konf_net_h
#define _konf_net_h

#include <konf/buf.h>

typedef struct konf_client_s konf_client_t;

#define KONFD_SOCKET_PATH "/tmp/konfd.socket"

konf_client_t *konf_client_new(const char *path);
void konf_client_free(konf_client_t *instance);
int konf_client_connect(konf_client_t *instance);
void konf_client_disconnect(konf_client_t *instance);
int konf_client_reconnect(konf_client_t *instance);
int konf_client_send(konf_client_t *instance, char *command);
int konf_client__get_sock(konf_client_t *instance);
konf_buf_t * konf_client_recv_data(konf_client_t * instance, konf_buf_t *buf);
int konf_client_recv_answer(konf_client_t * instance, konf_buf_t **data);

#endif
