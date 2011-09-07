#define FALSE 0
#define TRUE 1

#define MAX_CHORD_ID 1023

#define RFC_TITLE_LEN_MAX 512
#define RFC_BODY_LEN_MAX 1 << 20

#define PEER_P0_IP "127.0.0.1"
#define PEER_P0_PORT 5000

int is_p0 = FALSE;

typedef struct RFC_db_rec_ {

	int key;
	int value;
	char RFC_title[RFC_TITLE_LEN_MAX];
	char RFC_body[RFC_BODY_LEN_MAX];

	struct RFC_db_rec_ *next;

}RFC_db_rec;

RFC_db_rec *rfc_db_head=0;

