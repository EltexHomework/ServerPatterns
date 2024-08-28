#ifndef ENDPOINT_H
#define ENDPOINT_H

#include "common.h"

/**
 * Used as data struct containing ip address and port
 * converter from sockaddr_in struct from network type (Big Endian)
 * to host type (Little Endian).
 */
struct endpoint {
  char* ip;
  int port;
};

struct endpoint* addr_to_endpoint(struct sockaddr_in* addr);

#endif // !ENDPOINT_H

