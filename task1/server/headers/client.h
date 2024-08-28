#ifndef CLIENT_H
#define CLIENT_H

#include "../../common/headers/common.h"

/**
 * Used as data struct to specify clients
 * address, descriptor for communication and 
 * clients id (required for proper delete operations)
 */
struct client {
  /* Clients address */
  struct sockaddr_in* addr;
  
  /* IP and port */
  struct endpoint* endpoint;

  /* Pointer to connected server */
  struct server* server;
  
  /* Thread that handles new messages */
  pthread_t thread;
  
  /* File descriptor for communication */
  int fd;

  /* Identifier of user */
  int id;
};

#endif // !CLIENT_H
