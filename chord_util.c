#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <sys/time.h>
#include <signal.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include "chord_server.h"
#include <arpa/inet.h>
#include <netdb.h>


void print_RFC_Database ()
{
	RFC_db_rec *p;
	p = rfc_db_head;
	printf("\nThe RFC Database: \n");
	while(p!=NULL)
	{
		printf("KEY : %d \n",p->key);
		p=p->next;
	}
	printf("\n");
}

void generate_RFC_Database (int start, int end)
{
#ifdef DEBUG_FLAG
printf("Inside RFC Generate Database \n");
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
		rndm = rand()%(end - start) + start;
		#ifdef DEBUG_FLAG
		printf("RNDM : %d    %d \n",rndm, rndm%1024);
		#endif
		p = (RFC_db_rec *)malloc(sizeof(RFC_db_rec));
		
		p->next = NULL;
		p->key = (int)rndm % 1024;
        	p->value = (int)rndm;
		strcpy(p->RFC_title, "h");
        	strcpy(p->RFC_body, "b");

		if ( rfc_db_head == NULL) { 
			rfc_db_head = p;
			q = p;
		} else {
			q->next=p;
			q=q->next;
		}				
	}	

}

int create_server(int server_port)
{
        int server_fd, new_fd, set = 1;
        struct sockaddr_in server_addr, client_addr;
        socklen_t addr_len = 0;
//        pid_t pid;
        char p[50];
        /* Standard server side socket sequence*/

        if((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        {
                fprintf(stdout, "socket() failure\n");
                return -1;
        }
        if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &set,
                                sizeof(int)) == -1) {
                fprintf(stdout, "setsockopt() failed");
                return -1;
        }
        bzero(&server_addr, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(server_port) ;
        server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        memset(server_addr.sin_zero, '\0', sizeof(server_addr.sin_zero));

        if(bind(server_fd, (struct sockaddr*)&server_addr, sizeof
                                server_addr) == -1)
        {
                fprintf(stdout, "bind() failed\n");
                return -1;
        
        }

        if(listen(server_fd, 10) == -1)
        {
                fprintf(stdout, "listen() failed\n");
                return -1;
        }
        sprintf(p, "Portal listening on port:%d\n", server_port);
//       fprintf(stdout, p);
        while(1)
        {

                new_fd = accept(server_fd, (struct sockaddr*)&client_addr, &addr_len);
                if(new_fd == -1)
                {
                        //log
                        fprintf(stdout, "accept() failed\n");
                        return -1;
                
                }
                fprintf(stdout, "new connection accepted\n");
                
                //Create a Thread to handle this new request

        }

        return 0;
}


int create_client(char *address, int port)
{
    int sockfd, portno;
    struct sockaddr_in serv_addr;
    struct hostent *server;

//    if (argc < 3) {
  //     fprintf(stderr,"usage %s hostname port\n", argv[0]);
  //     exit(0);
  //  }
    portno = port;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        fprintf(stderr,"ERROR opening socket");
    server = gethostbyname(address);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(portno);
    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
        fprintf(stderr,"ERROR connecting");
    return sockfd;

}
