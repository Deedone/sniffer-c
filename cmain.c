#include "db.h"
#include "controls.h"
#include <string.h>
#include <dirent.h>
#include <arpa/inet.h>
#include <unistd.h>


void help();
void show_count(char* ip);
void show_stat(char* iface);
void show_all_stat();
void select_iface(char* iface);
int connect_or_run();
void print_results(int socket);
void start();
void stop();

int main(int argc, char** argv){

	int control = -1;
	if(argc == 1){
		help();
		return 0;
	}
	if(strcmp("start",argv[1]) == 0){
		start();
		return 0;
	}
	if(strcmp("stop",argv[1]) == 0){
		stop();
		return 0;
	}
	if(strcmp("show",argv[1]) == 0 && strcmp("count",argv[3]) == 0){
		show_count(argv[2]);
		return 0;
	}
	if(strcmp("stat",argv[1]) == 0){
		if(argc == 3){
			show_stat(argv[2]);
		}
		else{
			show_all_stat();
		}
		return 0;
	}
	if(argc == 4 && strcmp("select",argv[1]) == 0 && strcmp("iface",argv[2]) == 0){
		select_iface(argv[3]);
		return 0;
	}
	help();

	return 0;
}

void start(){
		printf("Starting daemon on eth0\n");
		int control = connect_or_run();
		if(control == -1) return;

		command_t cmd;
		cmd.cmd = START;
		send(control,&cmd,sizeof cmd,0);

		close(control);

}

void stop(){
		int control = connect_or_run();
		if(control == -1) return;

		command_t cmd;
		cmd.cmd = STOP;
		send(control,&cmd,sizeof cmd,0);

		close(control);
}

void show_all_stat(){
			int control = connect_or_run();
			if(control == -1) return;

			command_t cmd;
			cmd.cmd = ALL_STATS;
			if(send(control,&cmd,sizeof cmd,0) > 0)
				print_results(control);
			else
				perror("Connection error");
			close(control);
}

int connect_or_run(){
	int sock;
	if(make_ipc_socket(&sock,0) == -1){
		printf("Running daemon\n");
		system("sudo ./daemon");
		if(sock == -1 && make_ipc_socket(&sock,0) == -1){
			perror("Error starting daemon");
			return -1;
		}
	}

	return sock;
}

void help(){
	printf("Simple sniffer daemon \n"
			"Usage: \n"
			"\tstart - start daemon and begin sniffng on eth0\n"
			"\tstop - kill daemon and stop sniffing\n"
			"\tshow [ip] count - print number of packets received from ip address\n"
			"\tselect iface [iface] - select interface for sniffing eth0, wlan0, ethN,wlanN...\n"
			"\tstat [iface] - show all collected statistics for particular interface, if iface omitted - for all interfaces.\n"
			"\t--help - show this text\n"
			);


}

void print_results(int socket){
	char buf[255];
	do{
		int len = recv(socket, buf, 255,0);
		if(len == 0){
			perror("Err");
		}
		if(strncmp("SENDEND",buf,255) != 0){
				printf("%s",buf);
		}else{
			break;
		}

	}while(1);
}
	

void show_count(char* ip){

	struct in_addr addr;
	unsigned long total= 0;
	if(inet_aton(ip,&addr) == 0){
		printf("Error parsing ip address\n");
		return;
	}

	int control = connect_or_run();
	if(control == -1) return;
	command_t cmd;
	cmd.cmd = SHOW_COUNT;
	cmd.intarg = addr.s_addr;
	if(send(control,&cmd,sizeof cmd,0) > 0)
		print_results(control);
	else
		perror("Connection error");

}

void show_stat(char* iface){
	int control = connect_or_run();
	if(control == -1) return;

	command_t cmd;
	cmd.cmd = STATS;
	strcpy(cmd.chararg,iface);

	if(send(control,&cmd,sizeof cmd,0) > 0)
		print_results(control);
	else
		perror("Connection error");

}

void select_iface(char* iface){
	int control = connect_or_run();
	if(control == -1) return;

	command_t cmd;
	cmd.cmd = SELECT_IFACE;
	strcpy(cmd.chararg,iface);

	send(control,&cmd,sizeof cmd,0);

}

