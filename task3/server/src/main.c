#include "../headers/server.h"

struct server* server;

void cleanup();

int main(void) {
  server = create_server(SERVER_IP, SERVER_PORT, MSQ_FILE, SERVICES_AMOUNT);
  atexit(cleanup);
  run_server(server); 
  exit(EXIT_SUCCESS);
}

void cleanup() {
  free_server(server);  
}
