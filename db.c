#include "db.h"
static FILE* db_file = 0;
static char db_name[255];
static unsigned long db_size = 0;
static db_entry* db = 0;
static char db_filename[50]= "";


int open_db(char* name){
	strcpy(db_name,name);


	strncpy(db_filename,db_name,50);
	strncat(db_filename,".db",50);

	if(db_file  != 0){
		fclose(db_file );
	}
	if(db != 0){
		free(db);
		db = 0;
		db_size = 0;
	}

	db_file  = fopen(db_filename, "rb");
	if(db_file ==NULL) {
		printf("file %s\n",db_filename);
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
int get_db_size(){
	return db_size;
}

db_entry* get_db(){
	return db;
}

char* get_db_name(){
	return db_name;
}

void close_db(){
	dump_db();
	free(db);
	db = 0;
	db_size = 0;
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
		if(probe->addr < addr){
			L = i + 1;
		}
		else if(probe->addr > addr){
			R = i - 1;
			if(R == -1) break;
		}
	}
	return 0;
}
	
db_entry* insert_db(unsigned long addr){
	db_size++;
	db_entry* newdb = (db_entry*)realloc(db,sizeof(db_entry)*db_size);
	if(newdb == NULL){
		perror("Memory error");
		return 0;
	}
	db = newdb;
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
	if(entry == NULL){
		entry = insert_db(addr);
	}
	if(entry != NULL){
		entry->count++;
	}

}

void dump_db(){
	if(db_size == 0 || db == 0) return;
	db_file = fopen(db_filename,"wb");
	fwrite(&db_size, sizeof db_size,1,db_file);
	fwrite(db,sizeof(db_entry),db_size,db_file);

	fclose(db_file);
	db_file = 0;
}

void print_db(){
	if(db_size == 0 || db == 0) return;
	for(unsigned long i= 0;i<db_size;i++){
		struct in_addr addr;
		addr.s_addr = db[i].addr;
		printf("%s - %lu\n",inet_ntoa(addr),db[i].count);
	}
}



