#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <sys/time.h>
#include <signal.h>
#include <string.h>
#include "chord_server.h"

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

