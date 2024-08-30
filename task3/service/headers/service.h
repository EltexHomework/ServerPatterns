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
  /* Thread for service */
  pthread_t thread;
  
  /* Mutex for msq */
  pthread_mutex_t mutex;

  /* Id for message queue */
  int msqid;

  /* Id for service */
  int id;
};

struct service* create_service(int msqid, pthread_mutex_t msq_mutex, int id);

void* run_service(void* arg);

void handle_client_requests(struct service* service);

void send_message(struct client* client, char buffer[BUFFER_SIZE]);

struct msg recv_request(struct service* service);

char* edit_message(char* message);

void close_connection(struct client *client);

void free_service(struct service* service);
#endif // !SERVICE_H
