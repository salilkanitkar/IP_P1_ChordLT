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

void print_RFC_Database ()
{
	RFC_db_rec *p;
	p = rfc_db_head;
	printf("\nThe RFC Database: \n");
	while(p!=NULL)
	{
		printf("Key : %d Value:%d \n",p->key, p->value);
		p=p->next;
	}
	printf("\n");
}

int generate_random_number(int start, int end)
{
	return ( (rand()%(end - start)) + start ) ;
}


void generate_RFC_Database (int start, int end)
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
		sprintf(p->RFC_title, "ID:%d Value:%d", p->key, p->value);
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

int create_server(int server_port)
{
        int server_fd, new_fd, set = 1;
        struct sockaddr_in server_addr, client_addr;
        socklen_t addr_len = 0;
        char p[50];

	pthread_t thread_id;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	int thread_count = 0;

        /* Standard server side socket sequence*/

        if((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)  {
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
		pthread_create(&thread_id, &attr, lookup, NULL); //NULL will be replaced by param
		thread_count++;
		
        }

        return 0;
}

/* This is starting point of the lookup */
void * lookup() 
{
	//int chord_id = (int) param;

	//further processing on chord_id
	
	pthread_exit(NULL);
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
