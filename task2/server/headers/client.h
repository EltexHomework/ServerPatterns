#ifndef CLIENT_H
#define CLIENT_H

#include "../../common/headers/common.h"

/**
 * Used as data struct to specify clients
 * address, descriptor for communication 
 */
struct client {
  /* Clients address */
  struct sockaddr_in* addr;
  
  /* IP and port */
  struct endpoint* endpoint;

  /* File descriptor for communication */
  int fd;
};

#endif // !CLIENT_H
