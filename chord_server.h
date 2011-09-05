#ifndef ONLY_ONCE
#define ONLY_ONCE
#define FALSE 0
#define TRUE 1

#define MAX_CHORD_ID 1023

#define RFC_TITLE_LEN_MAX 512
#define RFC_BODY_LEN_MAX 1 << 20

#define PEER_P0_IP "127.0.0.1"
#define PEER_P0_PORT 5000

extern int is_p0;

typedef struct RFC_db_rec_ {

	int key;
	int value;
	char RFC_title[RFC_TITLE_LEN_MAX];
	char RFC_body[RFC_BODY_LEN_MAX];

	struct RFC_db_rec_ *next;

}RFC_db_rec;

extern RFC_db_rec *rfc_db_head;
extern void generate_RFC_Database(int,int);
extern void print_RFC_Database();
#endif
