#ifndef CHORD_CLIENT
#define CHORD_CLIENT

#define BUFLEN 1500

typedef struct peer_track_t_ {
        int chord_id;
        int portnum;
        char ip_addr[128];
} peer_track_t;

#endif
