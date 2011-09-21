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

int flag_getkey = 0;

void print_details(peer_info_t p_info)
{
	printf("Chord_Id:%d IP:%s Port:%d Successor:%d %s %d Pred:%d %s %d\n", p_info.chord_id, p_info.ip_addr, p_info.portnum, p_info.successor_id, p_info.successor_ip_addr, p_info.successor_portnum, p_info.pred_id, p_info.pred_ip_addr, p_info.pred_portnum);
	fflush(stdout);
}

void print_RFC_Database ()
{
	RFC_db_rec *p;
	int c=0;
	p = rfc_db_head;
	printf("\nThe RFC Database: \n");
	while(p!=NULL)
	{
		printf("Key : %d Value:%d  Title:%s\n",p->key, p->value,p->RFC_title);
		p=p->next;
		c++;
	}
//	printf("COUNT: %d\n",c);
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

void handle_messages(char msg_type[128], char msg[BUFLEN], int client_sock)
{
		
	int bytes_read, portnum, chord_id, successor_id;
	char sendbuf[BUFLEN], filename[128] = RFC_PATH, *needle, ip_addr[128],buf[BUFLEN],msg_1[BUFLEN],msg_type_1[128]; 
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

		send(client_sock, "FILEEND", 7, 0);
		
		fclose(fp);

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

		/*
		//Build GetKey to be sent to Successor of current peer_info
		if(flag_getkey == 0){
		 	build_GetKey_msg(msg_1);
	                strcpy(msg_type_1, "GetKey");
	                strcpy(buf, msg_1);
	                send_message(peer_info.successor_ip_addr, peer_info.successor_portnum, msg_type_1, buf);
	                printf("%s Message sent from Peer with ChordID %d to its successor with ChordID %d\n", msg_type_1, peer_info.chord_id,peer_info.successor_id);
			flag_getkey = 1;
		}
		

		if ( peer_info.chord_id == 0 ) {
			printf("\n\n The P2P System Details \n\n");
			print_peer_infos();
			printf("\n\n");
		}
		*/

	}
	else if(strcmp(msg_type,"GetKey") == 0){
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
		
		//Now.. find the appropriate keys to be sent to node with ip_addr and portnum
		RFC_db_rec *new_rfc,*p;
	
			
		
		new_rfc = find_keys_to_transfer(peer_info.chord_id,peer_info.pred_id);
		if(new_rfc==NULL)
			printf("*****NULL\n");
		if(new_rfc!=NULL){
			printf("NEW LIST HEAD key: %d\n",new_rfc -> key);
			char lists[15000],rec[200];
			int lc = 0;
			p = NULL;
			lists[0] = '\0';
			for(p = new_rfc ; p!=NULL ; p = p -> next)
			{
				sprintf(rec, "%d,%d,%s\n",p -> key, p -> value, p -> RFC_title);	
				strcat(lists,rec);
				rec[0] = '\0';
				lc ++; // LC has the nodeList count.. used in else if of NodeList to parse the message
			}
			
			printf("Remaining RFC List at this peer : \n");
			print_RFC_Database();
			//Build NodeList message
	                sprintf(sendbuf, "POST NodeList %s\nchord_id:%d\ncount:%d\n|%s", PROTOCOL_STR, chord_id, lc, lists);
			lc=0;
			lists[0] = '\0';
                	strcpy(msg_type, "NodeList");
	                send_message(ip_addr, portnum, msg_type, sendbuf);
		}
		else{ //no sublist found! .. return error message
			 strcpy(msg_type, "NodeList");
			printf("NOTHING TO SEND********\n");
			 sprintf(sendbuf,"POST NodeList Error!\n");
			 send_message(ip_addr, portnum, msg_type, sendbuf);
		}
 		
	}
	else if(strcmp(msg_type, "NodeList")==0){
	    needle = strtok(msg, "\n");
	    

	   if(strcmp(needle, "POST NodeList Error!")!=0) // no nodelists
	   {
//                needle = strtok(msg, "\n");
		

                needle = strtok(NULL, "\n");
                q = strtok(NULL, "\n");
		p = strtok(q, ":");
		s = strtok(NULL, "\n");
		int cnt = atoi(s);
		int i;
		printf("CNT :: %d\n",cnt);
		

		for(i = 0; msg[i]!='|';i++);
		char key[25],value[25],title[100];
		int kk=0,vv=0,tt=0,limit = 0;
		RFC_db_rec *head,*p,*q;
		

			
		for(i = i+1;msg[i]!='\0' && (limit < cnt);){
			
			while(msg[i]!=',')	
				key[kk++] = msg[i++];
			key[kk] = '\0';
			kk = 0; //key[kk] = '\0';
			i++;

			while(msg[i]!=',')
				value[vv++] = msg[i++];
                        value[vv] = '\0';
			vv = 0; //value[vv] = '\0';
 			i++;

			
			while(msg[i]!='\n')
				title[tt++] = msg[i++];
                        title[tt] = '\0';
			tt = 0; //title[tt] = '\0';
			i++;
	               
 						
		
			p = (RFC_db_rec *)malloc(sizeof(RFC_db_rec));
 	                if (!p) {
                        printf("Error while allocating memory!!\n");
                        exit(-1);
                        }
 			
		
			p -> next = NULL;	
			p -> key = atoi(key);
			p -> value = atoi(value);
			
			strcpy(p -> RFC_title, title);
			sprintf(p->RFC_body, "ID:%d Value:%d \n<<RFC Text Goes Here>>", p->key, p->value);
		
	                if ( rfc_db_head == NULL) {
	                        rfc_db_head = p;
	                        q = p;
	                } else {
	                        q->next=p;
	                        q=q->next;
	                }

			
			limit++;
		}
		//head = NULL;
	
	        sort_RFC_db();

                #ifdef DEBUG_FLAG
                printf("-------------Sorted: \n");
                print_RFC_Database();
                #endif
	   }
		
		
	}

}


