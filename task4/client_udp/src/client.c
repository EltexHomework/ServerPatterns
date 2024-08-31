#include "../headers/client.h"
#include <arpa/inet.h>
#include <sys/socket.h>

/*
 * create_client - used to create an object of
 * client struct. 
 * @ip - ip address of the server
 * @port - port of the server
 * Return: pointer to an object of client struct
 */
struct client* create_client(const char* ip, const int port) {
  struct client* client = (struct client*) malloc(sizeof(struct client));
  if (!client)
    print_error("malloc");

  /* Initialzie server sockaddr_un struct*/
  client->serv.sin_family = AF_INET;
  client->serv.sin_addr.s_addr = inet_addr(ip);
  client->serv.sin_port = htons(port);

  client->sfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (client->sfd == -1)
    print_error("socket");

  return client;
}

/* run_client - used to connect serv address
 * to file descriptor client->sfd
 * @client - pointer to an object of client struct
 */
void run_client(struct client* client) {
  socklen_t server_len = sizeof(client->serv);   
   
  if (connect(client->sfd, (struct sockaddr*) &client->serv, server_len) == -1)
    print_error("connect");

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

    printf("SERVER: Server %s:%d send response: %s\n", 
           inet_ntoa(client->serv.sin_addr), 
           ntohs(client->serv.sin_port), 
           message);
    free(message);
  }
}

/*
 * send_message - used to send message to server.  
 * @client - pointer to an object of client struct 
 * @buffer - message
 */
void send_message(struct client* client, char buffer[BUFFER_SIZE]) {
  ssize_t bytes_send;
  
  /* Send message to server */
  bytes_send = send(client->sfd, buffer, strlen(buffer), 0);

  if (bytes_send == -1)
    print_error("sendto");

  printf("CLIENT: Send message to %s:%d: %s\n", 
         inet_ntoa(client->serv.sin_addr), 
         ntohs(client->serv.sin_port), 
         buffer);
}

/*
 * recv_message - used to receive message from server.
 * Allocates memory for buffer which should be freed manually.
 *
 * Return: string (message) if successful, NULL if connection terminated
 */
char* recv_message(struct client* client) {
  ssize_t bytes_read;
  char* buffer = (char*) malloc(BUFFER_SIZE * sizeof(char));
  
  /* Receive message from server */ 
  bytes_read = recv(client->sfd, buffer, BUFFER_SIZE, 0);
  
  if (bytes_read == -1)
    print_error("recvfrom");
  else if (bytes_read == 0)
    return NULL;

  return buffer;
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
  free(client);
}
