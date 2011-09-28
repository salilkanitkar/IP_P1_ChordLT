#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <sys/time.h>
#include <signal.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <errno.h>
#include <pthread.h>

#include "chord_server.h"

#define BUFLEN 60000
int db_init = 0;

RFC_db_rec *new_head=0;
int ll_flag = 0;

peer_info_t fixfingersarr[MAX_NUM_OF_PEERS];

int sync_send_message(char ip_addr[128], int portnum, char msg_type[128], char msg[BUFLEN]);

void initialize_peer_info()
{
	int i;

	peer_info.chord_id = -1;
	strcpy(peer_info.ip_addr, "");
	peer_info.portnum = -1;
	
	for ( i=0 ; i<2 ; i++ ) {
		peer_info.successor[i].chord_id = -1;
		strcpy(peer_info.successor[i].ip_addr, "");
		peer_info.successor[i].portnum = -1;
	}

	for ( i=0 ; i<3 ; i++ ) {
		peer_info.finger[i].finger_id = -1;
		peer_info.finger[i].finger_node.chord_id = -1;
		strcpy(peer_info.finger[i].finger_node.ip_addr, "");
		peer_info.finger[i].finger_node.portnum = -1;
	}
}

void print_details(peer_info_t p_info)
{
	printf("Chord_Id:%d IP:%s Port:%d Successor:%d %s %d Pred:%d %s %d\n", p_info.chord_id, p_info.ip_addr, p_info.portnum, p_info.successor[0].chord_id, p_info.successor[0].ip_addr, p_info.successor[0].portnum, p_info.pred.chord_id, p_info.pred.ip_addr, p_info.pred.portnum);
	fflush(stdout);
}


void print_finger_table(peer_info_t p_info)
{
	int i=0;
	printf("\nThe Finger Table At Node %d\n", p_info.chord_id);
	for ( i=0 ; i<3 ; i++ ) {
		printf("Finger %d) Start:%d Successor:%d\n", i+1, p_info.finger[i].finger_id, p_info.finger[i].finger_node.chord_id);
	}
        fflush(stdout);
}

void print_RFC_Database ()
{
	RFC_db_rec *p;
	int count=0;
	p = rfc_db_head;
	printf("\nThe RFC Database: \n");
	if ( p ) {
		do {
			printf("Key : %d Value:%d  Title:%s\n",p->key, p->value,p->RFC_title);
			p=p->next;
			count += 1;
		} while (p!=rfc_db_head);
	}
	printf("Number of RFCs ar this Node: %d\n", count);
	fflush(stdout);
}

int generate_random_number(int start, int end)
{
	return ( (rand()%(end - start)) + start ) ;
}


void populate_RFC_Directory(char RFC_Dir[][RFC_TITLE_LEN_MAX])
{
	FILE *fp;
	int i,len;
	char path[RFC_TITLE_LEN_MAX];

	fp = popen("/bin/ls ./", "r");
	if (fp == NULL) {
	    printf("Failed to run command\n" );
	    exit(-1);
  	}
  	
	
  	for(i=0;i<RFC_NUM_MAX && fgets(path,sizeof(path),fp)!=NULL;i++){
		len = strlen(path);
		if( path[len-1] == '\n' )
		    path[len-1] = 0;
		strcpy(RFC_Dir[i],path);
  	}

  	/* close */
	pclose(fp);
	
}

int check_chordID(int random)
{
	int i;
	for(i=0;i<MAX_NUM_OF_PEERS;i++)
	{
		if(peer_list[i].chord_id==random)
		{
			return -1;
		}
	}
	return 1;
}



int next_free_position()
{
	int i;
	for(i=0;i<MAX_NUM_OF_PEERS;i++)
	{
		if(peer_list[i].chord_id == -1)
			return i;
	}
	return -1;
}


void initialize_peer_list()
{
	int i;
	for(i=0;i<MAX_NUM_OF_PEERS;i++)
	{
		peer_list[i].chord_id = -1;
		peer_list[i].successor[0].chord_id = peer_list[i].successor[1].chord_id = -1;
		peer_list[i].pred.chord_id = -1;
		
		peer_list[i].portnum = -1;
		strcpy(peer_list[i].iface_name,"\0");
		strcpy(peer_list[i].ip_addr,"\0");

		strcpy(peer_list[i].successor[0].ip_addr,"\0");
		strcpy(peer_list[i].successor[1].ip_addr,"\0");
		strcpy(peer_list[i].pred.ip_addr,"\0");

		peer_list[i].successor[0].portnum = peer_list[i].successor[1].portnum = -1;
		peer_list[i].pred.portnum = -1;
	}
}

void put_in_peer_list(int chord_id, char ip_addr[128], int portnum)
{
	int i, j, next_free;

	for ( i=0 ; i<MAX_NUM_OF_PEERS ; i++ ) {
		if ( peer_list[i].chord_id > chord_id ) {
			next_free = i;
			j = i;
			while ( peer_list[j].chord_id != -1 )
				j += 1;
			while ( j > i) {
				peer_list[j].chord_id = peer_list[j-1].chord_id;
				strcpy(peer_list[j].ip_addr, peer_list[j-1].ip_addr);
				peer_list[j].portnum = peer_list[j-1].portnum;
				strcpy(peer_list[j].iface_name, peer_list[j-1].iface_name);

				peer_list[j].successor[0].chord_id = peer_list[j-1].successor[0].chord_id;
				strcpy(peer_list[j].successor[0].ip_addr, peer_list[j-1].successor[0].ip_addr);
				peer_list[j].successor[0].portnum = peer_list[j-1].successor[0].portnum;

				peer_list[j].pred.chord_id = peer_list[j-1].pred.chord_id;
				strcpy(peer_list[j].pred.ip_addr, peer_list[j-1].pred.ip_addr);
				peer_list[j].pred.portnum = peer_list[j-1].pred.portnum;
	
				j -= 1;
			}
			break;	
		} else if ( peer_list[i].chord_id == -1 ) {
			next_free = i;
			break;
		}
	}

	peer_list[next_free].chord_id = chord_id;
	strcpy(peer_list[next_free].ip_addr, ip_addr);
	peer_list[next_free].portnum = portnum;
}

void print_peer_list()
{
	int i;
	for(i=0;i<MAX_NUM_OF_PEERS;i++)
	{
		if (peer_list[i].chord_id != -1)
			print_details(peer_list[i]);
	}

}

int generate_ChordID(int start, int end)
{
        long int seed;
        int rndm,next_free;
        struct timeval ct;

        gettimeofday(&ct, NULL);
        seed = (ct.tv_sec +ct.tv_usec);
        srand(seed);

	while( check_chordID(rndm = generate_random_number(1,1023) ) == -1 );
	
	return rndm;
}

int check_rand_arr(int rndm, int rand_arr[RFC_NUM_MAX], int k)
{
	int i;
	for(i=0;i<k;i++) {
		if (rndm == rand_arr[i])
			return 0;
	}
	return 1;
}

void generate_RFC_Database (int start, int end, char RFC_Dir[][RFC_TITLE_LEN_MAX])
{
#ifdef DEBUG_FLAG
printf("Enter generate_RFC_database \n");
#endif

	long int seed;
	int rndm;
	struct timeval ct;
	int i, rand_arr[RFC_NUM_MAX], k=0;
//	FILE *fp = fopen()
	RFC_db_rec *p, *q;
  
	gettimeofday(&ct, NULL);
        seed = (ct.tv_sec +ct.tv_usec);
        srand(seed);

	for(i=0; i<RFC_NUM_MAX; i++){

		/* TODO: Make sure that same random number is not generated again. */
		do {
			rndm = generate_random_number(start, end);
		} while ( !check_rand_arr(rndm%1024, rand_arr, k) );
		rand_arr[k++] = rndm%1024;
		
		#ifdef DEBUG_FLAG
		printf("%d %d \n",rndm, rndm%1024);
		#endif

		p = (RFC_db_rec *)malloc(sizeof(RFC_db_rec));
		if (!p) {
			printf("Error while allocating memory!!\n");
			exit(-1);
		}		
		p->next = NULL;
		p->key = (int)rndm % 1024;
        	p->value = (int)rndm;
		strcpy(p->RFC_title, RFC_Dir[i]);
        	sprintf(p->RFC_body, "ID:%d Value:%d \n<<RFC Text Goes Here>>", p->key, p->value);

		if ( rfc_db_head == NULL) {
			rfc_db_head = p;
			q = p;
		} else {
			q->next=p;
			q=q->next;
		}
		p->next = rfc_db_head;
	}	

#ifdef DEBUG_FLAG
printf("Exit generate_RFC_database \n");
#endif

}

/* This is starting point of the lookup */
void * lookup() 
{
	//int chord_id = (int) param;

	//further processing on chord_id
	
	pthread_exit(NULL);
}

