#ifndef CLIENT_H
#define CLIENT_H

#include "../../common/headers/common.h"
#include "../../common/headers/endpoint.h"

/*
 * Used as client for connection to local address
 * family (AF_INET) server via TCP protocol. 
 */
struct client {
  /* Adress of the server */
  struct sockaddr_in serv;
  
  /* IP and port of server */
  struct endpoint* serv_endpoint;

  /* Server file descriptor*/
  int sfd;
};

struct client* create_client(const char* ip, const int port);

void run_client(struct client* client);

void process_input(struct client* client);

void send_message(struct client* client, const char* buffer);

char* recv_message(struct client* client);

void shutdown_connection(struct client* client);

void close_connection(struct client* client);

void free_client(struct client* client);
  
#endif // !CLIENT_H
