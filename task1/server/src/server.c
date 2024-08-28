#include "../headers/server.h"

/*
 * create_server - used to create an object of server
 * struct, initializes its fields.
 * @path - path to socket file
 *
 * Return: pointer to an object of server struct 
 */
struct server* create_server(const char* ip, const int port) {
  struct server* server = (struct server*) malloc(sizeof(struct server));
  if (!server)
    print_error("malloc");

  /* Initialzie sockaddr_un struct */
  server->serv.sin_family = AF_INET;
  server->serv.sin_addr.s_addr = inet_addr(ip); 
  server->serv.sin_port = htons(port);

  /* Initialize clients array */
  server->clients_amount = 0;
  server->clients = (struct client**) malloc(CLIENTS_AMOUNT * sizeof(struct client*));
  if (!server->clients)
    print_error("malloc");

  /* Create a socket */
  server->sfd = socket(AF_INET, SOCK_STREAM, 0);
  if (server->sfd == -1)
    print_error("socket");

  return server;
}

/*
 * run_server - used to bind server, set it
 * to passive mode and accept connections
 * @server - pointer to an object of server struct
 */
void run_server(struct server* server) {
  struct sockaddr_in client;
  socklen_t client_size = sizeof(client);

  /* Bind Endpoint to socket */
  if (bind(server->sfd, (struct sockaddr*) &server->serv, sizeof(server->serv)) == -1)
    print_error("bind");
  
  /* Set socket to passive mode */
  if (listen(server->sfd, CLIENTS_AMOUNT) == -1)
    print_error("listen");
  
  struct endpoint* serv_ep = addr_to_endpoint(&server->serv); 
  printf("SERVER: Server %s:%d started\n", serv_ep->ip, serv_ep->port);
  free(serv_ep);

  /* Accept connections */
  while (1) {
    int client_fd;
    client_fd = accept(server->sfd, (struct sockaddr*) &client, &client_size);
    
    /* Error occured */
    if (client_fd == -1) {
      print_error("accept");
    }
    /* Connection shutdown */
    else if (client_fd == 0) {
      break;
    } 
    /* Message received */
    else {
      struct endpoint* client_ep = addr_to_endpoint(&client); 
      printf("SERVER: Client %s:%d connected\n", client_ep->ip, client_ep->port);
      add_client(server, &client, client_fd);
      free(client_ep);
    }
  }
}

/*
 * add_client - used to add client object to array
 * of clients.
 * @server - pointer to an object of server struct
 * @client_addr - pointer to an object of sockaddr_un struct  
 * client_fd - descriptor for communication with client
 */
void add_client(struct server* server, struct sockaddr_in* client_addr, int client_fd) {
  /* Check if server is full */
  if (server->clients_amount == CLIENTS_AMOUNT)
    return;

  struct client* client = (struct client*) malloc(sizeof(struct client));

  /* Initialzie client struct */
  client->addr = client_addr;
  client->fd = client_fd;
  client->id = server->clients_amount;
  client->server = server;
  client->endpoint = addr_to_endpoint(client_addr);

  /* Create thread for client */
  if (pthread_create(&client->thread, NULL, handle_client_connection, (void *) client) != 0)
    print_error("pthread_create");

  /* Add client to array*/
  server->clients[server->clients_amount] = client;
  server->clients_amount++;
}

/*
 * delete_client - used to delete client object from
 * array of clients.
 * @server - pointer to an object of server struct
 * @client - pointer to an object of client struct
 */
void delete_client(struct server* server, struct client* client) {
  int i;
  
  /* Find necessary client */
  for (i = 0; i < server->clients_amount; i++) {
    if (server->clients[i]->id == client->id) {
      free(server->clients[i]);
      break;
    } 
  }

  /* Move i + 1 clients to left */
  for (; i < server->clients_amount; i++) {
     server->clients[i] = server->clients[i + 1]; 
  }
  
  /* Delete last pointer */
  server->clients[i + 1] = NULL; 
  server->clients_amount--;
}

