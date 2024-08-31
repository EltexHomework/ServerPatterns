#ifndef SERVER_H
#define SERVER_H

#include "../../common/headers/common.h"
#include "../../common/headers/endpoint.h"
#include "client.h"
#include <poll.h>
#include <sys/epoll.h>
#include <sys/select.h>

#define run_monitor(server) do { if (STANDARD == 0) \
  {run_select(server);} \
  else if (STANDARD == 1) \
  {run_poll(server);} \
  else {run_epoll(server);}} while(0)

/**
 * Used to create server on TCP and UDP protocol.
 * Using AF_INET address family.
 */
struct server {
  /* Address of the server */
  struct sockaddr_in serv;
  
  /* Endpoint of server */
  struct endpoint* endpoint;
    
  /* Fd for tcp socket */
  int tcp_fd;
  
  /* Fd for udp socket */
  int udp_fd;
};

struct server* create_server(const char* ip, const int port);

void run_server(struct server* server);

void run_select(struct server* server);

void run_poll(struct server* server);

void run_epoll(struct server* server);

void communicate_tcp(struct server* server);

void communicate_udp(struct server* server);

void send_tcp(struct client* client, char buffer[BUFFER_SIZE]);

char* recv_tcp(struct client* client);

void send_udp(struct server* server, struct sockaddr_in* client, char buffer[BUFFER_SIZE]);
  
char* recv_udp(struct server* server, struct sockaddr_in* client);

char* edit_message(char* message);

void close_connection(struct client* client);

void free_server(struct server* server);

#endif // !SERVER_H
