


#define CONTROL_SOCKET "/tmp/sniffer.socket"

typedef enum{
	START,
	STOP,
	SHOW_COUNT,
	SELECT_IFACE,
	STATS,
	ALL_STATS
} c_enum;

typedef struct{
	c_enum cmd;
	int intarg;
	char chararg[20];
} command_t;

//If server==1 server-side socket opened, else client-side
int make_ipc_socket(int* sockfd, int server);
