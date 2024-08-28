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
  
  /* 
   * Mutex for services, necessary
   * because we have thread that
   * changes service objects and
   * server that uses them to
   * decide who is occupied.
   */
  pthread_mutex_t services_mutex;

  /* Mutex for message queue */
  pthread_mutex_t msq_mutex;

  /* Thread for services */
  pthread_t services_thread;

  /* Amount of services in array */
  int services_amount;
  
  /* Message queue id */
  int msqid;

  /* Passive socket to accept connecitons */
  int sfd;
};

struct server* create_server(const char* ip, const int port, 
    const char* msq_path, int services_amount);

void run_server(struct server* server);

void* handle_msq_updates(void* arg);

void change_service_status(struct server* server, int port, enum service_status status);

void send_addr(struct server* server, struct client* client);

struct service* get_free_service(struct server* server);

void shutdown_connection(struct client* client);

void free_server(struct server* server);

#endif // !SERVER_H
