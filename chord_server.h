#ifndef ONLY_ONCE
#define ONLY_ONCE
#define FALSE 0
#define TRUE 1

#define MAX_CHORD_ID 1023

#define RFC_TITLE_LEN_MAX 512
#define RFC_BODY_LEN_MAX 1 << 20

#define CHORD_PORT 5000

extern int is_p0;
extern int well_known_socket;
extern int well_known_port;

typedef struct RFC_db_rec_ {

	int key;
	int value;
	char RFC_title[RFC_TITLE_LEN_MAX];
	char RFC_body[RFC_BODY_LEN_MAX];

	struct RFC_db_rec_ *next;

}RFC_db_rec;

typedef struct peer_info_t_ {

	int chord_id;
	int successor;
	int portnum;

	char iface_name[64];
	char ip_addr[128];

}peer_info_t;

extern RFC_db_rec *rfc_db_head;
extern void generate_RFC_Database(int,int);
extern void print_RFC_Database();
extern int create_server(int);
extern int create_client (char *,int);
extern peer_info_t peer_info;
extern void populate_public_ip();
extern void populate_port_num();
extern void * lookup(); //will take void * as param
#endif
