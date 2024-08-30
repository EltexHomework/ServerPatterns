#include "../headers/server.h"
#include <sys/socket.h>

/*
 * create_server - used to create an object of server
 * struct, initializes its fields.
 * @path - path to socket file
 *
 * Return: pointer to an object of server struct 
 */
struct server* create_server(const char* ip, const int port, const char* msq_path, int services_amount) {
  key_t key;
  struct server* server = (struct server*) malloc(sizeof(struct server));
  if (!server)
    print_error("malloc");
  
  /* Create message queue */
  key = ftok(msq_path, 1);
  if (key == -1)
    print_error("ftok");
  
  server->msqid = msgget(key, IPC_CREAT | 0666); 
  if (server->msqid == -1)
    print_error("msgget");
  
  /* Initialize mutex */
  if (pthread_mutex_init(&server->msq_mutex, NULL) != 0)
    print_error("pthread_mutex_init");
  
  /* Initialize services */
  server->services = (struct service**) malloc(services_amount * sizeof(struct service*)); 
  server->services_amount = services_amount;
  for (int i = 0; i < server->services_amount; i++) {
    server->services[i] = create_service(server->msqid, server->msq_mutex, i + 1); 
  }
  
  /* Initialize clients array*/
  server->clients = (struct client**) malloc(CLIENTS_AMOUNT * sizeof(struct client*));
  server->clients_amount = 0; 

  /* Initialzie sockaddr_un struct */
  server->serv.sin_family = AF_INET;
  server->serv.sin_addr.s_addr = inet_addr(ip); 
  server->serv.sin_port = htons(port);
  
  /* Get endpoint in host form */
  server->endpoint = atoe(&server->serv);

  /* Create a socket */
  server->sfd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
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
  /* Bind Endpoint to socket */
  if (bind(server->sfd, (struct sockaddr*) &server->serv, sizeof(server->serv)) == -1)
    print_error("bind");
  
  /* Set socket to passive mode */
  if (listen(server->sfd, CLIENTS_AMOUNT) == -1)
    print_error("listen");
  
  printf("SERVER: Server %s:%d started\n", 
         inet_ntoa(server->serv.sin_addr), 
         ntohs(server->serv.sin_port));
  
  /* Run services */
  for (int i = 0; i < server->services_amount; i++) {
    if (pthread_create(&server->services[i]->thread, NULL, 
                   run_service, (void *) server->services[i]) != 0)
      print_error("pthread_create");
  }
  
  /* Accept connections */
  while (1) {
    struct sockaddr_in addr;
    socklen_t client_size = sizeof(addr);

    int client_fd;
    client_fd = accept(server->sfd, (struct sockaddr*) &addr, &client_size);
    
    if (client_fd == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) { 
      check_user_messages(server); 
      continue;
    }
   
    /* Error occured */
    if (client_fd == -1) {
      print_error("accept");
    }
    /* Message received */
    else {
      struct client* client = (struct client*) malloc(sizeof(struct client));
      
      /* Initialize client */
      client->addr = &addr; 
      client->endpoint = atoe(&addr);
      client->fd = client_fd;
    
      /* Log client conncection */
      printf("SERVER: Client %s:%d connected\n", 
             client->endpoint->ip, client->endpoint->port);
      
      /* Add client to collection */
      add_client(server, client);
    }
  }
}


void check_user_messages(struct server* server) {
  for (int i = 0; i < server->clients_amount; i++) {
    struct client* client = server->clients[i];
    char* message = recv_message(client);

    if (message != NULL) {
      send_request(server, client, message); 
    }
    else {
      if (!(errno == EAGAIN || errno == EWOULDBLOCK)) {
        close_connection(client);
        delete_client(server, client);
      }
    }
  }
}


void send_request(struct server* server, struct client* client, char* message) {
  struct msg msg;
  
  msg.mtype = 1;
  msg.payload.client = *client;
  strncpy(msg.payload.message, message, sizeof(msg.payload.message));

  if (msgsnd(server->msqid, &msg, sizeof(msg), 0) == -1)
    print_error("msgsnd");
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
  bytes_read = recv(client->fd, &net_len, sizeof(net_len), MSG_DONTWAIT);
  
  /* Error occured*/
  if (bytes_read < 0) {
    if (errno == EAGAIN || errno == EWOULDBLOCK)
      return NULL;     

    print_error("recv");
  } 
  /* Connection closed */
  else if (bytes_read == 0) {
    return NULL;
  }
  
  /* Convert message length to Little Endian */
  message_len = ntohl(net_len);
  
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
 * add_client - used to add client object to array
 * of clients.
 * @server - pointer to an object of server struct
 * @client_addr - pointer to an object of sockaddr_un struct  
 * client_fd - descriptor for communication with client
 */
void add_client(struct server* server, struct client* client) {
  /* Check if server is full */
  if (server->clients_amount == CLIENTS_AMOUNT)
    return;

  client->id = server->clients_amount;
  client->server = server;
  
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
      free_endpoint(server->clients[i]->endpoint);
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
 * shutdown_connection - used to shutdown connection, client
 * will close file descriptor.
 * @client - pointer to an object of client struct
 */
void shutdown_connection(struct client* client) {
  shutdown(client->fd, SHUT_RDWR);
}

/*
 * free_server - free allocated memory for server 
 * @server - pointer to an object of server struct
 */
void free_server(struct server* server) {
  free_endpoint(server->endpoint);
  for (int i = 0; i < server->services_amount; i++) {
    free_service(server->services[i]);
  }
  pthread_mutex_destroy(&server->msq_mutex);
  msgctl(server->msqid, IPC_RMID, NULL);
  free(server);
}
