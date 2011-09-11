#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#include "chord_server.h"

RFC_db_rec *rfc_db_head = 0;
int is_p0 = FALSE;	
peer_info_t peer_info;

int main(int argc, char *argv[])
{
	if (argc > 2) {
		printf("Incorrect number of Command line Arguments! \n");
		exit(-1);
	} else if (argc == 2 && (strcmp(argv[1], "--start") == 0) ) {
		/* This is node P0 */
		is_p0 = TRUE;
		printf("Node P0 is starting up..... \n");
	} else if (argc == 1) {
		/* This is a Peer Node */
		is_p0 = FALSE;
		printf("A Peer Node is startig up.....\n");
	} else {
		printf("Wrong set of arguments! \n");
		exit(-1);
	}

	if (is_p0 == TRUE) {
		/* Initialize the RFC Database. RFC Database is a Linked List. Intially Node P0 will contain the entire Database */
		printf("Initializing RFC Database.....\n");
		generate_RFC_Database(1000,6000);
		#ifdef DEBUG_FLAG
		print_RFC_Database();
		#endif
	}

	server_listen();

	return(0);
}

