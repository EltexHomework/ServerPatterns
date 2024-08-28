#include "../headers/endpoint.h"
#include <arpa/inet.h>
#include <netinet/in.h>

/*
 * addr_to_endpoint - used to create an object of endpoint struct.
 * Converts sin_addr and sin_port in sockaddr_in struct for comfortable
 * usage.
 * @addr - pointer to an object of sockaddr_in struct
 *
 * Return: pointer to an object of endpoint struct
 */
struct endpoint* addr_to_endpoint(struct sockaddr_in* addr) {
  struct endpoint* endpoint = (struct endpoint*) malloc(sizeof(struct endpoint));
  
  endpoint->ip = inet_ntoa(addr->sin_addr);
  endpoint->port = htons(addr->sin_port);

  return endpoint;
}
