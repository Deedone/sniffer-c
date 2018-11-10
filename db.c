#include "db.h"
static FILE* db_file = 0;
static char db_name[255];
static unsigned long db_size = 0;
static db_entry* db = 0;



int open_db(char* name){
	strcpy(db_name,name);
	strcat(db_name,".db");
	if(db_file  != 0){
		fclose(db_file );
	}
	if(db != 0){
		free(db);
		db_size = 0;
	}

	db_file  = fopen(db_name, "rb");
	if(db_file ==NULL) {
		perror("Error:");
		return 0;
	}
	fread(&db_size, sizeof db_size,1,db_file);
	db = (db_entry*)malloc(sizeof(db_entry)*db_size);
	fread(db,sizeof(db_entry),db_size,db_file);
	

	fclose(db_file );
	db_file = 0;

	return 1;
}

db_entry* get_by_ip(unsigned long addr){
	if(db_size == 0 || db == 0) return 0;
	unsigned long L = 0;
	unsigned long R = db_size-1;
	while(L <= R){
		unsigned long i = (L + R) / 2;
		db_entry* probe = &(db[i]);
		if(probe->addr == addr){
			return probe;
		}
		if(i == 0) break;//Structured like this to prevent R overflowing to superhigh values
		//Because of unsignedness of R and L expr L <= R dont capture sutiation like L = 0 R = -1

		if(probe->addr < addr){
			L = i + 1;
		}
		else if(probe->addr > addr){
			R = i - 1;
		}
	}
	return 0;
}
	
db_entry* insert_db(unsigned long addr){
	db_size++;
	db = (db_entry*)realloc(db,sizeof(db_entry)*db_size);
	unsigned long i = db_size - 1;
	while(i != 0){
		if(addr > db[i-1].addr){
			db[i].addr = addr;
			db[i].count= 0;
			return &(db[i]);
		}else{
			db[i] = db[i-1];
			i--;
		}
	}
	db[i].addr = addr;
	db[i].count= 0;
	return &(db[i]);
}

void add_db(unsigned long addr){
	db_entry* entry = 0;
	if(db_size >0){

		entry = get_by_ip(addr);
	}
	if(entry == 0){
		entry = insert_db(addr);
	}
	entry->count++;

}

void dump_db(){
	db_file = fopen(db_name,"wb");
	fwrite(&db_size, sizeof db_size,1,db_file);
	fwrite(db,sizeof(db_entry),db_size,db_file);

	fclose(db_file);
	db_file = 0;
}

void print_db(){

	for(unsigned long i= 0;i<db_size;i++){
		struct in_addr addr;
		addr.s_addr = db[i].addr;
		printf("%s - %lu\n",inet_ntoa(addr),db[i].count);
	}
}



