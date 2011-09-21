#ifndef ONLY_ONCE
#define ONLY_ONCE
#define FALSE 0
#define TRUE 1

#define MAX_CHORD_ID 1023

#define RFC_TITLE_LEN_MAX 512
#define RFC_BODY_LEN_MAX 1 << 20

#define CHORD_PORT 5000

#define RFC_PATH "./sample_RFCs/"

#define RFC_NUM_MAX 50

#define MAX_NUM_OF_PEERS 10

typedef struct RFC_db_rec_ {

	int key;
	int value;
	char RFC_title[RFC_TITLE_LEN_MAX];
	char RFC_body[RFC_BODY_LEN_MAX];

	struct RFC_db_rec_ *next;

}RFC_db_rec;

typedef struct peer_info_t_ {

	int chord_id;
	int portnum;
	char iface_name[64];
	char ip_addr[128];

	int successor_id;
	char successor_ip_addr[128];
	int successor_portnum;

	int pred_id;
	char pred_ip_addr[128];
	int pred_portnum;
	
}peer_info_t;

extern char PROTOCOL_STR[128];
extern int is_p0;
extern int well_known_socket;
extern int well_known_port;
extern int well_known_listen_socket;
extern int well_known_listen_port;
extern peer_info_t peer_info;
extern peer_info_t peer_infos[10];
extern RFC_db_rec *rfc_db_head;

extern void print_details();
extern void populate_RFC_Directory(char [][RFC_TITLE_LEN_MAX]);
extern void generate_RFC_Database(int,int,char [][RFC_TITLE_LEN_MAX]);
//extern RFC_db_rec * sort_RFC_db(RFC_db_rec *);
extern void sort_RFC_db();

extern void print_RFC_Database();
extern void initalize_peer_infos();
extern int next_free_position();
extern int check_chordID(int);
extern int generate_chordID(int,int);
extern void put_in_peer_infos(int , char [128], int );

extern void populate_public_ip();
extern void populate_port_num();

extern void server_listen();
extern int test_if_P0_alive(char [128], int);
extern void send_message(char [128], int , char [128], char [1500]);
extern peer_info_t setup_successor(int);
extern RFC_db_rec * find_keys_to_transfer(int, int);
extern void * lookup(); //will take void * as parami
extern void  build_GetKey_msg(char *);




#endif
