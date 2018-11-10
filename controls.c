#include "controls.h"
#include<stdio.h>	
#include<sys/un.h>
#include<sys/socket.h>
#include<unistd.h>

int make_ipc_socket(int* sockfd, int server){
	struct sockaddr_un addr;
	addr.sun_family = AF_UNIX;
	strcpy(addr.sun_path,CONTROL_SOCKET);
	if(server){
		unlink(addr.sun_path);
	}
	int len = strlen(addr.sun_path) + sizeof(addr.sun_family);

	*sockfd = socket(AF_UNIX,SOCK_STREAM,0);
	if(*sockfd < 0)
		return -1;
	if(server){
		if(bind(*sockfd, (const struct socaddr*)&addr, len)){
			return -1;
		}
	}else{
		if(connect(*sockfd, (const struct socaddr*)&addr, len)){
			return -1;
		}
	}

	
	return 1;
}
