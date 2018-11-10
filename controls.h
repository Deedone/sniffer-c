


#define CONTROL_SOCKET "/tmp/sniffer.socket"


//If server==1 server-side socket opened, else client-side
int make_ipc_socket(int* sockfd, int server);
