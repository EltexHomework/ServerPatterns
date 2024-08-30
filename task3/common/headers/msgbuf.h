#ifndef MSGBUF_H
#define MSGBUF_H

#include "common.h"
#include "../../server/headers/client.h"

struct user_request {
  struct client client;
  char message[BUFFER_SIZE];
};

struct msg {
  long mtype;
  struct user_request payload;
};

#endif // !MSGBUF_H

