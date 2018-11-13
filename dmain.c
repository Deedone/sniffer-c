#include <stdio.h>	//For standard things
#include <stdlib.h>	//malloc
#include <unistd.h>
#include <string.h>	//memset
#include <netinet/ip.h>	//Provides declarations for ip header
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <time.h>
#include <sys/select.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <net/ethernet.h>
#include <netpacket/packet.h>
#include <net/ethernet.h>
#include <dirent.h>

#include "db.h"
#include "controls.h"


void process_packet(unsigned char* , int);
int make_socket(int* sockfd, char* iface);
int process_command(char*, int* socket, int* control,int* sniff);
void daemonize();
void send_count(int addr, int control);
void send_all_stats(int control);
void send_stat(char* iface, int control);
void change_iface(char* iface, int* socket);

char cur_iface[30] = "eth0";

int main()
{

	daemonize();
	unsigned char *buffer = (unsigned char *)malloc(65536); //Its Big!
	if(buffer == NULL){
		perror("Failed to allocate frame buffer");
	}

	int socket, control;
	time_t lastsave = 0;//Time of last DB dump
	fd_set readfds; //This is for select()
	open_db("eth0");
	int sniff = 1;

	if(make_socket(&socket, "eth0") == -1){
			perror("Socket error");
			return 1;
	}
	if(make_ipc_socket(&control,1) == -1){
			perror("IPC error");
			return 1;
	}
	listen(control,1);
	
	while(1)
	{
		FD_ZERO(&readfds);
		if(sniff)
			FD_SET(socket,&readfds);
		FD_SET(control,&readfds);
		int result = select(((control>socket)?control:socket)+1,&readfds,NULL,NULL,NULL);//Wait for data to appear in sockets
		if(result == -1){
			perror("Error");
			break;
		}

		if(FD_ISSET(socket,&readfds)){
			//printf("Got inet dada\n");
			struct sockaddr_ll addr;
			socklen_t addr_len = sizeof(addr);
			int data_size = recvfrom(socket , buffer , 65536 , 0, (struct sockaddr*)&addr, &addr_len);

			if(data_size <0 ){
				perror("Error recvesting data");
				continue;
			}
			//Now process the packet
			if(addr.sll_pkttype != PACKET_OUTGOING){
				process_packet(buffer , data_size );
			}
			if(time(0) - lastsave > 2){
				dump_db();
				//printf("Db dumped\n");
				lastsave = time(0);
			}
		}
		if(FD_ISSET(control,&readfds)){
			//printf("Got control data\n");
			int control2 = accept(control,NULL,NULL);
			if(control2 == -1) perror("Control connection error");
			int len;
			if((len = read(control2,buffer,65536)) >= 0){
				printf("Command is %d %s\n",len,buffer);
				process_command(buffer, &socket, &control2, &sniff);
			}
			close(control2);
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
	struct sockaddr_in source, dest;
	//Get the IP Header part of this packet
	struct iphdr *iph = (struct iphdr*)(buffer+ sizeof(struct ethhdr));
	source.sin_addr.s_addr = iph->saddr;
	dest.sin_addr.s_addr = iph->daddr;
	printf("%s ",inet_ntoa(source.sin_addr));
	printf("%s\n",inet_ntoa(dest.sin_addr));
	add_db(iph->saddr);

}
int process_command( char* cmd, int* socket, int* control, int* sniff){
	command_t* com = (command_t*)cmd;
	switch(com->cmd){
		case STOP:
			*sniff = 0;
			close(*socket);
			close_db();
			break;
		case START:
			make_socket(socket, "eth0");
			open_db("eth0");
			*sniff = 1;
			break;
		case SHOW_COUNT:
			send_count(com->intarg, *control);
			break;
		case SELECT_IFACE:
			change_iface(com->chararg,socket);
			break;
		case ALL_STATS:
			send_all_stats(*control);
			break;
		case STATS:
			send_stat(com->chararg,*control);
			break;
	}

	return 0;
}

void change_iface(char* iface, int* socket){
			close(*socket);
			if(make_socket(socket,iface) == -1){
				perror("Iface changing error");
				close(*socket);
				make_socket(socket,cur_iface);
				return;
			}
			strcpy(cur_iface,iface);
			open_db(iface);
}

void send_count(int addr, int control){
	int db_isopen = get_db_size();
	char db_lastopen[20];
	if(db_isopen != 0){
		strcpy(db_lastopen,get_db_name());
		close_db();
	}

	unsigned long  total = 0;
	DIR* d;
	char sendbuf[255];
	struct dirent *ent;
	d = opendir(".");
	if (d){
		//Looping through all files in cwd
		while ((ent = readdir(d)) != NULL)
		{
			if(strstr(ent->d_name,".db") != NULL){
				//Removing .db from filenames
				char* iface = (char*)malloc(strlen(ent->d_name)-2);
				memcpy(iface,ent->d_name,strlen(ent->d_name)-3);
				iface[strlen(ent->d_name)-2] = '\0';
				open_db(iface);

				db_entry* data = get_by_ip(addr);
				printf("Ip: %u\n",addr);
				if(data == 0){
					snprintf(sendbuf,255,"%s - No data\n",iface);
					send(control,sendbuf,255,0);
				}
				else{
					snprintf(sendbuf,255,"%s - %lu\n",iface, data->count);
					send(control,sendbuf,255,0);
					total+=data->count;
				}
				free(iface);
			}
		}
	snprintf(sendbuf,255,"Total - %lu\n",total);
	send(control,sendbuf,255,0);
	closedir(d);
	}
	snprintf(sendbuf,255,"SENDEND");
	send(control,sendbuf,255,0);
	if(db_isopen != 0){
		open_db(db_lastopen);
	}
}

void send_all_stats(int control){
	int db_isopen = get_db_size();
	char db_lastopen[20];
	if(db_isopen != 0){
		strcpy(db_lastopen,get_db_name());
		close_db();
	}
	char sendbuf[255];
	DIR* d;
	struct dirent *ent;
	d = opendir(".");
	if (d){
		//Looping through all files in cwd
		while ((ent = readdir(d)) != NULL)
		{
			if(strstr(ent->d_name,".db") != NULL){
				//Removing .db from filenames
				ent->d_name[strlen(ent->d_name)-3] = 0;

				snprintf(sendbuf,255,"%s: \n",ent->d_name);
				send(control,sendbuf,255,0);
				open_db(ent->d_name);
				db_entry* db = get_db();
				int len = get_db_size();

				for(int i = 0;i<len;i++){
					struct in_addr addr;
					addr.s_addr = db[i].addr;
					snprintf(sendbuf,255,"%s - %lu\n",inet_ntoa(addr),db[i].count);
					send(control,sendbuf,255,0);
				}

			}
		}
	}
	snprintf(sendbuf,255,"SENDEND");
	send(control,sendbuf,255,0);
	if(db_isopen != 0){
		open_db(db_lastopen);
	}
}

void send_stat(char* iface, int control){
	int db_isopen = get_db_size();
	char db_lastopen[20];
	if(db_isopen != 0){
		strcpy(db_lastopen,get_db_name());
		close_db();
	}

	char sendbuf[255];
	open_db(iface);
	db_entry* db = get_db();
	int len = get_db_size();

	for(int i = 0;i<len;i++){
		struct in_addr addr;
		addr.s_addr = db[i].addr;
		snprintf(sendbuf,255,"%s - %lu\n",inet_ntoa(addr),db[i].count);
		send(control,sendbuf,255,0);
	}
	snprintf(sendbuf,255,"SENDEND");
	send(control,sendbuf,255,0);
	if(db_isopen != 0){
		open_db(db_lastopen);
	}
}


int make_socket(int* sockfd, char* iface){
	*sockfd = socket(AF_PACKET , SOCK_RAW , htons(ETH_P_ALL));
	if(*sockfd < 0)
		return -1;

	//Get iface index
	struct ifreq ifr;
	strcpy(ifr.ifr_name,iface);
	if(ioctl(*sockfd,SIOCGIFINDEX,&ifr) != 0){
		return -1;
	}
	struct sockaddr_ll cfg;
	memset(&cfg,0,sizeof cfg);
	cfg.sll_family = AF_PACKET;
	cfg.sll_protocol = htons(ETH_P_ALL);
	cfg.sll_ifindex = ifr.ifr_ifindex;

	//bind to interface
	if(bind(*sockfd,(struct sockaddr*)&cfg,sizeof cfg) == -1){
		return -1;
	}

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
