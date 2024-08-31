#include "../headers/server.h"

/*
 * create_server - used to create an object of server
 * struct, initializes its fields.
 * @ip - ip of the server 
 * @port - port of the server
 *
 * Return: pointer to an object of server struct 
 */
struct server* create_server(const char* ip, const int port) {
  key_t key;
  struct server* server = (struct server*) malloc(sizeof(struct server));
  if (!server)
    print_error("malloc");
  
  /* Initialzie sockaddr_un struct */
  server->serv.sin_family = AF_INET;
  server->serv.sin_addr.s_addr = inet_addr(ip); 
  server->serv.sin_port = htons(port);
  
  /* Get endpoint in host form */
  server->endpoint = atoe(&server->serv);

  /* Create tcp socket */
  server->tcp_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server->tcp_fd == -1)
    print_error("socket");
  
  /* Create udp socket */
  server->udp_fd = socket(AF_INET, SOCK_DGRAM, 0);
  if (server->udp_fd == -1)
    print_error("socket");

  return server;
}

/*
 * run_server - used to bind server, set it
 * to passive mode enable monitor for fds 
 * @server - pointer to an object of server struct
 */
void run_server(struct server* server) {
  /* Bind Endpoint to sockets */
  if (bind(server->tcp_fd, (struct sockaddr*) &server->serv, sizeof(server->serv)) == -1)
    print_error("bind");
  
  if (bind(server->udp_fd, (struct sockaddr*) &server->serv, sizeof(server->serv)) == -1)
    print_error("bind");

  /* Set socket to passive mode */
  if (listen(server->tcp_fd, CLIENTS_AMOUNT) == -1)
    print_error("listen");
  
  
  printf("SERVER: Server %s:%d started\n", 
         inet_ntoa(server->serv.sin_addr), 
         ntohs(server->serv.sin_port));

  run_monitor(server); 
}

/*
 * run_select - used to run select monitor for
 * fds, when client is connected (TCP) or send
 * request (UDP) runs specific calls for protocols
 * @server - pointer to an object of server struct 
 */
void run_select(struct server* server) {
  int max_fd;
  int result;
  fd_set fds;

  FD_ZERO(&fds);
  FD_SET(server->tcp_fd, &fds);
  FD_SET(server->udp_fd, &fds);
  
  max_fd = (server->tcp_fd > server->udp_fd) 
    ? server->tcp_fd 
    : server->udp_fd;

  while (1) {  
    /* Make monitor for fds */
    result = select(max_fd + 1, &fds, NULL, NULL, NULL);
    if (result == -1) {
      print_error("select");
    }
    else {
      /* TCP connection */
      if (FD_ISSET(server->tcp_fd, &fds)) {
        communicate_tcp(server);      
      }
      /* UDP connection */
      if (FD_ISSET(server->udp_fd, &fds)) {
        communicate_udp(server);
      }
    }
  }
}

/*
 * run_poll - used to run poll monitor for
 * fds, when client is connected (TCP) or send
 * request (UDP) runs specific calls for protocols
 * @server - pointer to an object of server struct 
 */
void run_poll(struct server* server) {
  struct pollfd fds[2];
  int result;

  /* Initialize tcp fd */
  fds[0].fd = server->tcp_fd;
  fds[0].events = POLLIN; 
  
  /* Initialize udp fd */
  fds[1].fd = server->udp_fd;
  fds[1].events = POLLIN; 
  
  /* Wait for events in fds */
  while (1) {
    /* Create poll */
    result = poll(fds, 2, 0);
    if (result == -1) {
      print_error("poll");
    } else {
      /* TCP connection */
      if (fds[0].revents & POLLIN) {
        communicate_tcp(server);
      }
      /* UDP request */
      else if (fds[0].revents & POLLIN) {
        communicate_udp(server);
      }
    }
  }
}

/*
 * run_epoll - used to run epoll monitor for
 * fds, when client is connected (TCP) or send
 * request (UDP) runs specific calls for protocols
 * @server - pointer to an object of server struct 
 */
void run_epoll(struct server* server) {
  struct epoll_event ev_tcp, ev_udp, events[2];
  int epollfd, nfds;
  
  /* Create epoll */
  epollfd = epoll_create1(0);
  if (epollfd == -1)
    print_error("epoll");
  
  /* Initialize events */
  ev_tcp.events = EPOLLIN;
  ev_tcp.data.fd = server->tcp_fd;

  ev_udp.events = EPOLLIN;
  ev_udp.data.fd = server->udp_fd;
  
  /* Add events */
  if (epoll_ctl(epollfd, EPOLL_CTL_ADD, server->tcp_fd, &ev_tcp) == -1)
    print_error("epoll_ctl");

  if (epoll_ctl(epollfd, EPOLL_CTL_ADD, server->udp_fd, &ev_udp) == -1)
    print_error("epoll_ctl");
  
  while (1) {
    nfds = epoll_wait(epollfd, events, 2, -1);
    if (nfds == -1) {
      print_error("epoll_wait");
    }
    
    for (int i = 0; i < nfds; i++) {
      if (events[i].events & EPOLLIN && events[i].data.fd == server->tcp_fd) {
        communicate_tcp(server);
      } else if (events[i].events & EPOLLIN && events[i].data.fd == server->udp_fd) {
        communicate_udp(server);
      }
    }
  }
}

