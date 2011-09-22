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

void print_details(peer_info_t p_info)
{
	printf("Chord_Id:%d IP:%s Port:%d Successor:%d %s %d Pred:%d %s %d\n", p_info.chord_id, p_info.ip_addr, p_info.portnum, p_info.successor_id, p_info.successor_ip_addr, p_info.successor_portnum, p_info.pred_id, p_info.pred_ip_addr, p_info.pred_portnum);
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

	fp = popen("/bin/ls ./sample_RFCs/", "r");
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
		if(peer_infos[i].chord_id==random)
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
		if(peer_infos[i].chord_id == -1)
			return i;
	}
	return -1;
}


void initialize_peer_infos()
{
	int i;
	for(i=0;i<MAX_NUM_OF_PEERS;i++)
	{
		peer_infos[i].chord_id = -1;
		peer_infos[i].successor_id = -1;
		peer_infos[i].pred_id = -1;
		peer_infos[i].portnum = -1;
		strcpy(peer_infos[i].iface_name,"\0");
		strcpy(peer_infos[i].ip_addr,"\0");
		strcpy(peer_infos[i].successor_ip_addr,"\0");
		strcpy(peer_infos[i].pred_ip_addr,"\0");
		peer_infos[i].successor_portnum = -1;
		peer_infos[i].pred_portnum = -1;
	}
}

void put_in_peer_infos(int chord_id, char ip_addr[128], int portnum)
{
	int i, j, next_free;

	for ( i=0 ; i<MAX_NUM_OF_PEERS ; i++ ) {
		if ( peer_infos[i].chord_id > chord_id ) {
			next_free = i;
			j = i;
			while ( peer_infos[j].chord_id != -1 )
				j += 1;
			while ( j > i) {
				peer_infos[j].chord_id = peer_infos[j-1].chord_id;
				strcpy(peer_infos[j].ip_addr, peer_infos[j-1].ip_addr);
				peer_infos[j].portnum = peer_infos[j-1].portnum;
				strcpy(peer_infos[j].iface_name, peer_infos[j-1].iface_name);
				peer_infos[j].successor_id = peer_infos[j-1].successor_id;
				strcpy(peer_infos[j].successor_ip_addr, peer_infos[j-1].successor_ip_addr);
				peer_infos[j].successor_portnum = peer_infos[j-1].successor_portnum;
				peer_infos[j].pred_id = peer_infos[j-1].pred_id;
				strcpy(peer_infos[j].pred_ip_addr, peer_infos[j-1].pred_ip_addr);
				peer_infos[j].pred_portnum = peer_infos[j-1].pred_portnum;
			j -= 1;
			}
			break;	
		} else if ( peer_infos[i].chord_id == -1 ) {
			next_free = i;
			break;
		}
	}

	peer_infos[next_free].chord_id = chord_id;
	strcpy(peer_infos[next_free].ip_addr, ip_addr);
	peer_infos[next_free].portnum = portnum;
}

void print_peer_infos()
{
	int i;
	for(i=0;i<MAX_NUM_OF_PEERS;i++)
	{
		if (peer_infos[i].chord_id != -1)
			print_details(peer_infos[i]);
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

void generate_RFC_Database (int start, int end, char RFC_Dir[][RFC_TITLE_LEN_MAX])
{
#ifdef DEBUG_FLAG
printf("Enter generate_RFC_database \n");
#endif

	long int seed;
	int rndm;
	struct timeval ct;
	int i;
	RFC_db_rec *p, *q;
  
	gettimeofday(&ct, NULL);
        seed = (ct.tv_sec +ct.tv_usec);
        srand(seed);

	for(i=0; i<50; i++){

		/* TODO: Make sure that same random number is not generated again. */
		rndm = generate_random_number(start, end);
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
	char buf[64];

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
				sprintf(peer_info.ip_addr, "%s", buf);
				sprintf(peer_info.iface_name, "%s", ifa->ifa_name);
			}
		}
	}

	freeifaddrs(myaddrs);

	#ifdef DEBUG_FLAG
	printf("My public interface and IP is:  %s %s\n", peer_info.iface_name, peer_info.ip_addr);
	#endif

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
                sock_server.sin_port = htons(portnum);
        }

        if (listen(well_known_socket, 10) == -1) {
                printf("listen error");
                exit(1);
        }
	
	peer_info.portnum = portnum;
}

