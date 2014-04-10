#include "spe.h"
#include <stdio.h>
#include <stdlib.h>

static LIST_HEAD(free_stub);

#define MSG_INIT  1
#define MSG_CONT  2
#define MSG_EXIT  3

static unsigned long obj_id = 1;

struct sf_stub_s {
  spe_object_HEAD
  spe_conn_t*       conn;
  unsigned long     peer_id;
  struct list_head  stub_node;
};
typedef struct sf_stub_s sf_stub_t;

static void
on_read(void* arg) {
  sf_stub_t* stub = arg;
  if (stub->conn->closed || stub->conn->error) {
    spe_conn_destroy(stub->conn);
    if (stub->peer_id) spe_object_send_msg(stub->peer_id, MSG_EXIT, NULL);
    free(stub);
    return;
  }
  if (stub->peer_id) {
    spe_string_t* cont = spe_string_create(31);
    spe_string_copy(cont, stub->conn->buffer);
    spe_object_send_msg(stub->peer_id, MSG_CONT, cont);
  }
  spe_conn_readuntil(stub->conn, "\r\n", SPE_HANDLER1(on_read, stub));
}

static void
stub_msg_handler(spe_object_t* obj, spe_msg_t* msg) {
  sf_stub_t* stub = (sf_stub_t*)obj;
  spe_string_t* cont;
  switch (msg->type) {
    case MSG_INIT:
      stub->peer_id = (unsigned long)msg->content;
      break;
    case MSG_CONT:
      cont = msg->content;
      spe_conn_write(stub->conn, cont);
      spe_conn_writes(stub->conn, "\r\n");
      spe_conn_flush(stub->conn, SPE_HANDLER_NULL);
      spe_string_destroy(cont);
      break;
    case MSG_EXIT:
      stub->peer_id = 0;
      list_add_tail(&stub->stub_node, &free_stub);
      break;
  }
}

static void
run(spe_server_t* srv, unsigned cfd) {
  sf_stub_t* stub = calloc(1, sizeof(sf_stub_t));
  if (!stub) {
    spe_sock_close(cfd);
    return;
  }
  spe_object_init((spe_object_t*)stub, obj_id++, stub_msg_handler);
  if (!(stub->conn = spe_conn_create(cfd))) {
    free(stub);
    spe_sock_close(cfd);
    return;
  }

  INIT_LIST_HEAD(&stub->stub_node);
  if (list_empty(&free_stub)) {
    list_add_tail(&stub->stub_node, &free_stub);
  } else {
    sf_stub_t* peer = list_first_entry(&free_stub, sf_stub_t, stub_node);
    list_del_init(&peer->stub_node);
    stub->peer_id = peer->id;
    spe_object_send_msg(stub->peer_id, MSG_INIT, (unsigned long*)stub->id);
  }
  spe_conn_readuntil(stub->conn, "\r\n", SPE_HANDLER1(on_read, stub));
}

static spe_server_conf_t srv_conf = {
  NULL,
  NULL,
  run,
};

static spe_server_t* srv;

static bool
stub_init(void) {
  int port = spe_opt_int("stub", "port", 7879);
  int sfd = spe_sock_tcp_server("127.0.0.1", port);
  if (sfd < 0) {
    return false;
  }
  srv= spe_server_create(sfd, &srv_conf); 
  if (!srv) return false;
  return true;
}

static bool
stub_start(void) {
  spe_worker_run();
  spe_server_enable(srv);
  return true;
}

static spe_module_t stub_module = {
  stub_init,
  NULL,
  stub_start,
  NULL,
};

__attribute__((constructor))
static void
__stub_init(void) {
  spe_register_module(&stub_module); 
}
