#ifndef SERVER_H
#define SERVER_H

#include "../../common/headers/common.h"
#include "../../common/headers/endpoint.h"
#include "client.h"

/**
 * Used to create server on internet adress family (AF_INET) with
 * TCP protocol. 
 */
struct server {
  /* Address of the server */
  struct sockaddr_in serv;
  
  /* Array of pointers to clients */
  struct client** clients;
  int clients_amount;

  /* Passive socket to accept connecitons */
  int sfd;
};

struct server* create_server(const char* ip, const int port);

void run_server(struct server* server);

void* handle_client_connection(void* arg);

void add_client(struct server* server, struct sockaddr_in* client_addr, int client_fd);

void delete_client(struct server* server, struct client* client);

void send_message(struct client* client, char buffer[BUFFER_SIZE]);

char* recv_message(struct client* client);

char* edit_message(char* message);

void shutdown_connection(struct client* client);

void close_connection(struct client *client);

void free_server(struct server* server);

#endif // !SERVER_H
