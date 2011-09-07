#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#include "chord_server.h"

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
		/* Intialize the RFC Database. RFC Database is a Linked List. Intially Node P0 will contain the entire Database */
		printf("Initializing RFC Database\n");
	}

	printf("test print\n");
	return(0);
}