int get_rfc(char title[128], char ip_addr[128], int portnum)
{
	//	#ifdef DEBUG_FLAG
			printf("Ekde!! IP addr: %s port %d\n",ip_addr,portnum);
	//	#endif
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
			sprintf(sendbuf, "GET FetchRFC %s %s\nIP:%s\nPort:%d\n", title, PROTOCOL_STR, ip_addr, portnum);
		
		//#ifdef DEBUG_FLAG
			printf("sendbuf:\n%s\n", sendbuf);
	//	#endif

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

void handle_messages(char msg_type[128], char msg[BUFLEN], int client_sock)
{
		
	int bytes_read, portnum, chord_id, successor_id;
	char sendbuf[BUFLEN], filename[128] = RFC_PATH, *needle, ip_addr[128]; 
	char *p, *q, *r, *s, *t, *u, *v, *x, *xx, *y, *yy, *z, *zz;
	FILE *fp;

	if (strcmp(msg_type, "FetchRFC") == 0) {

		needle = strtok(msg, " ");
		needle = strtok(NULL, " ");
		needle = strtok(NULL, " ");

		printf("\nThe needle is: %s\n", needle);
		strcat(filename, needle);
	
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
		put_in_peer_infos(chord_id, ip_addr, portnum);

		peer_info_t t ;
		t = setup_successor(chord_id);

		printf("\n\nThe P2P System Details \n\n");
		print_peer_infos();
		printf("\n\n");
		print_details(peer_info);
		printf("\n\nSENT NODEIDENTITY\n");

		sprintf(sendbuf, "POST NodeIdentity %s\nchord_id:%d\nsuccessor_id:%d\nsuccessor_IP:%s\nsuccessor_Port:%d\npred_id:%d\npred_IP:%s\npred_Port:%d\n", PROTOCOL_STR, chord_id, t.successor_id, t.successor_ip_addr, t.successor_portnum, t.pred_id, t.pred_ip_addr, t.pred_portnum);
		strcpy(msg_type, "NodeIdentity");
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
		peer_info.successor_id = atoi(r);

		u = strtok(s, ":");
		u = strtok(NULL, ":");
		strcpy(peer_info.successor_ip_addr, u);

		v = strtok(t, ":");
		v = strtok(NULL, ":");
		peer_info.successor_portnum = atoi(v);

		xx = strtok(x, ":");
		xx = strtok(NULL, ":");
		peer_info.pred_id = atoi(xx);

		yy = strtok(y, ":");
		yy = strtok(NULL, ":");
		strcpy(peer_info.pred_ip_addr, yy);

		zz = strtok(z, ":");
		zz = strtok(NULL, ":");
		peer_info.pred_portnum = atoi(zz);

		printf("\n\nReceived the NodeIdentity Message from P0\n");
		print_details(peer_info);
		printf("\n\n");

		if ( !db_init ) {
			/* Build GetKey to be sent to Successor of current peer_info */
			sprintf(msg, "GET GetKey %s\nIP:%s\nPort:%d\nChord-Id:%d\n", PROTOCOL_STR, peer_info.ip_addr, peer_info.portnum, peer_info.chord_id);
	        	strcpy(msg_type, "GetKey");
		        strcpy(sendbuf, msg);
		        send_message(peer_info.successor_ip_addr, peer_info.successor_portnum, msg_type, sendbuf);
	        	printf("%s Message sent from Peer with ChordID %d to its successor with ChordID %d\n", msg_type, peer_info.chord_id, peer_info.successor_id);
			db_init = 1;
		}

		if ( peer_info.chord_id == 0 ) {
			printf("\n\n The P2P System Details \n\n");
			print_peer_infos();
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
			if ( peer_infos[i].chord_id != -1 ) {
				if (peer_infos[i].chord_id == chord_id)
					k=i;
			} else
				break;
		}
		
		if ( k == 0 ) {
			new_rfc = find_keys_to_transfer(peer_infos[i-1].chord_id, chord_id);
		}
		else
			new_rfc = find_keys_to_transfer(peer_infos[k-1].chord_id, chord_id);

		#ifdef DEBUG_FLAG
		p = new_rfc;
	        do {
                	printf("%d %d %s\n", p->key, p->value, p->RFC_title);
        	        p = p->next;
	        } while (p != new_rfc);
		#endif

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

			ret = get_rfc(node_p->RFC_title, ip_addr, portnum);

		#ifdef DEBUG_FLAG
			printf("Starting Next Fetch\n");
		#endif

			node_p = node_p->next;


		} while (node_p != rfc_db_head);

		printf("RFC Db LL created! \n");

	} else if ( strcmp(msg_type, "PrintRFCDb")==0 ) {

	        printf("The RFC db at this node \n");
	        print_RFC_Database();

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
	} while ( i<50 );
	
	
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

        struct sockaddr_in sock_client;
	int client, slen = sizeof(sock_client), ret;
	char msg_type[128], msg[BUFLEN], tmp[BUFLEN], *p;

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
			goto close;
		}

		printf("\nReceived following Msg:\n%s", msg);
		memcpy((char *)tmp, (char *)msg, strlen(msg));

		p = strtok(tmp, " ");
		p = strtok(NULL, " ");

		sprintf(msg_type, "%s", p);
