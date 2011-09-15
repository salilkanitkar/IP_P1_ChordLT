#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>

char PROTOCOL_STR[128] = "Chord-LT/1.0";

char FETCHMSG_STR[128] = "FetchRFC";

#define BUFLEN 1500
char buf[BUFLEN];

void build_FetchRFC_msg(char *ip_addr, int portnum, char *rfc_title) 
{
	sprintf(buf, "GET %s %s %s\nIP:%s\nPort:%d\n", FETCHMSG_STR, rfc_title, PROTOCOL_STR, ip_addr, portnum);
}

int main()
{

        struct sockaddr_in sock_client;
        int sock, slen = sizeof(sock_client), ret;

	char ip_addr[128], rfc_title[128];
	int portnum;

	printf("Enter the IP Address of the Peer to connect to: ");
	scanf("%s", ip_addr);

	printf("Enter the Port of the above Peer to connect to: ");
	scanf("%d", &portnum);

	printf("Enter the RFC Tile of the RFC to be fetched: ");
	scanf("%s", rfc_title);
	
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

	build_FetchRFC_msg(ip_addr, portnum, rfc_title);
	printf("FetchRFC Msg:\n%s", buf);

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

	return(0);

}
