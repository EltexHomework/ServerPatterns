#include "../headers/endpoint.h"
#include <arpa/inet.h>
#include <netinet/in.h>

/*
 * atoe (address to endpoint) - used to create an object of endpoint struct.
 * Converts sin_addr and sin_port in sockaddr_in struct for comfortable
 * usage.
 * @addr - pointer to an object of sockaddr_in struct
 *
 * Return: pointer to an object of endpoint struct
 */
struct endpoint* atoe(struct sockaddr_in* addr) {
  struct endpoint* endpoint = (struct endpoint*) malloc(sizeof(struct endpoint));
  
  endpoint->ip = inet_ntoa(addr->sin_addr);
  endpoint->port = htons(addr->sin_port);

  return endpoint;
}

/*
 * stoe (string to endpoint) - used to parse string of patter
 * "x.x.x.x:xxxx" to ip and port in host form.
 * @buffer - string of specific pattern
 *
 * Return: pointer to an object of endpoint struct
 */
struct endpoint* stoe(char* buffer) {
  char* ip;
  char* port;
  struct endpoint* endpoint = (struct endpoint*) malloc(sizeof(struct endpoint));
   
  ip = strtok(buffer, ":");
  port = strtok(NULL, ":");

  endpoint->ip = ip; 
  endpoint->port = (unsigned short) atoi(port);  

  return endpoint;
}

/*
 * free_endpoint - used to free allocated memory
 * for endpoint struct object.
 * @endpoint - pointer to an object of endpoint struct
 */
void free_endpoint(struct endpoint* endpoint) {
  free(endpoint);
}