//		printf("\n\n\n ");		
		fflush(stdout);

		/* The below func could possibly be in a pthread. */
		handle_messages(msg_type, msg, client);

close:
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
		if ( peer_infos[i].chord_id == -1 )
			break;
	}
	n = i-1;

	for (i=0 ; i<MAX_NUM_OF_PEERS ; i++) {
		if ( peer_infos[i].chord_id == chord_id ) {
			if ( peer_infos[(i+1) % MAX_NUM_OF_PEERS].chord_id != -1 ) {
				t.successor_id = peer_infos[(i+1)%MAX_NUM_OF_PEERS].chord_id;
				strcpy(t.successor_ip_addr, peer_infos[(i+1)%MAX_NUM_OF_PEERS].ip_addr);
				t.successor_portnum = peer_infos[(i+1)%MAX_NUM_OF_PEERS].portnum;

				peer_infos[i].successor_id = peer_infos[(i+1)%MAX_NUM_OF_PEERS].chord_id;
				strcpy(peer_infos[i].successor_ip_addr, peer_infos[(i+1)%MAX_NUM_OF_PEERS].ip_addr);
				peer_infos[i].successor_portnum = peer_infos[(i+1)%MAX_NUM_OF_PEERS].portnum;

				//if (i!=0) {
					t.pred_id = peer_infos[(i-1)%MAX_NUM_OF_PEERS].chord_id;
					strcpy(t.pred_ip_addr, peer_infos[(i-1)%MAX_NUM_OF_PEERS].ip_addr);
					t.pred_portnum = peer_infos[(i-1)%MAX_NUM_OF_PEERS].portnum;

					peer_infos[i].pred_id = peer_infos[(i-1)%MAX_NUM_OF_PEERS].chord_id;
					strcpy(peer_infos[i].pred_ip_addr, peer_infos[(i-1)%MAX_NUM_OF_PEERS].ip_addr);
					peer_infos[i].pred_portnum = peer_infos[(i-1)%MAX_NUM_OF_PEERS].portnum;
				//} 
				/*else {
					t.pred_id = peer_infos[n].chord_id;
					strcpy(t.pred_ip_addr, peer_infos[n].ip_addr);
					t.pred_portnum = peer_infos[n].portnum;

					peer_infos[i].pred_id = peer_infos[n].chord_id;
					strcpy(peer_infos[i].pred_ip_addr, peer_infos[n].ip_addr);
					peer_infos[i].pred_portnum = peer_infos[n].portnum;
				}*/
				
				break;

			} else {
				t.successor_id = peer_infos[0].chord_id;
				strcpy(t.successor_ip_addr, peer_infos[0].ip_addr);
				t.successor_portnum = peer_infos[0].portnum;

				peer_infos[i].successor_id = peer_infos[0].chord_id;
				strcpy(peer_infos[i].successor_ip_addr, peer_infos[0].ip_addr);
				peer_infos[i].successor_portnum = peer_infos[0].portnum;

				t.pred_id = peer_infos[i-1].chord_id;
				strcpy(t.pred_ip_addr, peer_infos[i-1].ip_addr);
				t.pred_portnum = peer_infos[i-1].portnum;

				peer_infos[i].pred_id = peer_infos[i-1].chord_id;
				strcpy(peer_infos[i].pred_ip_addr, peer_infos[i-1].ip_addr);
				peer_infos[i].pred_portnum = peer_infos[i-1].portnum;

				break;
			}
		}
	}

	char sendbuf[BUFLEN], msg_type[128];

	for ( i=0 ; i< MAX_NUM_OF_PEERS ; i++ ) {

		if ( peer_infos[i].chord_id != -1 ) {

			if ( peer_infos[(i+1)%MAX_NUM_OF_PEERS].chord_id != -1 ) {

				if ( peer_infos[i].successor_id != peer_infos[(i+1)%MAX_NUM_OF_PEERS].chord_id ) {
	
					peer_infos[i].successor_id = peer_infos[(i+1)%MAX_NUM_OF_PEERS].chord_id;
					strcpy(peer_infos[i].successor_ip_addr, peer_infos[(i+1)%MAX_NUM_OF_PEERS].ip_addr);
					peer_infos[i].successor_portnum = peer_infos[(i+1)%MAX_NUM_OF_PEERS].portnum;

					if ( peer_infos[i].chord_id == peer_info.chord_id ) {
						peer_info.successor_id = peer_infos[i].successor_id;
						strcpy(peer_info.successor_ip_addr, peer_infos[i].successor_ip_addr);
						peer_info.successor_portnum = peer_infos[i].successor_portnum;
					} else {
						fwd_msg[k++] = i;
					}
				}

			} else {

				if ( peer_infos[i].successor_id != peer_infos[0].chord_id ) {

					peer_infos[i].successor_id = peer_infos[0].chord_id;
					strcpy(peer_infos[i].successor_ip_addr, peer_infos[0].ip_addr);
					peer_infos[i].successor_portnum = peer_infos[0].portnum;

					/* Send NodeIdentity msg to peer_infos[i] */

					if ( peer_infos[i].chord_id == peer_info.chord_id ) {
						peer_info.successor_id = peer_infos[i].successor_id;
						strcpy(peer_info.successor_ip_addr, peer_infos[i].successor_ip_addr);
						peer_info.successor_portnum = peer_infos[i].successor_portnum;
					} else {
						fwd_msg[k++] = i;
					}

				}

			}
		}
	}

	for ( i=0 ; i< MAX_NUM_OF_PEERS ; i++ ) {
		if ( peer_infos[i].chord_id != -1 ) {
			if ( i == 0 ) { //&& peer_infos[i].pred_id != peer_infos[n].chord_id) 
				peer_infos[i].pred_id = peer_infos[n].chord_id;
				strcpy(peer_infos[i].pred_ip_addr, peer_infos[n].ip_addr);
				peer_infos[i].pred_portnum = peer_infos[n].portnum;

				peer_info.pred_id = peer_infos[n].chord_id;
				strcpy(peer_info.pred_ip_addr, peer_infos[n].ip_addr);
				peer_info.pred_portnum = peer_infos[n].portnum;

			} else if ( peer_infos[i].pred_id != peer_infos[i-1].chord_id ) {
				peer_infos[i].pred_id = peer_infos[i-1].chord_id;
				strcpy(peer_infos[i].pred_ip_addr, peer_infos[i-1].ip_addr);
				peer_infos[i].pred_portnum = peer_infos[i-1].portnum;
				if ( check_if_exists(fwd_msg, k, i) ) 
					fwd_msg[k++] = i;
			}
			
		}
	}


	for (n=0;n<k;n++) {
		i = fwd_msg[n];
		if ( peer_infos[i].chord_id != 0 ) {
			printf("1) Send NodeIdentiy msg to Node %d\n", peer_infos[i].chord_id);
        	        sprintf(sendbuf, "POST NodeIdentity %s\nchord_id:%d\nsuccessor_id:%d\nsuccessor_IP:%s\nsuccessor_Port:%d\npred_id:%d\npred_IP:%s\npred_Port:%d\n", PROTOCOL_STR, peer_infos[i].chord_id, peer_infos[i].successor_id, peer_infos[i].successor_ip_addr, peer_infos[i].successor_portnum, peer_infos[i].pred_id, peer_infos[i].pred_ip_addr, peer_infos[i].pred_portnum);
                	strcpy(msg_type, "NodeIdentity");
	                send_message(peer_infos[i].ip_addr, peer_infos[i].portnum, msg_type, sendbuf);
		}
	}

	return t;

}


