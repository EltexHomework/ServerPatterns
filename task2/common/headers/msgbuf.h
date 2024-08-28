#ifndef MSGBUF_H
#define MSGBUF_H

enum service_status { OCCUPIED = 0, FREE = 1 };

struct connection_msg {
  enum service_status status;
  int sender_port;
};

struct msg {
  long mtype;
  struct connection_msg payload; 
};

#endif // !MSGBUF_H