void populate_public_ip()
{

	struct ifaddrs *myaddrs, *ifa;
	void *in_addr;
	char buf[64], intf[128];

	strcpy(peer_info.iface_name, "");

	if(getifaddrs(&myaddrs) != 0) {
		printf("getifaddrs failed! \n");
		exit(-1);
	}

	for (ifa = myaddrs; ifa != NULL; ifa = ifa->ifa_next) {

		if (ifa->ifa_addr == NULL)
			continue;

		if (!(ifa->ifa_flags & IFF_UP))
			continue;

		switch (ifa->ifa_addr->sa_family) {
        
			case AF_INET: { 
				struct sockaddr_in *s4 = (struct sockaddr_in *)ifa->ifa_addr;
				in_addr = &s4->sin_addr;
				break;
			}

			case AF_INET6: {
				struct sockaddr_in6 *s6 = (struct sockaddr_in6 *)ifa->ifa_addr;
				in_addr = &s6->sin6_addr;
				break;
			}

			default:
				continue;
		}

		if (inet_ntop(ifa->ifa_addr->sa_family, in_addr, buf, sizeof(buf))) {
			if ( ifa->ifa_addr->sa_family == AF_INET && strcmp(ifa->ifa_name, "lo")!=0 ) {
				printf("\nDo you want the chord_peer to bind to %s interface?(y/n): ", ifa->ifa_name);
				scanf("%s", intf);
				if ( strcmp(intf, "n") == 0 )
					continue;
				sprintf(peer_info.ip_addr, "%s", buf);
				sprintf(peer_info.iface_name, "%s", ifa->ifa_name);
			}
		}
	}

	freeifaddrs(myaddrs);
	
	if ( strcmp(peer_info.iface_name, "") == 0 ) {
		printf("Either no Interface is up or you did not select any interface ..... \nchord_peer Exiting .... \n\n");
		exit(0);
	}

	 printf("\n\nMy public interface and IP is:  %s %s\n\n", peer_info.iface_name, peer_info.ip_addr);

}

