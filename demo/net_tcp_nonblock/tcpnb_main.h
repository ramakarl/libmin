
#ifndef NETDEMO_MAIN
#define NETDEMO_MAIN

#include "network_system.h"

#define BUF_SIZE 1400
#define PKT_SIZE 1200 

#define ERROR_NONE 0
#define ERROR_MISSING_SERVER_KEYS 1
#define ERROR_MISSING_CLIENT_KEYS 2

#define PROTOCOL_TCP_ONLY 0
#define PROTOCOL_SSL_ONLY 1
#define PROTOCOL_TCP_AND_SSL 2

typedef struct pkt_struct {
	int seq_nr;
	char buf[ BUF_SIZE ];
} pkt_struct; 

#endif
