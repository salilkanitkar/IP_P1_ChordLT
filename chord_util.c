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

#define BUFLEN 1500

void print_details(peer_info_t p_info)
{
	printf("Chord_Id:%d IP:%s Port:%d Successor:%d\n", p_info.chord_id, p_info.ip_addr, p_info.portnum, p_info.successor_id);
}

void print_RFC_Database ()
{
	RFC_db_rec *p;
	p = rfc_db_head;
	printf("\nThe RFC Database: \n");
	while(p!=NULL)
	{
		printf("Key : %d Value:%d  Title:%s\n",p->key, p->value,p->RFC_title);
		p=p->next;
	}
	printf("\n");
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
	for(i=0;i<10;i++)
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
	for(i=0;i<10;i++)
	{
		if(peer_infos[i].chord_id == -1)
			return i;
	}
	return -1;
}


void initialize_peer_infos()
{
	int i;
	for(i=0;i<10;i++)
	{
		peer_infos[i].chord_id = -1;
		peer_infos[i].successor_id = -1;
		peer_infos[i].portnum = -1;
		strcpy(peer_infos[i].iface_name,"\0");
		strcpy(peer_infos[i].ip_addr,"\0");
	}
}

void put_in_peer_infos(int chord_id, char ip_addr[128], int portnum)
{
	int next_free = next_free_position();
	peer_infos[next_free].chord_id = chord_id;
	strcpy(peer_infos[next_free].ip_addr, ip_addr);
	peer_infos[next_free].portnum = portnum;
}

void print_peer_infos()
{
	int i;
	for(i=0;i<10;i++)
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
	char sendbuf[BUFLEN], filename[128] = RFC_PATH, *needle, *p, *q, *r, ip_addr[128];
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
		printf("%s\n", r);
		portnum = atoi(r);
	
		printf("IP:%s Port:%d\n", ip_addr, portnum);
		fflush(stdout);

		chord_id = generate_ChordID(1, 1023);
		put_in_peer_infos(chord_id, ip_addr, portnum);

		printf("\n\n The P2P System Details \n\n");
		print_peer_infos();
		printf("\n\n");

		successor_id = find_successor();

		sprintf(sendbuf, "POST NodeIdentity %s\nchord_id:%d\nsuccessor_id:%d\n", PROTOCOL_STR, chord_id, successor_id);
		strcpy(msg_type, "NodeIdentity");
		send_message(ip_addr, portnum, msg_type, sendbuf);

	} else if (strcmp(msg_type, "NodeIdentity") == 0) {

		needle = strtok(msg, "\n");
		needle = strtok(NULL, "\n");
		q = strtok(NULL, "\n");

		p = strtok(needle, ":");
		p = strtok(NULL, ":");
		peer_info.chord_id = atoi(p);

		r = strtok(q, ":");
		r = strtok(NULL, ":");
		peer_info.successor_id = atoi(r);

		printf("Received the NodeIdentity Message from P0\n");
		print_details(peer_info);
		printf("\n\n");

	}
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

int find_successor()
{
	return 0;
}
