#include "../headers/service.h"

/*
 * create_service - used to create an object of service struct.
 * @ip - ip address of service
 * @port - port of service
 * @msqid - id for msq for requests from listener server  
 * @msq_mutex - mutex to controll access to msq 
 * @id - id for msq messages (used to send messages)
 *
 * Return: pointer to an object of service struct 
 */
struct service* create_service(const char* ip, int port, 
                               int msqid, pthread_mutex_t msq_mutex, int id) {
  struct service* service = (struct service*) malloc(sizeof(struct service));
  if (!service)
    print_error("malloc");

  /* Initialize struct */
  service->addr.sin_family = AF_INET;
  service->addr.sin_addr.s_addr = inet_addr(ip);
  service->addr.sin_port = htons(port);
  service->status = FREE;
  service->msqid = msqid;
  service->id = id;
  service->endpoint = atoe(&service->addr);

  return service;
}

/*
 * run_service - used to create socket, bind it and
 * translate it to passive mode. Calls handle_client_connection
 * and waits for requests from listener server.
 * @arg - pointer that casted to service struct inside
 */
void* run_service(void* arg) {
  struct service* service = (struct service*) arg;

  /* Create socket */
  service->sfd = socket(AF_INET, SOCK_STREAM, 0);
  if (service->sfd == -1)
    print_error("socket");
  
  if (bind(service->sfd, (struct sockaddr*) &service->addr, sizeof(service->addr)) == -1)
    print_error("bind");
  
  if (listen(service->sfd, CLIENTS_AMOUNT) == -1)
    print_error("listen");
  
  /* Log start of service */
  printf("%s:%d : Service started\n",
         service->endpoint->ip, 
         service->endpoint->port);

  /* Wait for client connections */
  handle_client_connection(service);

  return NULL;
}

/*
 * handle_client_connection - used to wait for connections
 * on socket. sends notification to listener server about
 * occupation of service.
 * @service - pointer to an object of service struct
 */
void handle_client_connection(struct service* service) {
  /* Accept connections */
  while (1) {
    struct client client; 
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int cfd;
    
    /* Wait for client connection */
    cfd = accept(service->sfd, (struct sockaddr*) &client_addr, &client_len);
    if (cfd == -1)
      print_error("accept");
    
    /* Notify server of occupation */
    service->status = OCCUPIED;
    notify_server(service, OCCUPIED);

    /* Create client struct object */
    client.addr = &client_addr;
    client.endpoint = atoe(client.addr);
    client.fd = cfd;
    
    /* Log connection */
    printf("%s:%d : Client %s:%d connected. Starting conversation\n", 
           service->endpoint->ip, service->endpoint->port,
           client.endpoint->ip, client.endpoint->port);
    
    /* Start communication */
    communicate(service, &client);
    
    /* Free allocated memory */
    free_endpoint(client.endpoint);
  }
}

/*
 * communicate - used to communicate with client that is connected.
 * Receives message, edits it and sends back.
 * @service - pointert to an object of service struct
 * @client - pointer to an object of client struct
 */
void communicate(struct service* service, struct client* client) {
  while (1) {
    char* reply;
    char* message = recv_message(client);

    /* Shutdown called */
    if (message == NULL) {
      close_connection(client);
      printf("%s:%d : Client %s:%d disconnected\n",
             service->endpoint->ip, service->endpoint->port,
             client->endpoint->ip, client->endpoint->port);
      
      /* Send message to server */
      notify_server(service, FREE);
      break;
    }
    
    /* Log received message */
    printf("%s:%d : Received message from %s:%d : %s\n", 
           service->endpoint->ip, service->endpoint->port,
           client->endpoint->ip, client->endpoint->port,
           message);

    /* Edit received message */
    reply = edit_message(message);
    
    /* Send reply */
    send_message(client, reply);
    
    /* Log send reply */
    printf("%s:%d : Send reply to %s:%d : %s\n", 
           service->endpoint->ip, service->endpoint->port,
           client->endpoint->ip, client->endpoint->port,
           reply);

    /* Free allocated memory */
    free(reply);
    free(message);
  } 
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
  
  /* Send new message */
  if (send(client->fd, buffer, message_len, 0) == -1)
    print_error("send");
}

/*
 * notify_server - used to send message to server
 * through message queue.
 * @service - pointer to an object of service struct
 * @status - status of service (FREE or OCCUPIED) 
 */
void notify_server(struct service* service, enum service_status status) {
  struct msg msg;
  
  msg.mtype = service->id;
  msg.payload.status = status;
  msg.payload.sender_port = service->endpoint->port;
  
  pthread_mutex_lock(&service->mutex);

  if (msgsnd(service->msqid, &msg, sizeof(msg.payload), 0) == -1)
    print_error("msgsnd");

  pthread_mutex_unlock(&service->mutex);

  printf("%s:%d : Send notification to server\n",
         service->endpoint->ip, service->endpoint->port);
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
 * close_connection - used to close connection when client
 * called shutdown. Closes clients file descriptor and
 * frees memory allocated for client; 
 */
void close_connection(struct client *client) {
  close(client->fd);
}

/*
 * free_service - used to free allocated memory
 * for service struct.
 * @service - pointer to an object of service struct
 */
void free_service(struct service* service) {
  pthread_cancel(service->thread);
  close(service->sfd);
  free_endpoint(service->endpoint);
  free(service);
}
