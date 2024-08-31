#include "../headers/service.h"

/*
 * create_service - used to create an object of service struct.
 * @msqid - id for msq for requests from listener server  
 * @msq_mutex - mutex to controll access to msq 
 * @id - id for msq messages (used to send messages)
 *
 * Return: pointer to an object of service struct 
 */
struct service* create_service(int msqid, pthread_mutex_t msq_mutex, int id) {
  struct service* service = (struct service*) malloc(sizeof(struct service));
  
  if (!service)
    print_error("malloc");

  /* Initialize struct */
  service->msqid = msqid;
  service->mutex = msq_mutex;
  service->id = id;

  return service;
}

/*
 * run_service - used to log service start.
 * Calls handle_client_connection
 * and waits for requests from listener server.
 * @arg - pointer that casted to service struct inside
 */
void* run_service(void* arg) {
  struct service* service = (struct service*) arg;

  /* Log start of service */
  printf("%d : Service started\n",
         service->id);

  /* Wait for client connections */
  handle_client_requests(service);

  return NULL;
}

/*
 * handle_client_connection - used to wait for requests
 * from listener server and processes them.
 * @service - pointer to an object of service struct
 */
void handle_client_requests(struct service* service) {
  /* Accept connections */
  while (1) {
    char* reply;
    struct msg msg = recv_request(service);
     
    /* Log received message */
    printf("%d : Client %s:%d send message: %s\n", 
           service->id, 
           msg.payload.client.endpoint->ip, 
           msg.payload.client.endpoint->port,
           msg.payload.message);
    
    /* Add prefix to message */
    reply = edit_message(msg.payload.message);
    
    /* Send reply */
    send_message(&msg.payload.client, reply);
    
    /* Log reply */
    printf("%d : Send response to %s:%d : %s\n", 
           service->id, 
           msg.payload.client.endpoint->ip, 
           msg.payload.client.endpoint->port,
           msg.payload.message);

    free(reply);
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
 * recv_request - used to receive request from 
 * listener server through message queue 
 * @service - pointer to an object of service struct
 */
struct msg recv_request(struct service* service) {
  struct msg msg;
  
  pthread_mutex_lock(&service->mutex);
  
  if (msgrcv(service->msqid, &msg, sizeof(msg), 0, 0) == -1)
    print_error("msgrcv");
  
  pthread_mutex_unlock(&service->mutex);
  
  return msg;
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
  free(service);
}
