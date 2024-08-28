#include "../headers/client.h"
#include <arpa/inet.h>
#include <netinet/in.h>

/*
 * create_client - used to create an object of
 * client struct. Converts ip and port to Little Endian. 
 * @ip - IPv4 address of the server
 * @port - port of the server
 *
 * Return: pointer to an object of client struct
 */
struct client* create_client(const char* ip, const int port) {
  struct client* client = (struct client*) malloc(sizeof(struct client));
  if (!client)
    print_error("malloc");

  /* Initialzie sockaddr_in struct*/
  client->serv.sin_family = AF_INET;
  client->serv.sin_addr.s_addr = inet_addr(ip);
  client->serv.sin_port = htons(port);
  client->serv_endpoint = addr_to_endpoint(&client->serv);

  /* Open socket */
  client->sfd = socket(AF_INET, SOCK_STREAM, 0);
  if (client->sfd == -1)
    print_error("socket");

  return client;
}

/* run_client - used to conenct to server
 * specified int client->serv.
 * @client - pointer to an object of client struct
 */
void run_client(struct client* client) {
  socklen_t serv_size = sizeof(client->serv);   
  
  /* Connect to server */
  if (connect(client->sfd, (struct sockaddr*) &client->serv, serv_size) == -1)
    print_error("connect");
  
  printf("CLIENT: Connected to server %s:%d\n", client->serv_endpoint->ip, client->serv_endpoint->port);
  /* Process user input */
  process_input(client);
}

/*
 * process_input - used to receive user input
 * from stdin. Terminates received string, calls
 * send_message and waits for server response.
 * @client - pointer to an object of client struct
 */
void process_input(struct client* client) {
  char buffer[BUFFER_SIZE];
  
  /* Wait for user input */
  while (1) {
    printf("Enter message: ");
    
    /* Read user input */
    if (fgets(buffer, sizeof(buffer), stdin) == NULL)
      print_error("fgets");
    buffer[strlen(buffer) - 1] = '\0';

    /* Send user message */
    send_message(client, buffer);

    /* Receive answer */
    char* message = recv_message(client);
    if (message == NULL) {
      close_connection(client);
      break;
    }
    
    printf("SERVER: Server %s:%d send response: %s\n", client->serv_endpoint->ip, client->serv_endpoint->port, message);
    free(message);
  }
}

/*
 * send_message - used to send message to server. Sends
 * buffer length first, converted to Big Endian, then sends
 * message itself.
 * @client - pointer to an object of client struct
 * @buffer - string that needs to be sent
 */
void send_message(struct client* client, const char* buffer) {
  uint32_t buffer_len = strlen(buffer);
  uint32_t net_len = htonl(buffer_len);
  
  /* Send message length */
  if (send(client->sfd, &net_len, sizeof(net_len), 0) == -1)
    print_error("send");
  
  /* Log message len */
  printf("CLIENT: Send message len: %d\n", buffer_len);
  
  /* Send message */
  if (send(client->sfd, buffer, buffer_len, 0) == -1)
    print_error("send");
  
  /* Log message*/
  printf("CLIENT: Send message: %s\n", buffer);
}

/*
 * recv_message - used to receive message from server.
 * Receives message length first, converts to Little Endian,
 * allocates memory for message, then receives it all. Allocated
 * buffer must be freed manually.
 *
 * Return: string (message) if successful, NULL if connection terminated
 */
char* recv_message(struct client* client) {
  uint32_t net_len;
  uint32_t message_len;
  ssize_t bytes_read;
  ssize_t total_received;
  char* message;
  
  /* Receive message length */
  bytes_read = recv(client->sfd, &net_len, sizeof(net_len), 0);
  /* Error occured*/
  if (bytes_read < 0) {
    print_error("recv");
  } 
  /* Connection closed */
  else if (bytes_read == 0) {
    return NULL;
  }
  
  /* Convert message length to Little Endian */
  message_len = ntohl(net_len);
  
  printf("CLIENT: Received message length: %d\n", message_len);

  /* Allocate memory for message */
  message = (char*) malloc(message_len + 1);
  if (!message)
    print_error("malloc");
  
  /* Read all message */
  while (total_received < message_len) {
    bytes_read = recv(client->sfd, message + total_received, message_len - total_received, 0);
    if (bytes_read < 0) {
      free(message);
      print_error("recv");
    }
    total_received += bytes_read;
  }
  
  /* Terminate message */
  message[message_len] = '\0';

  return message;
}

/*
 * shutdown_connection - used to close connection with server.
 * Calls shutdown which sends EOF to socket. Server closes
 * conenction from its side.
 * @client - pointer to an object of client struct
 */
void shutdown_connection(struct client* client) {
  shutdown(client->sfd, SHUT_RDWR);
}

/*
 * close_connection - used to close connection with close
 * call.
 * @client - pointer to an object of client struct
 */
void close_connection(struct client* client) {
  close(client->sfd);
}

/*
 * free_client - used to free allocated memory for client
 * object.
 * @client - pointer to an object of client struct
 */
void free_client(struct client* client) {
  free(client->serv_endpoint);
  free(client);
}
