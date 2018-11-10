#include "db.h"
#include "controls.h"
#include <string.h>
#include <dirent.h>
#include <arpa/inet.h>


void help();
void show_count(char* ip);
void show_stat(char* iface);
void show_all_stat();
void close();
void select_iface(char* iface);

int main(int argc, char** argv){

	if(argc == 1){
		help();
		return 0;
	}
	if(strcmp("start",argv[1]) == 0){
		printf("Starting daemon on eth0\n");
		system("sudo ./daemon");
		return 0;
	}
	if(strcmp("stop",argv[1]) == 0){
		close();
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
	}
	if(argc == 4 && strcmp("select",argv[1]) == 0 && strcmp("iface",argv[2]) == 0){
		select_iface(argv[3]);
		return 0;
	}
	help();

	return 0;
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

void show_count(char* ip){
	struct in_addr addr;
	unsigned long total= 0;
	if(inet_aton(ip,&addr) == 0){
		printf("Error parsing ip address\n");
		return;
	}
	DIR* d;
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

				db_entry* data = get_by_ip(addr.s_addr);
				if(data == 0){
					printf("%s - No data\n",iface);
				}
				else{
					printf("%s - %lu\n",iface, data->count);
					total+=data->count;
				}
				free(iface);
			}


		}
		printf("Total - %lu\n",total);
		closedir(d);
	}


}

void show_stat(char* iface){
	open_db(iface);
	print_db();
}

void show_all_stat(){
	DIR* d;
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
				printf("%s: \n",iface);
				open_db(iface);
				print_db();
				free(iface);
			}
		}
	}
}

void close(){
		printf("Stopping daemon\n");
		int sock;
		if(make_ipc_socket(&sock,0) == -1){
			perror("Error:");
		}
		if(send(sock,"stopcmd",strlen("stopcmd")+1, 0) == -1){
			perror("Error:");
		}
		
		shutdown(sock,2);

}
void select_iface(char* iface){

		char* cmd = (char*)malloc(strlen(iface) + 2);
		cmd[0] = 'i';
		strcat(cmd,iface);
		int sock;
		if(make_ipc_socket(&sock,0) == -1){
			perror("Failed to connect to daemon: ");
		}
		if(send(sock,cmd,strlen(cmd)+1,0) == -1){
			perror("Failed to send data: ");
		}

		shutdown(sock,2);
}

