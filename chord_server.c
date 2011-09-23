#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#include "chord_server.h"

#define BUFLEN 1500

char PROTOCOL_STR[128] = "Chord-LT/1.0";
char msg[BUFLEN];
char msg_type[128];
int is_p0 = FALSE;
int well_known_socket=-1;
int well_known_port=0;
int well_known_listen_socket=-1;
int well_known_listen_port=0;

RFC_db_rec *rfc_db_head = 0;
peer_info_t peer_info;
peer_info_t peer_list[10];

void build_RegisterNode_msg();

int main(int argc, char *argv[])
{
	int i,port;
	char ip[128];
	char RFC_Dir[RFC_NUM_MAX][RFC_TITLE_LEN_MAX], buf[BUFLEN];
	

	if (argc == 2 && (strcmp(argv[1], "--start") == 0) ) {

		/* This is node P0 */
		is_p0 = TRUE;
		printf("\nNode P0 is starting up..... \n");

	} else if ( argc == 3 /*&& (strcmp(argv[2], "5000")==0)*/ ) {

		/* This is a Peer Node */
		strcpy(ip,argv[1]);
		port = atoi(argv[2]);
		is_p0 = FALSE;
		if ( test_if_P0_alive(ip, port) == -1 ) {
			printf("Node P0 is not up yet! \nStart up P0 via './chord_peer --start' and then start peers\n");
			exit(-1);
		}		
		printf("A Peer Node is startig up.....\n");

	} else {
		printf("Wrong set of arguments!\n");
		printf("Usage:\n");
		printf("To start up node P0: ./chord_peer --start\n");
		printf("To start up a peer: ./chord_peer <node-P0-ip> <node-P0-port>\n");
		exit(-1);
	}

	if (is_p0 == TRUE) {

		/* Initialize the RFC Database. RFC Database is a Linked List. Intially Node P0 will contain the entire Database */
		printf("Initializing RFC Database.....\n");
		for(i =0;i<RFC_NUM_MAX;i++)	
		{
			RFC_Dir[i][0]='\0';
		}

		/* This API Populates the RFC Titles Directory */
		populate_RFC_Directory(RFC_Dir);
		generate_RFC_Database(1000,6000,RFC_Dir);

		#ifdef DEBUG_FLAG
		//print_RFC_Database();
		//printf("Done Printing!\n");
		#endif
		
		sort_RFC_db();

	    #ifdef DEBUG_FLAG
	    print_RFC_Database();
        #endif
		
		peer_info.chord_id = 0;

		initialize_peer_list();

		populate_public_ip();
		well_known_port = CHORD_PORT;
		populate_port_num();

		/*TODO:Instead of below code.. this should be a call to put_in_peer_list()*/
		peer_info.successor[0].chord_id = 0;
		strcpy(peer_info.successor[0].ip_addr, peer_info.ip_addr);
		peer_info.successor[0].portnum = peer_info.portnum;
		peer_info.pred.chord_id = 0;
		strcpy(peer_info.pred.ip_addr, peer_info.ip_addr);
		peer_info.pred.portnum = peer_info.portnum;

		put_in_peer_list(peer_info.chord_id, peer_info.ip_addr, peer_info.portnum);

		peer_list[0].successor[0].chord_id = 0;
		strcpy(peer_list[0].successor[0].ip_addr, peer_info.ip_addr);
		peer_list[0].successor[0].portnum = peer_info.portnum;
		peer_list[0].pred.chord_id = 0;
		strcpy(peer_list[0].pred.ip_addr, peer_info.ip_addr);
		peer_list[0].pred.portnum = peer_info.portnum;

		print_peer_list();

		server_listen();

	} else {

		printf("P0s Parameters: %s \t  %d\n",ip,port);
		populate_public_ip();
		well_known_port = CHORD_PORT;
		populate_port_num();

		build_RegisterNode_msg();
		strcpy(msg_type, "RegisterNode");
		strcpy(buf, msg);
		send_message(ip, port, msg_type, buf);
		printf("RegisterNode Message sent to P0\n");		
	
		server_listen();
	
	}

	printf("\n");
	return(0);
}

void build_RegisterNode_msg()
{
        sprintf(msg, "GET RegisterNode %s\nIP:%s\nPort:%d\n", PROTOCOL_STR, peer_info.ip_addr, peer_info.portnum);
}


//void build_GetKey_msg()
//{
//        sprintf(msg, "GET Key %s\nIP:%s\nPort:%d\n", PROTOCOL_STR, peer_info.ip_addr, peer_info.portnum);
//}