void populate_port_num()
{

        struct sockaddr_in sock_server;
        int portnum = well_known_port;

        if ((well_known_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
                printf("error in socket creation");
                exit(1);
        }

        memset((char *) &sock_server, 0, sizeof(sock_server));
        sock_server.sin_family = AF_INET;
        sock_server.sin_port = htons(portnum);
        sock_server.sin_addr.s_addr = inet_addr(peer_info.ip_addr);

        /* Each server instance created should listen on a different port. Generate a random number between 1024 to 65535.
           Keep on generating new random numbers until bind succeeds.
         */
        while (bind(well_known_socket, (struct sockaddr *) &sock_server, sizeof(sock_server)) == -1) {
                portnum = rand() % ( (65535-1024) + 1024);
		portnum = generate_random_number(65400, 65500);
                sock_server.sin_port = htons(portnum);
        }

        if (listen(well_known_socket, 10) == -1) {
                printf("listen error");
                exit(1);
        }
	
	peer_info.portnum = portnum;
}

int get_rfc_1(char title[128], int value, char ip_addr[128], int portnum)
{
			#ifdef DEBUG_FLAG
			printf("Ekde!! IP addr: %s port %d\n",ip_addr,portnum);
			#endif
			char sendbuf[BUFLEN];

			struct sockaddr_in sock_client;
			int sock, slen = sizeof(sock_client), ret;

		        sock = socket(AF_INET, SOCK_STREAM, 0);
        		memset((char *) &sock_client, 0, sizeof(sock_client));

	        	sock_client.sin_family = AF_INET;
	        	sock_client.sin_port = htons(portnum);
		        sock_client.sin_addr.s_addr = inet_addr(ip_addr);

 		        ret = connect(sock, (struct sockaddr *) &sock_client, slen);
	        	if (ret == -1) {
                		printf("Connect failed! Check the IP and port number of the Sever! \n");
		                exit(-1);
        		}

			strcpy(sendbuf, "");
			sprintf(sendbuf, "GET GetRFC %s\nIP:%s\nPort:%d\nRFC-Value:%d\nFlag:0\n", PROTOCOL_STR, ip_addr, portnum, value);
		
			#ifdef DEBUG_FLAG
			printf("sendbuf:\n%s\n", sendbuf);
			#endif

	                if ( send(sock, sendbuf, BUFLEN, 0) == -1 ) {
        	        	printf("send failed ");
                	        exit(-1);
	                }

			FILE *fp = fopen(title, "wb");
                	char recvbuf[BUFLEN], *cp;
	                int bytes_read=1;

			#ifdef DEBUG_FLAG
			printf("Just before while (1)\n");
			#endif

        	        while ( 1 ) {
                	        bytes_read = recv(sock, recvbuf, 500, 0);
                        	/*if ( (cp = strstr(recvbuf, "FILEEND") ) != NULL ) { This was original line*/ 
                        	if ( bytes_read <= 0 ) {
                                	cp = '\0';	//old line was *cp = '\0'
	                             //   fwrite(recvbuf, 1, bytes_read-7, fp); This was removed coz we cant write -1 to the file
        	                        break;
                	        }
                        	fwrite(recvbuf, 1, bytes_read, fp);

                	}

		#ifdef DEBUG_FLAG
			printf("Receive Done\n");
		#endif
	                fclose(fp);

			close(sock);
			return (0);	
}

int rand_portnum=0;
int rand_sock=0;
void populate_random_port_num()
{

        struct sockaddr_in sock_server;
	rand_portnum = 65400;

        if ((rand_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
                printf("error in socket creation");
                exit(1);
        }

        memset((char *) &sock_server, 0, sizeof(sock_server));
        sock_server.sin_family = AF_INET;
        sock_server.sin_port = htons(rand_portnum);
        sock_server.sin_addr.s_addr = inet_addr(peer_info.ip_addr);

        while (bind(rand_sock, (struct sockaddr *) &sock_server, sizeof(sock_server)) == -1) {
                rand_portnum = rand() % ( (65535-1024) + 1024);
		rand_portnum = generate_random_number(65400, 65500);
                sock_server.sin_port = htons(rand_portnum);
        }

        if (listen(rand_sock, 10) == -1) {
                printf("listen error");
                exit(1);
        }
	
}

peer_info_t listen_on_random_port()
{
	struct sockaddr_in sock_server, sock_client;
        int slen = sizeof(sock_client);
        char recvbuf[BUFLEN];
        int client;
        int ret;
        int opt=1;
	char *a,*b,*c;
	peer_info_t t;

        if ((client = accept(rand_sock, (struct sockaddr *) &sock_client, &slen)) == -1) {
                printf("accept error");
                exit(1);
        }

        if ((ret = recv(client, recvbuf, BUFLEN, 0)) == -1) {
                printf("recv error: %d\n", ret);
                exit(1);
        }

        a = strtok(recvbuf, ":");
	b = strtok(NULL, ":");
	c = strtok(NULL, ":");

	t.chord_id = atoi(a);
	strcpy(t.ip_addr, b);
	t.portnum = atoi(c);
	#ifdef DEBUG_FLAG
	printf("In liten_on %d %s %d\n", t.chord_id, t.ip_addr, t.portnum);
	#endif

        close(client);

        return(t);
}

int get_rfc(char title[128], int value, char ip_addr[128], int portnum)
{
			char sendbuf[BUFLEN];

			struct sockaddr_in sock_client;
			int sock, slen = sizeof(sock_client), ret;

		        sock = socket(AF_INET, SOCK_STREAM, 0);
        		memset((char *) &sock_client, 0, sizeof(sock_client));

	        	sock_client.sin_family = AF_INET;
	        	sock_client.sin_port = htons(portnum);
		        sock_client.sin_addr.s_addr = inet_addr(ip_addr);

 		        ret = connect(sock, (struct sockaddr *) &sock_client, slen);
	        	if (ret == -1) {
                		printf("Connect failed! Check the IP and port number of the Sever! \n");
		                exit(-1);
        		}

			strcpy(sendbuf, "");
			sprintf(sendbuf, "GET GetRFC %s\nIP:%s\nPort:%d\nRFC-Value:%d\nFlag:1\n", PROTOCOL_STR, ip_addr, portnum, value);
		
			#ifdef DEBUG_FLAG
			printf("sendbuf:\n%s\n", sendbuf);
			#endif

	                if ( send(sock, sendbuf, BUFLEN, 0) == -1 ) {
        	        	printf("send failed ");
                	        exit(-1);
	                }

			FILE *fp = fopen(title, "wb");
                	char recvbuf[BUFLEN], *cp;
	                int bytes_read=1;

			#ifdef DEBUG_FLAG
			printf("Just before while (1)\n");
			#endif

                        while ( 1 ) {
                                bytes_read = recv(sock, recvbuf, 500, 0);
                                /*if ( (cp = strstr(recvbuf, "FILEEND") ) != NULL ) { This was original line*/
                                if ( bytes_read <= 0 ) {
                                        cp = '\0';      //old line was *cp = '\0'
                                     //   fwrite(recvbuf, 1, bytes_read-7, fp); This was removed coz we cant write -1 to the file
                                        break;
                                }
                                fwrite(recvbuf, 1, bytes_read, fp);

                        }

		#ifdef DEBUG_FLAG
			printf("Receive Done\n");
		#endif
	                fclose(fp);

			close(sock);
			return (0);
}

void fix_fingers_P0()
{
        int i, start, succ, p;

        for ( i=0 ; i<3 ; i++ ) {
                p = ( 1 << i );
                start = peer_info.chord_id + p;
                peer_info.finger[i].finger_id = start;
                if ( is_in_between(peer_info.chord_id, peer_info.successor[0].chord_id, start) ) {
                        peer_info.finger[i].finger_node.chord_id = peer_info.successor[0].chord_id;
                        strcpy(peer_info.finger[i].finger_node.ip_addr, peer_info.successor[0].ip_addr);
                        peer_info.finger[i].finger_node.portnum = peer_info.successor[0].portnum;
                } else {
			int j;
			for (j=0;j<MAX_NUM_OF_PEERS;j++) {
				if ( peer_list[j].chord_id == peer_info.successor[0].chord_id ) {
					if ( peer_list[j+1].chord_id == -1 ) {
		 	                	peer_info.finger[i].finger_node.chord_id = peer_list[0].chord_id;
                        			strcpy(peer_info.finger[i].finger_node.ip_addr, peer_list[0].ip_addr);
			                        peer_info.finger[i].finger_node.portnum = peer_list[0].portnum;
					} else {
		 	                	peer_info.finger[i].finger_node.chord_id = peer_list[j+1].chord_id;
                        			strcpy(peer_info.finger[i].finger_node.ip_addr, peer_list[j+1].ip_addr);
			                        peer_info.finger[i].finger_node.portnum = peer_list[j+1].portnum;
					}
				}
			}
                }

        }

	peer_info.successor[1].chord_id = peer_info.finger[1].finger_node.chord_id;
	strcpy(peer_info.successor[1].ip_addr, peer_info.finger[1].finger_node.ip_addr);
	peer_info.successor[1].portnum = peer_info.finger[1].finger_node.portnum;
		
}


void fix_fingers()
{
	int i, start, succ, p;
	char sendbuf[BUFLEN], msg_type[128];
	
	for ( i=0 ; i<3 ; i++ ) {
		p = ( 1 << i );
		start = peer_info.chord_id + p;
		peer_info.finger[i].finger_id = start;
		if ( is_in_between(peer_info.chord_id, peer_info.successor[0].chord_id, start) ) {
			peer_info.finger[i].finger_node.chord_id = peer_info.successor[0].chord_id;
			strcpy(peer_info.finger[i].finger_node.ip_addr, peer_info.successor[0].ip_addr);
			peer_info.finger[i].finger_node.portnum = peer_info.successor[0].portnum;
		} else {
			/* Send GetFinger to my successor. */
	                printf("Forwards are required to get %d Finger \n", i);
                        rand_sock = 0;
                        rand_portnum = 0;
                        populate_random_port_num();
                        sprintf(sendbuf, "GET GetFinger %s\nIP:%s\nPortnum:%d\nFinger-val:%d\n", PROTOCOL_STR, peer_info.ip_addr, rand_portnum, start);
                        strcpy(msg_type, "GetFinger");
                        send_message(peer_info.successor[0].ip_addr, peer_info.successor[0].portnum, msg_type, sendbuf);
                        peer_info_t t;
                        t = listen_on_random_port();
                        printf("The Requested finger is %d %s %d\n", t.chord_id, t.ip_addr, t.portnum);
                        peer_info.finger[i].finger_node.chord_id = t.chord_id;
                        strcpy(peer_info.finger[i].finger_node.ip_addr, t.ip_addr);
                        peer_info.finger[i].finger_node.portnum = t.portnum;
		}
		
	}

	peer_info.successor[1].chord_id = peer_info.finger[1].finger_node.chord_id;
	strcpy(peer_info.successor[1].ip_addr, peer_info.finger[1].finger_node.ip_addr);
	peer_info.successor[1].portnum = peer_info.finger[1].finger_node.portnum;

}

void * handle_messages(void *args)
{
	
	
	struct thread_args_t *thread_args = (struct thread_args_t *)args;

	char msg_type[128],msg[BUFLEN];
	int client_sock;
	
	
	int bytes_read, portnum, chord_id, successor_id,rfc_value, key_val, val, send_RFC_reply=0;
	char sendbuf[BUFLEN], *needle, ip_addr[128], *keyval;
	char title[128]= "", filename[128];
	char *p, *q, *r, *s, *t, *u, *v, *x, *xx, *y, *yy, *z, *zz;
	FILE *fp;

	client_sock = thread_args->client;
	strcpy(msg_type,thread_args->msg_type);
	strcpy(msg,thread_args->msg);

	if (strcmp(msg_type, "FetchRFC") == 0) {


		needle = strtok(msg, " ");
		needle = strtok(NULL, " ");
		needle = strtok(NULL, " ");
		keyval = strtok(NULL, " ");
		val = atoi(keyval);
		key_val = val % 1024;
		strcpy(title, needle);
		printf("\nThe RFC requested is: Key:%d Value %d Title: %s\n", key_val, val, title);

		peer_info_t sendt;
		if ( peer_info.chord_id == peer_info.pred.chord_id && peer_info.chord_id == peer_info.successor[0].chord_id ) {
			printf("Edge Case!\n");
			send_RFC_reply = 1;

		}
		else if ( is_in_between(peer_info.pred.chord_id, peer_info.chord_id, key_val) ) {
			/* This means that I have the RFC */
			send_RFC_reply = 1;

		} else if ( is_in_between(peer_info.chord_id, peer_info.successor[0].chord_id, key_val) ) {
			/* My Successor has the RFC. Send GetRFC to him to get the RFC body. */
			get_rfc(title, val, peer_info.successor[0].ip_addr, peer_info.successor[0].portnum);
			send_RFC_reply = 1;

		} else if ( peer_info.finger[1].finger_node.chord_id != peer_info.successor[0].chord_id && is_in_between(peer_info.finger[0].finger_node.chord_id, peer_info.finger[1].finger_node.chord_id, key_val) ) {
			printf("Forwards are required to fetch this RFC!!! \n");
			rand_sock = 0;
			rand_portnum = 0; 
			populate_random_port_num();
			sprintf(sendbuf, "GET ForwardGet %s\nIP:%s\nPortnum:%d\nRFC-value:%d\n", PROTOCOL_STR, peer_info.ip_addr, rand_portnum, val);
        	        strcpy(msg_type, "ForwardGet");
                	send_message(peer_info.finger[1].finger_node.ip_addr, peer_info.finger[1].finger_node.portnum, msg_type, sendbuf);
			peer_info_t t;
			t = listen_on_random_port();
			printf("The Requested RFC is at Node %d %s %d\n", t.chord_id, t.ip_addr, t.portnum);
			get_rfc(title, val, t.ip_addr, t.portnum);
			send_RFC_reply = 1;

		} else if ( peer_info.finger[2].finger_node.chord_id != peer_info.successor[0].chord_id && is_in_between(peer_info.finger[1].finger_node.chord_id, peer_info.finger[2].finger_node.chord_id, key_val) ) {
			printf("Forwards are required to fetch this RFC!!! \n");
			rand_sock = 0;
			rand_portnum = 0; 
			populate_random_port_num();
			sprintf(sendbuf, "GET ForwardGet %s\nIP:%s\nPortnum:%d\nRFC-value:%d\n", PROTOCOL_STR, peer_info.ip_addr, rand_portnum, val);
        	        strcpy(msg_type, "ForwardGet");
                	send_message(peer_info.finger[2].finger_node.ip_addr, peer_info.finger[2].finger_node.portnum, msg_type, sendbuf);
			peer_info_t t;
			t = listen_on_random_port();
			printf("The Requested RFC is at Node %d %s %d\n", t.chord_id, t.ip_addr, t.portnum);
			get_rfc(title, val, t.ip_addr, t.portnum);
			send_RFC_reply = 1;			
		}

		else {
			printf("Forwards are required to fetch this RFC!!! \n");
			rand_sock = 0;
			rand_portnum = 0; 
			populate_random_port_num();
			sprintf(sendbuf, "GET ForwardGet %s\nIP:%s\nPortnum:%d\nRFC-value:%d\n", PROTOCOL_STR, peer_info.ip_addr, rand_portnum, val);
        	        strcpy(msg_type, "ForwardGet");
                	send_message(peer_info.successor[0].ip_addr, peer_info.successor[0].portnum, msg_type, sendbuf);
			peer_info_t t;
			t = listen_on_random_port();
			printf("The Requested RFC is at Node %d %s %d\n", t.chord_id, t.ip_addr, t.portnum);
			get_rfc(title, val, t.ip_addr, t.portnum);
			send_RFC_reply = 1;
		}
		
		strcpy(filename, "");
		strcat(filename, title);
			
		fp = fopen(filename, "rb");

		while (	( bytes_read = fread(sendbuf, sizeof(char), 500, fp) ) > 0) {

			if ( send(client_sock, sendbuf, bytes_read, 0) < 0 ) {
				printf("Error in sending file data! \n");
				exit(-1);
			}

		}
		printf("Sending end line of RFC\n");
		send(client_sock, "FILEEND", 7, 0);
		
		fclose(fp);
		close(client_sock);

		printf("Node %d is done sending an RFC to a Client.\n\n", peer_info.chord_id);
		printf("\n\n");
		fflush(stdout);


	} else if (strcmp(msg_type, "RegisterNode") == 0) {


		/* Only P0 can receive RegisterNode */
		needle = strtok(msg, "\n");
		needle = strtok(NULL, "\n");
		q = strtok(NULL, "\n");

		p = strtok(needle, ":");
		p = strtok(NULL, ":");
		strcpy(ip_addr, p);

		r = strtok(q, ":");
		r = strtok(NULL, ":");
		portnum = atoi(r);
	
		fflush(stdout);
	
		chord_id = generate_ChordID(1, 1023);
		put_in_peer_list(chord_id, ip_addr, portnum);

		peer_info_t t ;
		t = setup_successor(chord_id);
		/*TODO:Also setup next successor*/

		printf("\n\nThe P2P System Details \n\n");
		print_peer_list();
		printf("\n\n");
		print_details(peer_info);

	        printf("Calling fix_fingers at P0... \n");
        	fix_fingers_P0(); /* This does not involve any */
	        print_finger_table(peer_info);

		printf("\n\nSENT NODEIDENTITY\n");

		sprintf(sendbuf, "POST NodeIdentity %s\nchord_id:%d\nsuccessor_id:%d\nsuccessor_IP:%s\nsuccessor_Port:%d\npred_id:%d\npred_IP:%s\npred_Port:%d\n", PROTOCOL_STR, chord_id, t.successor[0].chord_id, t.successor[0].ip_addr, t.successor[0].portnum, t.pred.chord_id, t.pred.ip_addr, t.pred.portnum);
		strcpy(msg_type, "NodeIdentity");
		send_message(ip_addr, portnum, msg_type, sendbuf);

		int i, ret;
		for ( i=0;i<MAX_NUM_OF_PEERS;i++) {

			if ( fixfingersarr[i].chord_id != -1 ) {

	                        printf("2) Send FixFingers msg to Node %d\n", fixfingersarr[i].chord_id);
				strcpy(sendbuf, "");
                	        sprintf(sendbuf, "POST FixFingers %s\n", PROTOCOL_STR);
                        	strcpy(msg_type, "FixFingers");
	                        ret = sync_send_message(fixfingersarr[i].ip_addr, fixfingersarr[i].portnum, msg_type, sendbuf);
			} else 
				break;
			
		}

		strcpy(sendbuf, "");
                sprintf(sendbuf, "POST FixFingers %s\n", PROTOCOL_STR);
                strcpy(msg_type, "FixFingers");
                ret = sync_send_message(ip_addr, portnum, msg_type, sendbuf);

		for ( i=0;i<MAX_NUM_OF_PEERS;i++) {
			if ( fixfingersarr[i].chord_id != -1 ) {
                                printf("3) Send GetDb msg to Node %d\n", fixfingersarr[i].chord_id);
                                strcpy(sendbuf, "");
                                sprintf(sendbuf, "GET GetDb %s\n", PROTOCOL_STR);
                                strcpy(msg_type, "GetDb");
                                send_message(fixfingersarr[i].ip_addr, fixfingersarr[i].portnum, msg_type, sendbuf);
			}
		}

                strcpy(sendbuf, "");
                sprintf(sendbuf, "GET GetDb %s\n", PROTOCOL_STR);
                strcpy(msg_type, "GetDb");
                send_message(ip_addr, portnum, msg_type, sendbuf);

	} else if (strcmp(msg_type, "NodeIdentity") == 0) {


		needle = strtok(msg, "\n");
		needle = strtok(NULL, "\n");
		q = strtok(NULL, "\n");
		s = strtok(NULL, "\n");
		t = strtok(NULL, "\n");
		x = strtok(NULL, "\n");
		y = strtok(NULL, "\n");
		z = strtok(NULL, "\n");

		p = strtok(needle, ":");
		p = strtok(NULL, ":");
		peer_info.chord_id = atoi(p);

		r = strtok(q, ":");
		r = strtok(NULL, ":");
		peer_info.successor[0].chord_id = atoi(r);

		u = strtok(s, ":");
		u = strtok(NULL, ":");
		strcpy(peer_info.successor[0].ip_addr, u);

		v = strtok(t, ":");
		v = strtok(NULL, ":");
		peer_info.successor[0].portnum = atoi(v);

		xx = strtok(x, ":");
		xx = strtok(NULL, ":");
		peer_info.pred.chord_id = atoi(xx);

		yy = strtok(y, ":");
		yy = strtok(NULL, ":");
		strcpy(peer_info.pred.ip_addr, yy);

		zz = strtok(z, ":");
		zz = strtok(NULL, ":");
		peer_info.pred.portnum = atoi(zz);

		printf("\n\nReceived the NodeIdentity Message from P0\n");
		print_details(peer_info);
		printf("\n\n");

	} else if ( strcmp(msg_type, "FixFingers") == 0) {
                printf("Calling fix_fingers at Node %d... \n", peer_info.chord_id);
                fix_fingers(); 
                print_finger_table(peer_info);
		send(client_sock, "FixFingers", 11, 0);

	} else if ( strcmp(msg_type, "GetDb") == 0 ) { 

		if ( !db_init ) {
			/* Build GetKey to be sent to Successor of current peer_info */
			sprintf(msg, "GET GetKey %s\nIP:%s\nPort:%d\nChord-Id:%d\n", PROTOCOL_STR, peer_info.ip_addr, peer_info.portnum, peer_info.chord_id);
	        	strcpy(msg_type, "GetKey");
		        strcpy(sendbuf, msg);
		        send_message(peer_info.successor[0].ip_addr, peer_info.successor[0].portnum, msg_type, sendbuf);
	        	printf("%s Message sent from Peer with ChordID %d to its successor with ChordID %d\n", msg_type, peer_info.chord_id, peer_info.successor[0].chord_id);
			db_init = 1;
		}

		if ( peer_info.chord_id == 0 ) {
			printf("\n\n The P2P System Details \n\n");
			print_peer_list();
			printf("\n\n");
		}

	}
	else if(strcmp(msg_type,"GetKey") == 0){
                needle = strtok(msg, "\n");
                needle = strtok(NULL, "\n");
                q = strtok(NULL, "\n");
		u = strtok(NULL, "\n");

                p = strtok(needle, ":");
                p = strtok(NULL, ":");
	        strcpy(ip_addr, p);

                r = strtok(q, ":");
                r = strtok(NULL, ":");
                portnum = atoi(r);

                s = strtok(u, ":");
                t = strtok(NULL, ":");
                chord_id = atoi(t);

                fflush(stdout);
		
		/* Now.. find the appropriate keys to be sent to node with ip_addr and portnum */
		RFC_db_rec *new_rfc,*p;
		int i, k;
		for (i=0;i<10;i++) {
			if ( peer_list[i].chord_id != -1 ) {
				if (peer_list[i].chord_id == chord_id)
					k=i;
			} else
				break;
		}
		
		if ( k == 0 ) {
			new_rfc = find_keys_to_transfer(peer_list[i-1].chord_id, chord_id);
		}
		else
			new_rfc = find_keys_to_transfer(peer_list[k-1].chord_id, chord_id);

		p = new_rfc;

		new_head = NULL;
		RFC_db_rec *new_p, *new_q;

		if ( new_rfc) {
		        do {
				#ifdef DEBUG_FLAG
        	        	printf("%d %d %s\n", p->key, p->value, p->RFC_title);
				#endif

				new_p = (RFC_db_rec *)malloc(sizeof(RFC_db_rec)*1);
				new_p->key = p->key;
				new_p->value = p->value;
				strcpy(new_p->RFC_title, p->RFC_title);
				if ( new_head ==NULL ) {
					new_head = new_p;
					new_q = new_p;
				} else {
					new_q->next = new_p;
					new_q = new_q->next;
				}
				new_p->next = new_head;

        		        p = p->next;

		        } while (p != new_rfc);
		}
		

		char listbuf[15000], tmpbuf[1500], sendlistbuf[15000];
		int count=0;
		strcpy(listbuf, "");
	
		if ( new_rfc ) { 		
			p = new_rfc;
			do {
				sprintf(tmpbuf, "%d:%d:%s\n", p->key, p->value, p->RFC_title);
				strcat(listbuf, tmpbuf);
				p = p->next;
				count += 1;
			} while ( p!= new_rfc ); 
		}
		fflush(stdout);

		//Build NodeList message
                sprintf(sendlistbuf, "POST NodeList %s\nIP:%s\nPortnum:%d\ncount:%d\n%s", PROTOCOL_STR, peer_info.ip_addr, peer_info.portnum, count, listbuf);
               	strcpy(msg_type, "NodeList");
                send_message(ip_addr, portnum, msg_type, sendlistbuf);
 		
		printf("NodeList Message sent: %d to %d\n", chord_id, peer_info.chord_id);
	}
	else if(strcmp(msg_type, "NodeList")==0) {

		int count, cnt=0, key, value, k=0, m, portnum;
		char title[128], *w, dbbuf[15000], db[15000], t1[128], t2[1500], ip_addr[128];
		RFC_db_rec *node_p, *node_q;

		strcpy(dbbuf, msg);

                needle = strtok(msg, "\n");
                needle = strtok(NULL, "\n");
                y = strtok(NULL, "\n");
		z = strtok(NULL, "\n");

		xx = strtok(needle, ":");
		xx = strtok(NULL, ":");
		strcpy(ip_addr, xx);

		yy = strtok(y, ":");
		yy = strtok(NULL, ":");
		portnum = atoi(yy);

                p = strtok(z, ":");
                p = strtok(NULL, ":");
                count = atoi(p);
		
		w = strtok(dbbuf, "\n");
		w = strtok(NULL, "\n");
		w = strtok(NULL, "\n");
		w = strtok(NULL, "\n");

		if ( count == 0 ) {
			printf("The RFC Database at this Node is Empty!! \n");
			return;
		}

		while ( cnt<count ) {

			w = strtok(NULL, "\n");
			strcpy(t2, w);
			k=0;m=0;
			while( t2[m] != ':' ) {
				t1[k] = t2[m];
				k++;m++;
			}
			t1[k] = '\0';
			key = atoi(t1);
                        k=0;m++;
                        while( t2[m] != ':' ) {
                                t1[k] = t2[m];
                                k++; m++;
                        }
                        t1[k] = '\0';
			value = atoi(t1);

                        k=0;m++;
                        while( t2[m] != '\0' ) {
                                t1[k] = t2[m];
                                k++;m++;
                        }
                        t1[k] = '\0';
			strcpy(title, t1);

			node_p = (RFC_db_rec *)malloc(sizeof(RFC_db_rec));
	                if (!node_p) {
        	                printf("Error while allocating memory!!\n");
                	        exit(-1);
	                }
        	        node_p->next = NULL;
                	node_p->key = key;
	                node_p->value = value;
        	        strcpy(node_p->RFC_title, title);

	                if ( rfc_db_head == NULL) {
        	                rfc_db_head = node_p;
                	        node_q = node_p;
	                } else {
        	                node_q->next=node_p;
                	        node_q=node_q->next;
	                }
        	        node_p->next = rfc_db_head;
			cnt++;

		}

		printf("Request for the actual RFC Files..... \n");

		int ret;
		node_p = rfc_db_head;

		do {

			printf("Fetching RFC with key: %d\n", node_p->key);
			ret = get_rfc_1(node_p->RFC_title, node_p->value, ip_addr, portnum);

		#ifdef DEBUG_FLAG
			printf("Starting Next Fetch\n");
		#endif

			node_p = node_p->next;


		} while (node_p != rfc_db_head);


		printf("RFC Db LL created! \n");
		print_details(peer_info);


	} else if(strcmp(msg_type, "PutKey")==0) {

		int count, cnt=0, key, value, k=0, m, portnum;
		char title[128], *w, dbbuf[15000], db[15000], t1[128], t2[1500], ip_addr[128];
		RFC_db_rec *node_p, *node_q, *old_head;

		strcpy(dbbuf, msg);

                needle = strtok(msg, "\n");
                needle = strtok(NULL, "\n");
                y = strtok(NULL, "\n");
		z = strtok(NULL, "\n");

		xx = strtok(needle, ":");
		xx = strtok(NULL, ":");
		strcpy(ip_addr, xx);

		yy = strtok(y, ":");
		yy = strtok(NULL, ":");
		portnum = atoi(yy);

                p = strtok(z, ":");
                p = strtok(NULL, ":");
                count = atoi(p);
		
		w = strtok(dbbuf, "\n");
		w = strtok(NULL, "\n");
		w = strtok(NULL, "\n");
		w = strtok(NULL, "\n");

		if ( count == 0 ) {
			printf("The RFC Database at this Node is Empty!! \n");
			return;
		}
		
		if(rfc_db_head != NULL){
			node_q = rfc_db_head;
			while(node_q -> next != rfc_db_head)
				node_q = node_q -> next;
		}
		old_head = rfc_db_head;
		while ( cnt<count ) {

			w = strtok(NULL, "\n");
			strcpy(t2, w);
			k=0;m=0;
			while( t2[m] != ':' ) {
				t1[k] = t2[m];
				k++;m++;
			}
			t1[k] = '\0';
			key = atoi(t1);
                        k=0;m++;
                        while( t2[m] != ':' ) {
                                t1[k] = t2[m];
                                k++; m++;
                        }
                        t1[k] = '\0';
			value = atoi(t1);

                        k=0;m++;
                        while( t2[m] != '\0' ) {
                                t1[k] = t2[m];
                                k++;m++;
                        }
                        t1[k] = '\0';
			strcpy(title, t1);

			node_p = (RFC_db_rec *)malloc(sizeof(RFC_db_rec));
	                if (!node_p) {
        	                printf("Error while allocating memory!!\n");
                	        exit(-1);
	                }
        	        node_p->next = NULL;
                	node_p->key = key;
	                node_p->value = value;
        	        strcpy(node_p->RFC_title, title);

	                if ( rfc_db_head == NULL) {
        	                rfc_db_head = node_p;
                	        node_q = node_p;
				rfc_db_head -> next = node_p;
	                } else {
				node_p->next = node_q->next;
        	                node_q->next = node_p;
				node_q = node_p;
	                }
			if(cnt == 0)
				rfc_db_head = node_p;
			cnt++;

		}

		printf("Request for the actual RFC Files..... \n");

		int ret;
		node_p = rfc_db_head;

		do {

			printf("Fetching RFC with key: %d\n", node_p->key);
			ret = get_rfc(node_p->RFC_title, node_p->value, ip_addr, portnum);

		#ifdef DEBUG_FLAG
			printf("Starting Next Fetch\n");
		#endif

			node_p = node_p->next;


		} while (node_p != old_head);


		printf("RFC Db LL created! \n");
		print_details(peer_info);
		
		/*create a RemoveNode Message for the predecessor of this node and send to P0*/
               	sprintf(sendbuf, "GET RemoveNode %s\nChord id:%d\nIP:%s\nPortnum:%d", PROTOCOL_STR, peer_info.pred.chord_id, peer_info.pred.ip_addr, peer_info.pred.portnum);
        	strcpy(msg_type, "RemoveNode");

		send_message(ip_p0, port_p0, msg_type, sendbuf); 
		printf("RemoveNode Message sent to P0 is\n %s \n", sendbuf);
		
	} else if (strcmp(msg_type,"RemoveNode")==0) {
				
		peer_info_t suc,pred;
		int pred_id,suc_id;
		int i,j;
		int ret;
	
                needle = strtok(msg, "\n");
                needle = strtok(NULL, "\n");
                y = strtok(NULL, "\n");
		z = strtok(NULL, "\n");
		
                p = strtok(needle, ":");
                p = strtok(NULL, ":");
                chord_id = atoi(p);
		
		xx = strtok(y, ":");
		xx = strtok(NULL, ":");
		strcpy(ip_addr, xx);

		yy = strtok(z, ":");
		yy = strtok(NULL, ":");
		portnum = atoi(yy);

		for(i=0;i<MAX_NUM_OF_PEERS;i++){
			if(chord_id == peer_list[i].chord_id){
				pred_id = i-1;
				if(peer_list[i+1].chord_id == -1)
					suc_id = 0;
				else
					suc_id = i+1;

				peer_list[pred_id].successor[0].chord_id = peer_list[suc_id].chord_id;
				strcpy(peer_list[pred_id].successor[0].ip_addr,peer_list[suc_id].ip_addr);
				peer_list[pred_id].successor[0].portnum = peer_list[suc_id].portnum;

				peer_list[suc_id].pred.chord_id = peer_list[pred_id].chord_id;
				strcpy(peer_list[suc_id].pred.ip_addr,peer_list[pred_id].ip_addr);
				peer_list[suc_id].pred.portnum = peer_list[pred_id].portnum;
				
				suc.chord_id = peer_list[suc_id].chord_id;
				strcpy(suc.ip_addr,peer_list[suc_id].ip_addr);
				suc.portnum = peer_list[suc_id].portnum;

				suc.successor[0].chord_id = peer_list[suc_id].successor[0].chord_id;
				strcpy(suc.successor[0].ip_addr,peer_list[suc_id].successor[0].ip_addr);
				suc.successor[0].portnum = peer_list[suc_id].successor[0].portnum;

				suc.pred.chord_id = peer_list[suc_id].pred.chord_id;
				strcpy(suc.pred.ip_addr,peer_list[suc_id].pred.ip_addr);
				suc.pred.portnum = peer_list[suc_id].pred.portnum;

				pred.chord_id = peer_list[pred_id].chord_id;
				strcpy(pred.ip_addr,peer_list[pred_id].ip_addr);
				pred.portnum = peer_list[pred_id].portnum;

				pred.successor[0].chord_id = peer_list[pred_id].successor[0].chord_id;
				strcpy(pred.successor[0].ip_addr,peer_list[pred_id].successor[0].ip_addr);
				pred.successor[0].portnum = peer_list[pred_id].successor[0].portnum;

				pred.pred.chord_id = peer_list[pred_id].pred.chord_id;
				strcpy(pred.pred.ip_addr,peer_list[pred_id].pred.ip_addr);
				pred.pred.portnum = peer_list[pred_id].pred.portnum;
				
				break;
			}
		}
		for(j=i+1;j<MAX_NUM_OF_PEERS;j++){
					
			peer_list[j-1].chord_id = peer_list[j].chord_id;
			strcpy(peer_list[j-1].ip_addr,peer_list[j].ip_addr);
			peer_list[j-1].portnum = peer_list[j].portnum;

			for(i=0;i<2;i++) {
				peer_list[j-1].successor[i].chord_id = peer_list[j].successor[i].chord_id;
				strcpy(peer_list[j-1].successor[i].ip_addr,peer_list[j].successor[i].ip_addr);
				peer_list[j-1].successor[i].portnum = peer_list[j].successor[i].portnum;
			}

			for(i=0;i<3;i++){
				peer_list[j-1].finger[i].finger_id = peer_list[j].finger[i].finger_id;
				peer_list[j-1].finger[i].finger_node = peer_list[j].finger[i].finger_node;
			}

			peer_list[j-1].pred.chord_id = peer_list[j].pred.chord_id;
			strcpy(peer_list[j-1].pred.ip_addr,peer_list[j].pred.ip_addr);
			peer_list[j-1].pred.portnum = peer_list[j].pred.portnum;
		}

		peer_list[j-1].chord_id = -1;
		strcpy(peer_list[j-1].ip_addr,"");
		peer_list[j-1].portnum = -1;

		if(suc.chord_id !=0){
			printf("\nSend NodeIdentiy msg to Successor of deleted node\n");
                	sprintf(sendbuf, "POST NodeIdentity %s\nchord_id:%d\nsuccessor_id:%d\nsuccessor_IP:%s\nsuccessor_Port:%d\npred_id:%d\npred_IP:%s\npred_Port:%d\n", PROTOCOL_STR, suc.chord_id, suc.successor[0].chord_id, suc.successor[0].ip_addr, suc.successor[0].portnum, suc.pred.chord_id, suc.pred.ip_addr, suc.pred.portnum);
               		strcpy(msg_type, "NodeIdentity");
	        	send_message(suc.ip_addr, suc.portnum, msg_type, sendbuf);
		}
		
		if(pred.chord_id != 0){
			printf("Send NodeIdentiy msg Predecessor of the deleted node\n");
                	sprintf(sendbuf, "POST NodeIdentity %s\nchord_id:%d\nsuccessor_id:%d\nsuccessor_IP:%s\nsuccessor_Port:%d\npred.chord_id:%d\npred_IP:%s\npred_Port:%d\n", PROTOCOL_STR, pred.chord_id, pred.successor[0].chord_id, pred.successor[0].ip_addr, pred.successor[0].portnum, pred.pred.chord_id, pred.pred.ip_addr, pred.pred.portnum);
               		strcpy(msg_type, "NodeIdentity");
	        	send_message(pred.ip_addr, pred.portnum, msg_type, sendbuf);
		}

		if(suc.chord_id !=0){
			strcpy(sendbuf, "");
                	sprintf(sendbuf, "POST FixFingers %s\n", PROTOCOL_STR);
        	        strcpy(msg_type, "FixFingers");
	                ret = sync_send_message(suc.ip_addr, suc.portnum, msg_type, sendbuf);
		}

		if(pred.chord_id != 0){
			strcpy(sendbuf, "");
                	sprintf(sendbuf, "POST FixFingers %s\n", PROTOCOL_STR);
        	        strcpy(msg_type, "FixFingers");
	                ret = sync_send_message(pred.ip_addr, pred.portnum, msg_type, sendbuf);
		}
		printf("Peer with chord id %d removed\n",chord_id);
		printf("Updated peer List is:\n");
		print_peer_list();

	} else if ( strcmp(msg_type, "PrintRFCDb")==0 ) {

	        printf("The RFC db at this node \n");
	        print_RFC_Database();

	}

	else if(strcmp(msg_type, "GetRFC") == 0 ){ // Request for the final Server who HAS the RFC
		//this message will build the response with ACTUAL body of RFC.
		char *w, *u, *v;
                needle = strtok(msg, "\n");
                needle = strtok(NULL, "\n");
                q = strtok(NULL, "\n");
		w = strtok(NULL, "\n");
		u = strtok(NULL, "\n");

                p = strtok(needle, ":");
                p = strtok(NULL, ":");
                strcpy(ip_addr, p);

                r = strtok(q, ":");
                r = strtok(NULL, ":");
                portnum = atoi(r);

		s = strtok(w,":");
		s = strtok(NULL,":");
		rfc_value = atoi(s);

		v = strtok(u,":");
		v = strtok(NULL,":");
		ll_flag = atoi(v);

		//printf("In GetRFC, searching for %d\n", rfc_value);
		//Look up the RFC-value in the rfc_db_head of the current server to get the file name corresponding to the RFC-value
		
		RFC_db_rec *hh;
		if ( ll_flag == 1 )
			hh = rfc_db_head;
		else {
			hh = new_head;
			ll_flag = 0;
		}
		if ( hh ) {
			do {
				if(hh->value == rfc_value)
				{
					strcpy(filename, "");
					strcpy(filename, hh->RFC_title);
					break;
				}
				hh = hh->next;
			} while ( hh != rfc_db_head );
		}

		//printf("In GetRFC, about to send file %s\n", filename);
		//open file with filename and send to ip_addr and portnum
                fp = fopen(filename, "rb");

                while ( ( bytes_read = fread(sendbuf, sizeof(char), 500, fp) ) > 0) {

                        if ( send(client_sock, sendbuf, bytes_read, 0) < 0 ) {
                                printf("Error in sending file data! \n");
                                exit(-1);
                        }

                }
                printf("Sending end line of RFC\n");
                send(client_sock, "FILEEND", 7, 0);

                fclose(fp);
                close(client_sock);

                printf("Node %d is done sending an RFC to requesting server.\n\n", peer_info.chord_id);
                printf("\n\n");
                fflush(stdout);
		//This guy's Job is done!
		//Now, the requesting server (in sync_send() mode) will accept this file and return it to the requesting client
	
	}
	else if(strcmp(msg_type, "ForwardGet") == 0){ //Response to the original server after the lookup forwarding

                char *w;
                needle = strtok(msg, "\n");
                needle = strtok(NULL, "\n");
                q = strtok(NULL, "\n");
                w = strtok(NULL, "\n");

                p = strtok(needle, ":");
                p = strtok(NULL, ":");
                strcpy(ip_addr, p);

                r = strtok(q, ":");
                r = strtok(NULL, ":");
                portnum = atoi(r);

                s = strtok(w,":");
                s = strtok(NULL,":");
                fflush(stdout);
                rfc_value = atoi(s);

		printf("ForwardGet received! %d %d %d %d\n", peer_info.chord_id, peer_info.successor[0].chord_id, rfc_value, rfc_value%1024);
		if ( is_in_between(peer_info.chord_id, peer_info.successor[0].chord_id, rfc_value%1024) ) {
                        /* My Successor has the RFC. Reply back to the requesting peer the chord id of my successor */
		        struct sockaddr_in fwd_sock_client;
		        int fwd_sock, fwd_slen = sizeof(fwd_sock_client), fwd_ret;
			char fwd_buf[BUFLEN] = "";
			sprintf(fwd_buf, "%d:%s:%d:", peer_info.successor[0].chord_id, peer_info.successor[0].ip_addr, peer_info.successor[0].portnum);
			printf("Sending %s\n", fwd_buf);

		        fwd_sock = socket(AF_INET, SOCK_STREAM, 0);
		        memset((char *) &fwd_sock_client, 0, sizeof(fwd_sock_client));

		        fwd_sock_client.sin_family = AF_INET;
		        fwd_sock_client.sin_port = htons(portnum);
		        fwd_sock_client.sin_addr.s_addr = inet_addr(ip_addr);

		        fwd_ret = connect(fwd_sock, (struct sockaddr *) &fwd_sock_client, fwd_slen);
		        if (fwd_ret == -1) {
                		printf("Connect failed! Check the IP and port number of the Sever! \n");
		                exit(-1);
		        }

		        if ( send(fwd_sock, fwd_buf, BUFLEN, 0) == -1 ) {
		                printf("send failed ");
		                exit(-1);
		        }

		        close(fwd_sock);
        
                } else {
			/* Send ForwardGet to my successor. Put the ip_addr and portnum received in that message */
			strcpy(sendbuf, "");
			sprintf(sendbuf, "GET ForwardGet %s\nIP:%s\nPortnum:%d\nRFC-value:%d\n", PROTOCOL_STR, ip_addr, portnum, rfc_value);
                        strcpy(msg_type, "ForwardGet");
                        send_message(peer_info.successor[0].ip_addr, peer_info.successor[0].portnum, msg_type, sendbuf);
			printf("ForwardGet Sent by Node %d to Node %d\n", peer_info.chord_id, peer_info.successor[0].chord_id);
			fflush(stdout);
		}

                /* Build GetRFC message which will originate from the initial server to the server which guarantees that RFC is there */
		//Populate variable 'rfc-value' somewhere
		
	} else if (strcmp(msg_type, "PeerDetails") == 0) {

		int i;
		char tmpbuf[1500];
		printf("In Get PeerDetails\n");
		sprintf(sendbuf,"POST PeerDetais %s\n",PROTOCOL_STR);
		for(i=0;i<MAX_NUM_OF_PEERS;i++)	{
			if (peer_list[i].chord_id != -1){
				sprintf(tmpbuf,"Chord_id:%d\nIP:%s\nPort:%d\n",peer_list[i].chord_id,peer_list[i].ip_addr,peer_list[i].portnum);
				printf("Current Peer %d in the list is: %s", i, tmpbuf);
				strcat(sendbuf,tmpbuf);
			}
		}
		printf("Message to be sent is %s",sendbuf);
		if ( send(client_sock, sendbuf, strlen(sendbuf), 0) < 0 ) {
			printf("Error in sending file data! \n");
			exit(-1);
		}
	} else if(strcmp(msg_type, "GetFinger") == 0){ //Response to the original server after the lookup forwarding

		int start;
                char *w;
                needle = strtok(msg, "\n");
                needle = strtok(NULL, "\n");
                q = strtok(NULL, "\n");
                w = strtok(NULL, "\n");

                p = strtok(needle, ":");
                p = strtok(NULL, ":");
                strcpy(ip_addr, p);

                r = strtok(q, ":");
                r = strtok(NULL, ":");
                portnum = atoi(r);

                s = strtok(w,":");
                s = strtok(NULL,":");
                fflush(stdout);
                start = atoi(s);

		printf("GetFinger received! %d %d %d\n", peer_info.chord_id, peer_info.successor[0].chord_id, start);
		if ( is_in_between(peer_info.chord_id, peer_info.successor[0].chord_id, start) ) {
                        /* My Successor has the RFC. Reply back to the requesting peer the chord id of my successor */
		        struct sockaddr_in fwd_sock_client;
		        int fwd_sock, fwd_slen = sizeof(fwd_sock_client), fwd_ret;
			char fwd_buf[BUFLEN] = "";
			sprintf(fwd_buf, "%d:%s:%d:", peer_info.successor[0].chord_id, peer_info.successor[0].ip_addr, peer_info.successor[0].portnum);
			printf("Sending %s\n", fwd_buf);

		        fwd_sock = socket(AF_INET, SOCK_STREAM, 0);
		        memset((char *) &fwd_sock_client, 0, sizeof(fwd_sock_client));

		        fwd_sock_client.sin_family = AF_INET;
		        fwd_sock_client.sin_port = htons(portnum);
		        fwd_sock_client.sin_addr.s_addr = inet_addr(ip_addr);

		        fwd_ret = connect(fwd_sock, (struct sockaddr *) &fwd_sock_client, fwd_slen);
		        if (fwd_ret == -1) {
                		printf("Connect failed! Check the IP and port number of the Sever! \n");
		                exit(-1);
		        }

		        if ( send(fwd_sock, fwd_buf, BUFLEN, 0) == -1 ) {
		                printf("send failed ");
		                exit(-1);
		        }

		        close(fwd_sock);
        
                } else {
			/* Send GetFinger to my successor. Put the ip_addr and portnum received in that message */
			strcpy(sendbuf, "");
			sprintf(sendbuf, "GET GetFinger %s\nIP:%s\nPortnum:%d\nRFC-value:%d\n", PROTOCOL_STR, ip_addr, portnum, start);
                        strcpy(msg_type, "GetFinger");
                        send_message(peer_info.successor[0].ip_addr, peer_info.successor[0].portnum, msg_type, sendbuf);
			printf("GetFinger Sent by Node %d to Node %d\n", peer_info.chord_id, peer_info.successor[0].chord_id);
			fflush(stdout);
		}

	} else if(strcmp(msg_type, "PeerExit") == 0) {
		RFC_db_rec *p;
		if(peer_info.chord_id != 0){
			if(peer_info.successor[0].chord_id == 0){
				printf("The successor of this node is P0. Exit for this node can't be triggered from client\n");
			}
			else {
				char listbuf[15000], tmpbuf[1500], sendlistbuf[15000];
				int count=0;
				strcpy(listbuf, "");
		
				if ( rfc_db_head ) { 		
					p = rfc_db_head;
					do {
						sprintf(tmpbuf, "%d:%d:%s\n", p->key, p->value, p->RFC_title);
						strcat(listbuf, tmpbuf);
						p = p->next;
						count += 1;
					} while ( p!= rfc_db_head ); 
				}
				new_head = rfc_db_head;
				fflush(stdout);
	
				//Send PutKey message to the successor 
                		sprintf(sendlistbuf, "POST PutKey %s\nIP:%s\nPortnum:%d\ncount:%d\n%s", PROTOCOL_STR, peer_info.ip_addr, peer_info.portnum, count, listbuf);
               			strcpy(msg_type, "PutKey");
	
				strcpy(ip_addr,peer_info.successor[0].ip_addr);
				portnum = peer_info.successor[0].portnum;
		
                		send_message(ip_addr, portnum, msg_type, sendlistbuf);
	
				printf("PutKey Message sent: to the successor %d by leaving node %d \n", peer_info.successor[0].chord_id, peer_info.chord_id);
			}	
		}
		else	printf("Peer0 can not Exit\n");
	}

}


void sort_RFC_db()
{
        
	int i, j, n=0;
	RFC_db_rec *p, *q, x;

	p = rfc_db_head;
	do {
		n += 1;
		p = p->next;
	} while ( p!=rfc_db_head );

	for( i=0, p=rfc_db_head ; i<n ;  p = p->next , ++i) { 
		for( j=0 , q = rfc_db_head  ; j < n-1 ;  q = q->next , ++j) { 
			if( q->key > (q->next)->key ) {

				x.key = q->key;
				x.value = q->value;
				strcpy(x.RFC_title, q->RFC_title);
	
				q->key = (q->next)->key;
				q->value = (q->next)->value;
				strcpy(q->RFC_title, (q->next)->RFC_title);
	
				(q->next)->key = x.key;
				(q->next)->value = x.value;
				strcpy((q->next)->RFC_title, x.RFC_title);
			}
		}
	}
 	
}

/* Is key in between prev and next*/
int is_in_between(int prev, int next, int key)
{
	if ( prev <= next ) {
		if ( prev <= key && key < next ) 
			return 1;
	}
	else {
		if ( (prev < key && key > next) || ( prev > key && key < next ) ) 
			return 1;
	}

	return 0;
}

void delete_node(RFC_db_rec *r)
{
	RFC_db_rec *tmp = rfc_db_head, *prev;

	prev = rfc_db_head;
	do {
		prev = prev->next;
	} while ( prev->next != rfc_db_head);

	if ( tmp == r ) {
		rfc_db_head = tmp->next;
		prev->next = rfc_db_head;
	}
	else {
		while ( tmp->next != r ) {
			tmp = tmp->next;		
		}
		tmp->next = r->next;
		free(r);
	}
}

RFC_db_rec * find_keys_to_transfer(int lower_bound, int upper_bound)
{

	RFC_db_rec *new_head=NULL, *tmp=rfc_db_head, *p, *q, *r;
	int n=0, i=0;

	if ( tmp == NULL )
		return NULL;

	do {
		if ( is_in_between(lower_bound, upper_bound, tmp->key) ) {
			p = (RFC_db_rec *)malloc(sizeof(RFC_db_rec)*1);
			p->key = tmp->key;
			p->value = tmp->value;
			strcpy(p->RFC_title, tmp->RFC_title);

       			if ( new_head == NULL) {
		                new_head = p;
                		q = p;
		        } else {
                		q->next=p;
		                q=q->next;
        		}
		        p->next = new_head;
			
			if (peer_info.chord_id != 0) {
				r = tmp;
				delete_node(r);
			}
		}
		tmp = tmp->next;
		i += 1;
	} while ( i<RFC_NUM_MAX );
	
	
	#ifdef DEBUG_FLAG
	printf("The RFCs to be transmitted --> \n");
	tmp = new_head;
	do {
		printf("%d %d %s\n", tmp->key, tmp->value, tmp->RFC_title);
		tmp = tmp->next;
	} while (tmp!= new_head);

	printf("The RFC db at this node \n");
	print_RFC_Database();
	#endif

	return new_head;
}

void server_listen()
{
	
//	pthread_t thread_id;
	
        struct sockaddr_in sock_client;
	int client, slen = sizeof(sock_client), ret;
	char msg_type[128], msg[BUFLEN], tmp[BUFLEN], *p;

	if ( peer_info.chord_id == 0 ) {
        	printf("Calling fix_fingers at P0... \n");
	        fix_fingers_P0(); /* This does not involve any */
        	print_finger_table(peer_info);
	}

	while (1) {

		if ((client = accept(well_known_socket, (struct sockaddr *) &sock_client, &slen)) == -1) {
			printf("accept call failed! \n");
			exit(-1);
		}

		ret = recv(client, msg, BUFLEN, 0);

		if ( ret == -1 ) {
			printf("Recv error! \n");
			exit(-1);
		} else if ( ret == 0) {
			close(client);
			continue;
		}

		pthread_t thread_id;
		printf("\nReceived following Msg:\n%s", msg);
		memcpy((char *)tmp, (char *)msg, strlen(msg));

		p = strtok(tmp, " ");
		p = strtok(NULL, " ");

		sprintf(msg_type, "%s", p);
//		printf("\n\n\n ");		
		fflush(stdout);

		/* The below func could possibly be in a pthread. */
	
		args.client=client;
		strcpy(args.msg_type,msg_type);
		strcpy(args.msg,msg);
	
		pthread_create(&thread_id,NULL,handle_messages,(void *)&args);
//		handle_messages(msg_type, msg, client);

		pthread_join(thread_id,NULL);
		close(client);
	}

}

int test_if_P0_alive(char ip_addr[128], int portnum)
{

	struct sockaddr_in sock_client;
	int sock, slen = sizeof(sock_client), ret;

        sock = socket(AF_INET, SOCK_STREAM, 0);
        memset((char *) &sock_client, 0, sizeof(sock_client));

        sock_client.sin_family = AF_INET;
        sock_client.sin_port = htons(portnum);
        sock_client.sin_addr.s_addr = inet_addr(ip_addr);

        ret = connect(sock, (struct sockaddr *) &sock_client, slen);
	if (ret == -1) {
		close(sock);
                return(-1);
	}
	else {
		close(sock);
		return(1);
	}

}

void send_message(char ip_addr[128], int portnum, char msg_type[128], char msg[BUFLEN])
{

        struct sockaddr_in sock_client;
        int sock, slen = sizeof(sock_client), ret;

        sock = socket(AF_INET, SOCK_STREAM, 0);
        memset((char *) &sock_client, 0, sizeof(sock_client));

        sock_client.sin_family = AF_INET;
        sock_client.sin_port = htons(portnum);
        sock_client.sin_addr.s_addr = inet_addr(ip_addr);

        ret = connect(sock, (struct sockaddr *) &sock_client, slen);
        if (ret == -1) {
                printf("Connect failed! Check the IP and port number of the Sever! \n");
                exit(-1);
        }

        if ( send(sock, msg, BUFLEN, 0) == -1 ) {
                printf("send failed ");
                exit(-1);
        }

	close(sock);

}

int sync_send_message(char ip_addr[128], int portnum, char msg_type[128], char msg[BUFLEN])
{

        struct sockaddr_in sock_client;
        int sock, slen = sizeof(sock_client), ret;

        sock = socket(AF_INET, SOCK_STREAM, 0);
        memset((char *) &sock_client, 0, sizeof(sock_client));

        sock_client.sin_family = AF_INET;
        sock_client.sin_port = htons(portnum);
        sock_client.sin_addr.s_addr = inet_addr(ip_addr);

        ret = connect(sock, (struct sockaddr *) &sock_client, slen);
        if (ret == -1) {
                printf("Connect failed! Check the IP and port number of the Sever! \n");
                exit(-1);
        }

        if ( send(sock, msg, BUFLEN, 0) == -1 ) {
                printf("send failed ");
                exit(-1);
        }

	if ( recv(sock, msg, 11, 0) == -1 ) {
		printf("recv failed ");
		exit(-1);
	}

	close(sock);

	return 0;

}
int check_if_exists(int fwd_msg[32], int k, int s)
{
	int i;
	for(i=0;i<k;i++) {
		if ( fwd_msg[i] == s ) 	
			return 0;
	}
	return 1;
}

peer_info_t setup_successor(int chord_id)
{

	peer_info_t t;
	int i, n, k=0, fwd_msg[32];

	for ( i=0;i<MAX_NUM_OF_PEERS;i++ ) {
		if ( peer_list[i].chord_id == -1 )
			break;
	}
	n = i-1;

	for ( i=0;i<MAX_NUM_OF_PEERS;i++) {
		fixfingersarr[i].chord_id = -1;
	}

	for (i=0 ; i<MAX_NUM_OF_PEERS ; i++) {
		if ( peer_list[i].chord_id == chord_id ) {
			if ( peer_list[(i+1) % MAX_NUM_OF_PEERS].chord_id != -1 ) {
				t.successor[0].chord_id = peer_list[(i+1)%MAX_NUM_OF_PEERS].chord_id;
				strcpy(t.successor[0].ip_addr, peer_list[(i+1)%MAX_NUM_OF_PEERS].ip_addr);
				t.successor[0].portnum = peer_list[(i+1)%MAX_NUM_OF_PEERS].portnum;

				peer_list[i].successor[0].chord_id = peer_list[(i+1)%MAX_NUM_OF_PEERS].chord_id;
				strcpy(peer_list[i].successor[0].ip_addr, peer_list[(i+1)%MAX_NUM_OF_PEERS].ip_addr);
				peer_list[i].successor[0].portnum = peer_list[(i+1)%MAX_NUM_OF_PEERS].portnum;

				//if (i!=0) {
					t.pred.chord_id = peer_list[(i-1)%MAX_NUM_OF_PEERS].chord_id;
					strcpy(t.pred.ip_addr, peer_list[(i-1)%MAX_NUM_OF_PEERS].ip_addr);
					t.pred.portnum = peer_list[(i-1)%MAX_NUM_OF_PEERS].portnum;

					peer_list[i].pred.chord_id = peer_list[(i-1)%MAX_NUM_OF_PEERS].chord_id;
					strcpy(peer_list[i].pred.ip_addr, peer_list[(i-1)%MAX_NUM_OF_PEERS].ip_addr);
					peer_list[i].pred.portnum = peer_list[(i-1)%MAX_NUM_OF_PEERS].portnum;
				//} 
				/*else {
					t.pred.chord_id = peer_list[n].chord_id;
					strcpy(t.pred.ip_addr, peer_list[n].ip_addr);
					t.pred.portnum = peer_list[n].portnum;

					peer_list[i].pred.chord_id = peer_list[n].chord_id;
					strcpy(peer_list[i].pred.ip_addr, peer_list[n].ip_addr);
					peer_list[i].pred.portnum = peer_list[n].portnum;
				}*/
				
				break;

			} else {
				t.successor[0].chord_id = peer_list[0].chord_id;
				strcpy(t.successor[0].ip_addr, peer_list[0].ip_addr);
				t.successor[0].portnum = peer_list[0].portnum;

				peer_list[i].successor[0].chord_id = peer_list[0].chord_id;
				strcpy(peer_list[i].successor[0].ip_addr, peer_list[0].ip_addr);
				peer_list[i].successor[0].portnum = peer_list[0].portnum;

				t.pred.chord_id = peer_list[i-1].chord_id;
				strcpy(t.pred.ip_addr, peer_list[i-1].ip_addr);
				t.pred.portnum = peer_list[i-1].portnum;

				peer_list[i].pred.chord_id = peer_list[i-1].chord_id;
				strcpy(peer_list[i].pred.ip_addr, peer_list[i-1].ip_addr);
				peer_list[i].pred.portnum = peer_list[i-1].portnum;

				break;
			}
		}
	}

	char sendbuf[BUFLEN], msg_type[128];

	for ( i=0 ; i< MAX_NUM_OF_PEERS ; i++ ) {

		if ( peer_list[i].chord_id != -1 ) {

			if ( peer_list[(i+1)%MAX_NUM_OF_PEERS].chord_id != -1 ) {

				if ( peer_list[i].successor[0].chord_id != peer_list[(i+1)%MAX_NUM_OF_PEERS].chord_id ) {
	
					peer_list[i].successor[0].chord_id = peer_list[(i+1)%MAX_NUM_OF_PEERS].chord_id;
					strcpy(peer_list[i].successor[0].ip_addr, peer_list[(i+1)%MAX_NUM_OF_PEERS].ip_addr);
					peer_list[i].successor[0].portnum = peer_list[(i+1)%MAX_NUM_OF_PEERS].portnum;

					if ( peer_list[i].chord_id == peer_info.chord_id ) {
						peer_info.successor[0].chord_id = peer_list[i].successor[0].chord_id;
						strcpy(peer_info.successor[0].ip_addr, peer_list[i].successor[0].ip_addr);
						peer_info.successor[0].portnum = peer_list[i].successor[0].portnum;
					} else {
						fwd_msg[k++] = i;
					}
				}

			} else {

				if ( peer_list[i].successor[0].chord_id != peer_list[0].chord_id ) {

					peer_list[i].successor[0].chord_id = peer_list[0].chord_id;
					strcpy(peer_list[i].successor[0].ip_addr, peer_list[0].ip_addr);
					peer_list[i].successor[0].portnum = peer_list[0].portnum;

					/* Send NodeIdentity msg to peer_list[i] */

					if ( peer_list[i].chord_id == peer_info.chord_id ) {
						peer_info.successor[0].chord_id = peer_list[i].successor[0].chord_id;
						strcpy(peer_info.successor[0].ip_addr, peer_list[i].successor[0].ip_addr);
						peer_info.successor[0].portnum = peer_list[i].successor[0].portnum;
					} else {
						fwd_msg[k++] = i;
					}

				}

			}
		}
	}

	for ( i=0 ; i< MAX_NUM_OF_PEERS ; i++ ) {
		if ( peer_list[i].chord_id != -1 ) {
			if ( i == 0 ) { //&& peer_list[i].pred.chord_id != peer_list[n].chord_id) 
				peer_list[i].pred.chord_id = peer_list[n].chord_id;
				strcpy(peer_list[i].pred.ip_addr, peer_list[n].ip_addr);
				peer_list[i].pred.portnum = peer_list[n].portnum;

				peer_info.pred.chord_id = peer_list[n].chord_id;
				strcpy(peer_info.pred.ip_addr, peer_list[n].ip_addr);
				peer_info.pred.portnum = peer_list[n].portnum;

			} else if ( peer_list[i].pred.chord_id != peer_list[i-1].chord_id ) {
				peer_list[i].pred.chord_id = peer_list[i-1].chord_id;
				strcpy(peer_list[i].pred.ip_addr, peer_list[i-1].ip_addr);
				peer_list[i].pred.portnum = peer_list[i-1].portnum;
				if ( check_if_exists(fwd_msg, k, i) ) 
					fwd_msg[k++] = i;
			}
			
		}
	}

	for (n=0;n<k;n++) {
		i = fwd_msg[n];
	
		fixfingersarr[n].chord_id = peer_list[i].chord_id;
		strcpy(fixfingersarr[n].ip_addr, peer_list[i].ip_addr);
		fixfingersarr[n].portnum = peer_list[i].portnum;

		if ( peer_list[i].chord_id != 0 ) {
			printf("1) Send NodeIdentiy msg to Node %d\n", peer_list[i].chord_id);
        	        sprintf(sendbuf, "POST NodeIdentity %s\nchord_id:%d\nsuccessor_id:%d\nsuccessor_IP:%s\nsuccessor_Port:%d\npred.chord_id:%d\npred_IP:%s\npred_Port:%d\n", PROTOCOL_STR, peer_list[i].chord_id, peer_list[i].successor[0].chord_id, peer_list[i].successor[0].ip_addr, peer_list[i].successor[0].portnum, peer_list[i].pred.chord_id, peer_list[i].pred.ip_addr, peer_list[i].pred.portnum);
                	strcpy(msg_type, "NodeIdentity");
	                send_message(peer_list[i].ip_addr, peer_list[i].portnum, msg_type, sendbuf);
		}
	}


	return t;

}


