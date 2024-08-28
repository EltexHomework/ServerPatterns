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

struct endpoint* atoe(struct sockaddr_in* addr);

struct endpoint* stoe(char* buffer);

void free_endpoint(struct endpoint* endpoint);

#endif // !ENDPOINT_H

