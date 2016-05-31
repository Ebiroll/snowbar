
/// Periodic polling of the socket server
void pollSocket();

/// Sends data on the socket
void sendDataOnSockets(const char *data,int len);

/// Initialize websocket
int socket_main(int argc, char **argv,int port,bool webClient);
