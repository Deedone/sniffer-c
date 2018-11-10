#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include<arpa/inet.h>

/* .db file structure:
 * [int] size - number of entries
 * [db_entry] * size - enties in ascending order
 */

 
typedef struct{
	unsigned long addr;
	unsigned long count;
} db_entry;

//Loads db to memory and sets it for all next operations
int open_db(char* name);
// Writes db to file
void dump_db();
// Prints db to stdout
void print_db();
// Adds 1 to counter of addr, if entry is nonexistant creates it and sets counter to 1
void add_db(unsigned long addr);
// Searches for in O(log(n)), if not found returns 0
db_entry* get_by_ip(unsigned long addr);

