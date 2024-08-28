#include "../headers/server.h"
#include <pthread.h>
#include <stdio.h>

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
  if (pthread_mutex_init(&server->services_mutex, NULL) != 0)
    print_error("pthread_mutex_init");
  
  if (pthread_mutex_init(&server->msq_mutex, NULL) != 0)
    print_error("pthread_mutex_init");

  /* Initialize services */
  server->services = (struct service**) malloc(services_amount * sizeof(struct service*)); 
  server->services_amount = services_amount;
  for (int i = 0; i < server->services_amount; i++) {
    int service_port = port + i + 1; 
    server->services[i] = create_service(ip, port + i + 1, server->msqid, server->msq_mutex, port); 
  }

  /* Initialzie sockaddr_un struct */
  server->serv.sin_family = AF_INET;
  server->serv.sin_addr.s_addr = inet_addr(ip); 
  server->serv.sin_port = htons(port);
  
  /* Get endpoint in host form */
  server->endpoint = atoe(&server->serv);

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
    pthread_create(&server->services[i]->thread, NULL, 
                   run_service, (void *) server->services[i]);
  }
  
  /* Start monitoring msq */
  pthread_create(&server->services_thread, NULL, handle_msq_updates, (void *) server);
  
  /* Accept connections */
  while (1) {
    struct sockaddr_in addr;
    socklen_t client_size = sizeof(addr);

    int client_fd;
    client_fd = accept(server->sfd, (struct sockaddr*) &addr, &client_size);
    
    /* Error occured */
    if (client_fd == -1) {
      print_error("accept");
    }
    /* Message received */
    else {
      struct client client;
      
      /* Initialize client */
      client.addr = &addr; 
      client.endpoint = atoe(&addr);
      client.fd = client_fd;
    
      /* Log client conncection */
      printf("SERVER: Client %s:%d connected\n", 
             client.endpoint->ip, client.endpoint->port);
      
      /* Send endpoint of service to client */
      send_addr(server, &client);
      
      /* Close conenction */
      shutdown_connection(&client);
      free_endpoint(client.endpoint);
    }
  }
}

/*
 * handle_msq_updates - handler to pass in thread that
 * monitors msq for new messages and call change_service_status
 * when message appears.
 * @arg - pointer to an object of server struct 
 */
void* handle_msq_updates(void* arg) {
  struct server* server = (struct server*) arg;

  while (1) {
    struct msg msg;
    
    /* Receive first message in queue */
    if (msgrcv(server->msqid, &msg, sizeof(msg), server->endpoint->port, 0) == -1)
      print_error("msgrcv");
    
    /* Change service status */
    change_service_status(server, msg.payload.sender_port, msg.payload.status); 
  }

  return NULL;
}

/*
 * change_service_status - used to change status of service
 * with specific port. Locks mutex to block get_free_service
 * function, finds service by port, changes its status.
 * @server - pointer to an object of server struct 
 * @port - port of service  
 * @status - status of service 
 */
void change_service_status(struct server* server, int port, enum service_status status) {
  pthread_mutex_lock(&server->services_mutex);
  
  /* Find service */
  for (int i = 0; i < server->services_amount; i++) {
    if (server->services[i]->endpoint->port == port) {
      /* Change service status */
      server->services[i]->status = status; 
      printf("SERVER: Changed %s:%d status to %d\n", 
             server->services[i]->endpoint->ip,
             server->services[i]->endpoint->port,
             status);
      break;
    }
  }

  pthread_mutex_unlock(&server->services_mutex);
}

/*
 * send_addr - used to send endpoint of service to
 * client. Sends it as a string, client must parse it
 * by ':'.
 * @server - pointer to an object of server struct
 * @client - pointer to an object of client struct 
 */
void send_addr(struct server* server, struct client* client) {
  struct service* service = get_free_service(server);
  char buffer[BUFFER_SIZE];
  uint32_t message_size;
  uint32_t net_size;

  /* All services are occupied*/
  if (service == NULL) {
    char* msg = "occupied";
    strncpy(buffer, msg, strlen(msg)); 
    buffer[strlen(msg)] = '\0';
    send_message(client, buffer);
    return;
  }
  
  /* Fill the buffer with address of service */
  snprintf(buffer, sizeof(buffer), "%s:%d", 
           service->endpoint->ip, service->endpoint->port);
  
  /* Send address of the service */
  send_message(client, buffer);
}

/*
 * get_free_service - used to find unoccupied service
 * in services array.
 * @server - pointer to an object of server struct
 *
 * Return: pointer to an object of service struct if
 * successful, NULL if free service not found
 */
struct service* get_free_service(struct server* server) {
  pthread_mutex_lock(&server->services_mutex);
  
  for (int i = 0; i < server->services_amount; i++) {
    if (server->services[i]->status == FREE) {
      pthread_mutex_unlock(&server->services_mutex);
      return server->services[i];  
    } 
  }

  pthread_mutex_unlock(&server->services_mutex);
  return NULL;
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
  pthread_cancel(server->services_thread);
  for (int i = 0; i < server->services_amount; i++) {
    free_service(server->services[i]);
  }
  pthread_mutex_destroy(&server->msq_mutex);
  pthread_mutex_destroy(&server->services_mutex);
  msgctl(server->msqid, IPC_RMID, NULL);
  free(server);
}
