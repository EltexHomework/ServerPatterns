#ifndef SERVICE_H
#define SERVICE_H

#include "../../common/headers/common.h"
#include "../../common/headers/endpoint.h"
#include "../../server/headers/client.h"
#include "../../common/headers/msgbuf.h"

/**
 * Service for communication with client. Containts
 * address in network form, endpoint in host form, thread,
 * id for message queue (IPC between listener server), id 
 * of messages for service.
 */
struct service {
  /* Network address */
  struct sockaddr_in addr;
  
  /* Host address */
  struct endpoint* endpoint;
  
  /* Thread for service */
  pthread_t thread;
  
  /* Mutex for msq */
  pthread_mutex_t mutex;

  /* Service status */
  enum service_status status;   

  /* Id for message queue */
  int msqid;

  /* Socket file descriptor */
  int sfd;

  /* Id for messages */
  int id; 
};

struct service* create_service(const char* ip, int port, 
                               int msqid, pthread_mutex_t msq_mutex, int id);

void* run_service(void* arg);

void handle_client_connection(struct service* service);

void communicate(struct service* service, struct client* client);

void send_message(struct client* client, char buffer[BUFFER_SIZE]);

void notify_server(struct service* service, enum service_status status);

char* recv_message(struct client* client);

char* edit_message(char* message);

void close_connection(struct client *client);

void free_service(struct service* service);
#endif // !SERVICE_H
