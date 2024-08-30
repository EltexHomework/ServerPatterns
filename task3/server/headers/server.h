#ifndef SERVER_H
#define SERVER_H

#include "../../common/headers/common.h"
#include "../../common/headers/endpoint.h"
#include "../../service/headers/service.h"
#include "client.h"

/**
 * Used to create server on internet adress family (AF_INET) with
 * TCP protocol. 
 */
struct server {
  /* Address of the server */
  struct sockaddr_in serv;
  
  /* Endpoint of server */
  struct endpoint* endpoint;

  /* Array of sub-servers (services) */
  struct service** services; 
  
  /* Array of clients */
  struct client** clients;

  /* Mutex for message queue */
  pthread_mutex_t msq_mutex;
  
  /* Amount of services in array */
  int services_amount;
  
  /* Amount of currently connected clients */
  int clients_amount;

  /* Message queue id */
  int msqid;

  /* Passive socket to accept connecitons */
  int sfd;
};

struct server* create_server(const char* ip, const int port, 
    const char* msq_path, int services_amount);

void run_server(struct server* server);

void check_user_messages(struct server* server);

char* recv_message(struct client* client);

void add_client(struct server* server, struct client* client);

void delete_client(struct server* server, struct client* client);

void send_request(struct server* server, struct client* client, char* message); 

void shutdown_connection(struct client* client);

void free_server(struct server* server);

#endif // !SERVER_H