/*
 * communicate_tcp - used to communicated with client
 * on TCP protocol.
 * @server - pointer to an object of server struct
 */
void communicate_tcp(struct server* server) {
  int client_fd;
  struct client client;
  struct sockaddr_in client_addr;
  socklen_t client_len = sizeof(client);
  
  /* Accept connection */
  client_fd = accept(server->tcp_fd, (struct sockaddr*) &client_addr, &client_len);
  if (client_fd == -1)
    print_error("accept");
  
  /* Initialize client */
  client.fd = client_fd;
  client.addr = &client_addr;
  client.endpoint = atoe(&client_addr);
  
  /* Log connection */
  printf("SERVER: Client %s:%d connected\n",
         client.endpoint->ip,
         client.endpoint->port);

  /* Communicate with client */
  while (1) {
    char* reply;
    char* message = recv_tcp(&client);
    
    /* Shutdown called */
    if (message == NULL) {
      close_connection(&client);
      printf("SERVER: Client %s:%d disconnected\n",
             client.endpoint->ip, client.endpoint->port);
      break;
    }
    
    printf("SERVER: Received message from %s:%d: %s\n", 
           client.endpoint->ip, 
           client.endpoint->port, 
           message);
   
    /* Add prefix to message */
    reply = edit_message(message);
    
    /* Send reply to client */
    send_tcp(&client, reply);
    
    /* Log reply */
    printf("SERVER: Send response to %s:%d : %s\n",
           client.endpoint->ip, 
           client.endpoint->port,
           reply);

    /* Free allocated memory */
    free(reply);
    free(message);
  }

  free_endpoint(client.endpoint);
}

/*
 * communicate_udp - used to communicate with client
 * on UDP protocol.
 * @server - pointer to an object of server struct
 */
void communicate_udp(struct server* server) {
  struct sockaddr_in client;
  
  /* Wait for messages */
  while (1) {
    char* buffer = recv_udp(server, &client);
    char* reply = edit_message(buffer);
  
    /* Log received message */
    printf("SERVER: Received message from %s:%d: %s\n", 
           inet_ntoa(client.sin_addr), 
           ntohs(client.sin_port), 
           buffer);
  
    /* Send response */
    send_udp(server, &client, reply);
    
    /* Log reply */
    printf("SERVER: Send response to %s:%d : %s\n",
           inet_ntoa(client.sin_addr), 
           ntohs(client.sin_port), 
           reply);
    
    /* Free allocated memory */
    free(buffer);
    free(reply);
  } 
}

/*
 * send_tcp - used to send message to client via TCP. First
 * sends length of the buffer, then sends the message.
 * @client - pointer to an object of client struct 
 * @buffer - message
 */
void send_tcp(struct client* client, char buffer[BUFFER_SIZE]) {
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
 * recv_message - used to receive message from client. Receives
 * message length first, then allocates memory for message and
 * receives full message. Returned message should be freed manually.
 * @client - pointer to an object of client struct
 *
 * Return: string (message) if successful, NULL if connection closed 
 */
char* recv_tcp(struct client* client) {
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
 * send_udp - used to send message to client via UDP.
 * @server - pointer to an object of server struct
 * @client - pointer to address of the client (sockaddr_in)
 * @buffer - message
 */
void send_udp(struct server* server, struct sockaddr_in* client, char buffer[BUFFER_SIZE]) {
  ssize_t bytes_send;
  socklen_t client_len = sizeof(*client);

  bytes_send = sendto(server->udp_fd, buffer, strlen(buffer), 0, (struct sockaddr*) client, client_len);

  if (bytes_send == -1)
    print_error("sendto");
}

/*
 * recv_message - used to receive message from client via UDP.
 * Allocates memory for message, then receives it all. Allocated
 * buffer must be freed manually.
 * @server - pointer to an object of server struct 
 * @client - address of the client (sockaddr_in)
 *
 * Return: string (message) if successful, NULL if connection terminated
 */
char* recv_udp(struct server* server, struct sockaddr_in* client) {
  ssize_t bytes_read;
  socklen_t client_len;
  char* buffer = (char*) malloc(BUFFER_SIZE * sizeof(char));
  
  /* Get length of clients address */
  client_len = sizeof(*client);
  
  /* Receive message */
  bytes_read = recvfrom(server->udp_fd, buffer, BUFFER_SIZE, 0, (struct sockaddr*) client, &client_len);  
  if (bytes_read == -1)
    print_error("recvfrom");
  else if (bytes_read == 0)
    return NULL;

  return buffer;
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
 * close_connection - used to close connection after
 * client calls shutdown.
 * @client - pointer to an object of client struct
 */
void close_connection(struct client* client) {
  close(client->fd);
}

/*
 * free_server - free allocated memory for server 
 * @server - pointer to an object of server struct
 */
void free_server(struct server* server) {
  free_endpoint(server->endpoint);
  free(server);
}
