#include<stdio.h>	//For standard things
#include<stdlib.h>	//malloc
#include<unistd.h>
#include<string.h>	//memset
#include<netinet/ip.h>	//Provides declarations for ip header
#include<sys/socket.h>
#include<arpa/inet.h>
#include<time.h>
#include<sys/select.h>
#include<signal.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "db.h"
#include "controls.h"


void process_packet(unsigned char* , int);
int make_socket(int* sockfd, char* iface);
int process_command(unsigned char*, int socket);
void daemonize();



int main()
{

	daemonize();
	unsigned char *buffer = (unsigned char *)malloc(65536); //Its Big!
	int socket, control;
	time_t lastsave = 0;//Time of last DB dump
	fd_set readfds; //This is for select()
	open_db("eth0");

	if(make_socket(&socket, "eth0") == -1){
			perror("Socket error: ");
			return 1;
	}
	if(make_ipc_socket(&control,1) == -1){
			perror("IPC error: ");
			return 1;
	}
	listen(control,1);
	
	while(1)
	{
		FD_ZERO(&readfds);
		FD_SET(socket,&readfds);
		FD_SET(control,&readfds);
		int result = select(control+1,&readfds,NULL,NULL,NULL);//Wait for data to appear in sockets
		if(result == -1){
			perror("Error: ");
			break;
		}

		if(FD_ISSET(socket,&readfds)){
			//printf("Got inet dada\n");
			int data_size = recv(socket , buffer , 65536 , 0);
			if(data_size <0 ){
				perror("Error");
				//printf("Recvfrom error , failed to get packets\n");
				return 1;
			}
			//Now process the packet
			process_packet(buffer , data_size);
			if(time(0) - lastsave > 2){
				dump_db();
				//printf("Db dumped\n");
				lastsave = time(0);
			}
		}
		if(FD_ISSET(control,&readfds)){
			//printf("Got control data\n");
			int control2 = accept(control,NULL,NULL);
			if(recv(control2,buffer,65536,0)){

				//printf("Got command %s",buffer);

			}else{
				perror("Control socket error: ");
				shutdown(control2,0);
				break;
			}
			int should_stop = process_command(buffer, socket);
			if(should_stop) break;
		}
	}
	shutdown(socket,0); //Close socket
	shutdown(control,0);
	unlink(CONTROL_SOCKET);
	//printf("Finished");
	return 0;
}

void process_packet(unsigned char* buffer, int size)
{
	struct sockaddr_in source;
	//Get the IP Header part of this packet
	struct iphdr *iph = (struct iphdr*)buffer;
	source.sin_addr.s_addr = iph->saddr;
	////printf("%s ",inet_ntoa(source.sin_addr));
	add_db(iph->saddr);

}
int process_command(unsigned char* cmd, int socket){
	if(strcmp("stopcmd",cmd) == 0) return 1;
	if(cmd[0] == 'i'){
		setsockopt(socket , SOL_SOCKET , SO_BINDTODEVICE , cmd+1, strlen(cmd) );//passing cmd without first char
		open_db(cmd+1);
	}

	return 0;
}

int make_socket(int* sockfd, char* iface){
	*sockfd = socket(AF_INET , SOCK_RAW , IPPROTO_TCP);
	if(*sockfd < 0)
		return -1;
	setsockopt(*sockfd , SOL_SOCKET , SO_BINDTODEVICE , iface, strlen(iface)+ 1 );

	return 1;
}

void daemonize(){
	pid_t pid = fork();
	if (pid < 0)
			exit(EXIT_FAILURE);
	if (pid > 0)
			exit(EXIT_SUCCESS);

	if (setsid() < 0)
			exit(EXIT_FAILURE);

	signal(SIGCHLD, SIG_IGN);
	signal(SIGHUP, SIG_IGN);

	freopen("err.log","w",stderr);
	close(STDOUT_FILENO);
	close(STDIN_FILENO);
	umask(0);
}
