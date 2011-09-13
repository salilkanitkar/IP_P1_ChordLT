#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#include "chord_server.h"

RFC_db_rec *rfc_db_head = 0;
peer_info_t peer_info;
int is_p0 = FALSE;
int well_known_socket=-1;
int well_known_port=0;

int main(int argc, char *argv[])
{
	if (argc > 2) {
		printf("Incorrect number of Command line Arguments! \n");
		exit(-1);
	} else if (argc == 2 && (strcmp(argv[1], "--start") == 0) ) {
		/* This is node P0 */
		is_p0 = TRUE;
		printf("\nNode P0 is starting up..... \n");
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

		peer_info.chord_id = 0;
		populate_public_ip();
		well_known_port = CHORD_PORT;
		populate_port_num();

		printf("\n");
		printf("Node P0 is listening of following IP address and Port Number: \n");
		printf("IP Address: %s  Port Num: %d\n", peer_info.ip_addr, peer_info.portnum);

		server_listen();

	} else {

		populate_public_ip();
		well_known_port = CHORD_PORT;
		populate_port_num();

		//send generate_chord_id msg to P0 - include my ip and myport in this msg
		//receive chordid & successor from P0

	}

	printf("\n");
	return(0);
}

