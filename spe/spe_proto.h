#ifndef __SPE_PROTO_H
#define __SPE_PROTO_H

#include "spe_conn.h"
#include "spe_string.h"

struct spe_proto_s {
  void* on_connect(spe_conn_t* conn);
  void* on_send(spe_conn_t* conn, spe_string_t* msg);
  void* on_recv(spe_conn_t* conn, spe_string_t* msg);
  void* on_subscribe(spe_conn_t* conn, spe_string_t* msg);
};
typedef struct spe_proto_s spe_proto_t;

#endif