void sort_RFC_db()
{
        int i,j,n=0;
        RFC_db_rec *head,*lst,*prev,*pprev = rfc_db_head,*temp = rfc_db_head;
        
        for(;temp;temp=temp->next){
                n++;
        }
        
        for(i = 0; i < n-1; i++)
        {
                for(j=0,lst = rfc_db_head ; lst && lst->next && (j <= n-i-1);j++)
                {
                        if(!j)
                                prev = lst;
                        if(lst -> key > lst -> next -> key)
                        {
                                temp = (lst -> next? lst -> next -> next:0);
                                if(!j && (prev == rfc_db_head))
                                        rfc_db_head = lst -> next;
                                pprev = lst -> next;
                                prev -> next = lst -> next;
                                lst -> next -> next = lst;
                                lst -> next = temp;
                                prev = pprev;
                        }
                        else
                        {
                                lst = lst -> next;
                                if(j)
                                        prev = prev -> next;
                        }
                }
        }
//        return list;
}





RFC_db_rec * find_keys_to_transfer(int lower_bound, int upper_bound)
{
	int i,flag=0;
	RFC_db_rec *seek,*new,*new1,*prev,*ahead,*new2;
	seek = rfc_db_head;
	new = NULL;


		
	if(lower_bound > upper_bound)
		lower_bound = 0;

	printf("RFC LIST HEAD IS AT: %d \n",rfc_db_head->key);	
	if((upper_bound - lower_bound) < rfc_db_head -> key) //no NodeList can be foung
	{
		printf("HERE.. returning NULL \n");
		return NULL;
	}

	printf("------------Lower Bound: %d \t Upper Bound: %d\n",lower_bound,upper_bound);	
	seek = rfc_db_head;
	while(seek -> key <= lower_bound){
		flag = 1;
		ahead = seek;
		seek = seek -> next;
	}
	

	if(flag ==1)	{
//		if(ahead!=NULL)
		prev = ahead; //prev points to the last node that is <= lower_bound (it will point to the 1st entry which is greater than upper_bound)

	}
//	if(seek!=NULL){
		new = seek; //new points to 1st entry greater than the lowerbound
		//printf("AFTER LB: %d \n",new -> key);
		new1 = new;
	
		new2 = new1;
		while(new1 -> key <= upper_bound){
			ahead = new1;
			new1 = new1 -> next;
		}
	
		if(flag ==1)
			prev -> next = new1;
		ahead -> next = NULL;
		if(flag == 0)
			rfc_db_head = new1;
		
//	}
//	else
//		return NULL;
	return new2;
      
}

void  build_GetKey_msg(char *msg)
{
        sprintf(msg, "GET GetKey %s\nIP:%s\nPort:%d\n", PROTOCOL_STR, peer_info.ip_addr, peer_info.portnum);

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

				if (i!=0) {
					t.pred_id = peer_infos[(i-1)%MAX_NUM_OF_PEERS].chord_id;
					strcpy(t.pred_ip_addr, peer_infos[(i-1)%MAX_NUM_OF_PEERS].ip_addr);
					t.pred_portnum = peer_infos[(i-1)%MAX_NUM_OF_PEERS].portnum;

					peer_infos[i].pred_id = peer_infos[(i-1)%MAX_NUM_OF_PEERS].chord_id;
					strcpy(peer_infos[i].pred_ip_addr, peer_infos[(i-1)%MAX_NUM_OF_PEERS].ip_addr);
					peer_infos[i].pred_portnum = peer_infos[(i-1)%MAX_NUM_OF_PEERS].portnum;
				} else {
					t.pred_id = peer_infos[n].chord_id;
					strcpy(t.pred_ip_addr, peer_infos[n].ip_addr);
					t.pred_portnum = peer_infos[n].portnum;

					peer_infos[i].pred_id = peer_infos[n].chord_id;
					strcpy(peer_infos[i].pred_ip_addr, peer_infos[n].ip_addr);
					peer_infos[i].pred_portnum = peer_infos[n].portnum;
				}
				
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
			if ( i == 0 && peer_infos[i].pred_id != peer_infos[n].chord_id) {
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