/*
 * handle_client_connection - used int thread to
 * handle new messages from connected user. If
 * client calls shutdown, connection will be closed,
 * memory freed.
 * @arg - pointer to an object of client struct
 */
void* handle_client_connection(void* arg) {
  /* Cast arg to client struct*/
  struct client* client = (struct client*) arg;

  char buffer[BUFFER_SIZE];
  int bytes_read;
  
  while (1) {
    char* message = recv_message(client);
    /* Connection closed */
    if (message == NULL) {
      printf("SERVER: Client %s:%d disconnected\n", client->endpoint->ip, client->endpoint->port);
      close_connection(client);
      break;
    }
    
    /* Log message */
    printf("SERVER: Received message from client %s:%d: %s\n", client->endpoint->ip, client->endpoint->port, message);

    /* Edit message */
    char* new_message = edit_message(message);
    send_message(client, new_message);

    /* Free allocated memory */
    free(new_message);
    free(message);
  }

  return NULL;  
}

/*
 * send_message - used to send message to client. First
 * sends length of the buffer, then sends the message.
 * @client - pointer to an object of client struct 
 * @buffer - message
 */
void send_message(struct client* client, char buffer[BUFFER_SIZE]) {
  uint32_t message_len;
  uint32_t net_len;
  
  message_len = strlen(buffer);
  net_len = htonl(message_len);
  
  /* Send message length */
  if (send(client->fd, &net_len, sizeof(net_len), 0) == -1)
    print_error("send");
  
  printf("SERVER: Send message length: %d\n", message_len);

  /* Send new message */
  if (send(client->fd, buffer, message_len, 0) == -1)
    print_error("send");

  printf("SERVER: Server send message %s\n", buffer);
}

/*
 * recv_message - used to receive message from client. Receives
 * message length first, then allocates memory for message and
 * receives full message. Returned message should be freed manually.
 * @client - pointer to an object of client struct
 *
 * Return: string (message) if successful, NULL if connection closed 
 */
char* recv_message(struct client* client) {
  uint32_t net_len;
  uint32_t message_len;
  ssize_t bytes_read;
  ssize_t total_received;
  char* message;
  
  /* Receive message length */
  bytes_read = recv(client->fd, &net_len, sizeof(net_len), 0);
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
  
  printf("SERVER: Received message length: %d\n", message_len);

  /* Allocate memory for message */
  message = (char*) malloc(message_len + 1);
  if (!message)
    print_error("malloc");
  
  /* Read all message */
  while (total_received < message_len) {
    bytes_read = recv(client->fd, message + total_received, message_len - total_received, 0);
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
 * edit_message - used to add prefix "Server" to message.
 * Allocates new buffer. Return buffer should be free manually.
 * @message - message from client that needs to be changed
 * 
 * Return: string with prefix
 */
char* edit_message(char* message) {
  char* prefix = "Server";
  char* new_message = (char*) malloc(strlen(message) + strlen(prefix) + 2);
  if (!new_message)
    print_error("malloc");

  /* Add prefix to message */
  snprintf(new_message, strlen(message) + strlen(prefix) + 2, "%s %s", "Server", message);

  return new_message;
}

/*
 * shutdown_connection - used to shutdown connection, client
 * will close file descriptor.
 * @client - pointer to an object of client struct
 */
void shutdown_connection(struct client* client) {
  shutdown(client->fd, SHUT_RDWR);
  delete_client(client->server, client);
}

/*
 * close_connection - used to close connection when client
 * called shutdown. Closes clients file descriptor and
 * frees memory allocated for client; 
 */
void close_connection(struct client* client) {
  close(client->fd);
  delete_client(client->server, client);
}

/*
 * free_server - free allocated memory for server 
 * @server - pointer to an object of server struct
 */
void free_server(struct server* server) {
  for (int i = 0; i < server->clients_amount; i++) {
    shutdown_connection(server->clients[i]);
    free(server->clients[i]->endpoint);
    free(server->clients[i]);
  }
  free(server->clients);
  free(server);
}
