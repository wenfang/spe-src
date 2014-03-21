#ifndef __SPE_SOCK_H 
#define __SPE_SOCK_H

#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>

extern int  
spe_sock_accept(int fd);

extern int  
spe_sock_accept_timeout(int fd, int timeout);

extern bool 
spe_sock_set_block(int fd, int block);

extern bool
spe_sock_is_nonblock(int fd);

extern int  
spe_sock_tcp_server(int port);

extern int
spe_sock_tcp_server_p(const char* listen_addr);

static inline int 
spe_sock_tcp_socket() {
  return socket(AF_INET, SOCK_STREAM, 0);
}

static inline int 
spe_sock_udp_socket() {
  return socket(AF_INET, SOCK_DGRAM, 0);
}

static inline int 
spe_sock_close(int fd) {
  return close(fd);
}

#endif
