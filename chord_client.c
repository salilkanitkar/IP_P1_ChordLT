#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>

#include <sys/time.h>

#include"chord_client.h"
char PROTOCOL_STR[128] = "Chord-LT/1.0";

char FETCHMSG_STR[128] = "FetchRFC";
char PRINTRFCDBMSG_STR[128] = "PrintRFCDb";
char PEERDETAILSMSG_STR[128] = "PeerDetails";
char PEEREXIT_STR[128] = "PeerExit";
char buf[BUFLEN];
peer_track_t peer_list[10];

void build_FetchRFC_msg(char *ip_addr, int portnum, char *rfc_title, int rfc_value) 
{
	sprintf(buf, "GET %s %s %d %s\nIP:%s\nPort:%d\n", FETCHMSG_STR, rfc_title, rfc_value, PROTOCOL_STR, ip_addr, portnum);
}
void build_PeerDetails_msg(char *ip_addr, int portnum) 
{
	sprintf(buf, "GET %s %s\nIP:%s\nPort:%d\n", PEERDETAILSMSG_STR, PROTOCOL_STR, ip_addr, portnum);
}
void build_PeerExit_msg(char *ip_addr, int portnum) 
{
	sprintf(buf, "GET %s %s\nIP:%s\nPort:%d\n", PEEREXIT_STR, PROTOCOL_STR, ip_addr, portnum);
}

void set_peer_list(char* msg)
{
	int n,i=0;
	char *state, *p, *q, *r, *s;
       	strtok_r(msg,"\n",&state); 
	p = strtok_r(NULL, "\n",&state);
	while(p!=NULL) {
 	       	q = strtok_r(NULL, "\n",&state);
        	r = strtok_r(NULL, "\n",&state);

	        s = strtok(p, ":");
        	s = strtok(NULL, ":");
		n = atoi(s);
        	peer_list[i].chord_id = n;

		s = strtok(q, ":");
	        s = strtok(NULL, ":");
		strcpy(peer_list[i].ip_addr,s);

	        s = strtok(r, ":");
        	s = strtok(NULL, ":");
	        n = atoi(s);
        	peer_list[i].portnum = n;
		
		i++;
		p = strtok_r(NULL,"\n",&state);
	}
}

init_peer_list()
{
	int i;
	for(i=0;i<10;i++) {
		peer_list[i].portnum = -1;
		peer_list[i].chord_id = -1;
		strcpy(peer_list[i].ip_addr,"\0");
	}
}

print_peer_list()
{
	int i;
	printf("The current list of Peers is:\n");
	for(i=0;i<10;i++) {
		if(peer_list[i].chord_id != -1) {
			printf("Chord id: %d, IP Address:%s, Port:%d\n",peer_list[i].chord_id,peer_list[i].ip_addr,peer_list[i].portnum);
		}
	}
}

int main()
{

        struct sockaddr_in sock_client;
        int sock, slen = sizeof(sock_client), ret;

	char ip_addr[128], rfc_title[128], msg_type[128];
	int portnum, rfc_value;

	char recvbuf[BUFLEN];
	int bytes_read;

	struct timeval p,q;
	
	printf("\n=================== Chord Client ======================= \n");

	while (1) {
		printf("\nAvailable Tpes of Messages\nFetchRFC, PrintRFCDb, PeerDetails, PeerExit\n\n");
		printf("Enter \"End\" to Exit from Chord Client\n\n");

		printf("Enter the Type of the Message: ");
		scanf("%s", msg_type);

		if ( strcmp(msg_type, "End") == 0 ) {
			printf("Chord Cient Exiting ... \n");
			exit(0);
		}

		if ( strcmp(msg_type,"FetchRFC") == 0 ) {

			printf("Enter the IP Address of the Peer to connect to: ");
			scanf("%s", ip_addr);

			printf("Enter the Port of the above Peer to connect to: ");
			scanf("%d", &portnum);

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

			printf("Enter the RFC Title of the RFC to be fetched: ");
			scanf("%s", rfc_title);

			printf("Enter the RFC value of the RFC to be fetched: ");
			scanf("%d", &rfc_value);
			
			build_FetchRFC_msg(ip_addr, portnum, rfc_title, rfc_value);
			printf("FetchRFC Msg:\n%s", buf);

	                gettimeofday(&p, NULL);

	        	if ( send(sock, buf, BUFLEN, 0) == -1 ) {
        	        	printf("send failed ");
	        	        exit(-1);
        		}

			FILE *fp = fopen(rfc_title, "wb");
			char recvbuf[BUFLEN], *cp;
			int bytes_read;

			while (1) {

				bytes_read = recv(sock, recvbuf, 500, 0);
				if ( (cp = strstr(recvbuf, "FILEEND") ) != NULL ) {
					*cp = '\0';
					fwrite(recvbuf, 1, bytes_read-7, fp);
					break;
				}
				fwrite(recvbuf, 1, bytes_read, fp);
			}

			fclose(fp);

			close(sock);

	                gettimeofday(&q, NULL);

        	        printf("The latency for the Lookup operation in Microseconds is: %8ld\n\n", q.tv_usec - p.tv_usec + (q.tv_sec-p.tv_sec)*1000000);


		} else if ( strcmp(msg_type, "PrintRFCDb") == 0 ) {

			printf("Enter the IP Address of the Peer to connect to: ");
			scanf("%s", ip_addr);

			printf("Enter the Port of the above Peer to connect to: ");
			scanf("%d", &portnum);

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

			sprintf(buf, "GET %s %s\n", PRINTRFCDBMSG_STR, PROTOCOL_STR);

                	if ( send(sock, buf, BUFLEN, 0) == -1 ) {
	                        printf("send failed ");
        	                exit(-1);
                	}
			close(sock);

		} else if ( strcmp(msg_type, "PeerDetails") == 0) {

			init_peer_list();
			printf("Enter the IP Address of Peer0: ");
			scanf("%s", ip_addr);
			printf("Enter the Port of Peer0: ");
			scanf("%d", &portnum);
			strcpy(msg_type,PEERDETAILSMSG_STR);

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

			build_PeerDetails_msg(ip_addr, portnum);
			printf("peerDetails Msg:\n%s", buf);

		       	if ( send(sock, buf, BUFLEN, 0) == -1 ) {
        		    	printf("send failed ");
                		exit(-1);
			}

			bytes_read = recv(sock, recvbuf, BUFLEN, 0);
			if(bytes_read == -1){
				printf("Receive failed\n");
				exit(-1);
			}	
			recvbuf[bytes_read] = '\0';

			set_peer_list(recvbuf);
			print_peer_list();
			close(sock);
		}
		else if ( strcmp(msg_type,"PeerExit") == 0 ) {

			printf("Enter the IP Address of the Peer leaving the System: ");
			scanf("%s", ip_addr);

			printf("Enter the Port of the above Peer leaving the system: ");
			scanf("%d", &portnum);

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

			build_PeerExit_msg(ip_addr, portnum);
			printf("PeerExit Msg:\n%s", buf);

		       	if ( send(sock, buf, BUFLEN, 0) == -1 ) {
        		    	printf("send failed ");
                		exit(-1);
			}
			close(sock);
		}
	}

	return(0);

}
