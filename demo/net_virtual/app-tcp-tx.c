// *********************************************************************************************************************
// 
//  - Name: app-tcp-tx.c
//  - Created on: Oct 10, 2018
//  - Author: Marcus Pieskä
// 
// *********************************************************************************************************************

#include <netinet/tcp.h>
#include <sys/ioctl.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h> 
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <netinet/tcp.h>
#include <fcntl.h>

// *********************************************************************************************************************

#define DIR_SIZE 200
#define SEQ_INDEX 0
#define TS_INDEX 4
#define BUF_SIZE 2000
#define PKT_SIZE 1200 
#define PERIOD 1e6

// *********************************************************************************************************************

typedef double f64;

typedef struct sock_addr {
  uint16_t port;
  char* addr;  	
} sock_addr;

typedef struct pkt_struct {
	int seq_nr;
	char buf[BUF_SIZE];
} pkt_struct; 

// ** Tracing **********************************************************************************************************

f64 __ref;
FILE* __t_flow = stdout;

FILE* setup_trace (const char* trace_base, const char* name) {
  FILE* trace_ptr;
  char trace_file[DIR_SIZE];
  memset (trace_file, 0, DIR_SIZE);
  strcat (trace_file, trace_base);
  strcat (trace_file, name);
  trace_ptr = fopen (trace_file, "w");
  chmod (trace_file, S_IRWXO);
  return trace_ptr;
}

void setup_flow_trace (const char* trace_base) {
  __t_flow = setup_trace (trace_base, "tcp-app-tx-flow");	
}

f64 get_time (void) {
  struct timespec t;
  clock_gettime (CLOCK_REALTIME, &t);
  return t.tv_sec + t.tv_nsec / 1.0e9 - __ref;
}

// ** Run TX loop ******************************************************************************************************

int init_buf (char* buf, const int size) {
  for (int i = 0, c = 65; i < size; i++) {
    if (i == size - 1) {
      memset (buf + i, '*', 1);
      memset (buf + i + 1, '\0', 1);
    }
    else if (i % 50 == 49) {
      memset (buf + i, '\n', 1);
      c++;
    }
    else if (i % 10 == 9) {
      memset (buf + i, c, 1);
    }
    else {
      memset (buf + i, '-', 1);
    }
  }
  printf ("*** Packet content:\n\n%s\n*** Size is %luB \n", buf, strlen(buf));
  return (int)strlen(buf);
}

void run_tx_loop (int sock, f64 tx_time) {
  struct timespec ts;
  pkt_struct tx_pkt;
  tx_pkt.seq_nr = 1;
  int pkt_size = init_buf (tx_pkt.buf, PKT_SIZE) + sizeof(int);
  f64 end_time = tx_time, current_time;
  int to_send = pkt_size, sent = 0, offset = 0;
  
  clock_gettime (CLOCK_REALTIME, &ts);
  end_time += ts.tv_sec + ts.tv_nsec / 1.0e9;
  current_time = ts.tv_sec + ts.tv_nsec / 1.0e9;
  
  printf ("*** Entering TCP TX loop\n");
  
  while (current_time < end_time) {
	offset = pkt_size - to_send;
    if ((sent = write (sock, (char*)&tx_pkt + offset, to_send)) > 0) {   
      fprintf (__t_flow, "%.3f:%u:%u\n", get_time (), tx_pkt.seq_nr, sent);
      fflush (__t_flow);
      to_send -= sent;
    } 
    if (to_send <= 0) {
	  tx_pkt.seq_nr++;
	  to_send = pkt_size;
	}
    clock_gettime (CLOCK_REALTIME, &ts); 
    current_time = ts.tv_sec + ts.tv_nsec / 1.0e9; 
  }
}

// ** Setup TCP sock ***************************************************************************************************

int get_sock (sock_addr* remote, char* tcp_type) { 
  struct sockaddr_in remote_addr;
  int slen = sizeof (remote_addr), sock = 0, no_delay = 0;

  memset ((char*)&remote_addr, 0, sizeof (remote_addr)); 
  remote_addr.sin_family = AF_INET;
  remote_addr.sin_port = htons (remote->port);
  remote_addr.sin_addr.s_addr = inet_addr (remote->addr);

  if ((sock = socket (PF_INET, SOCK_STREAM, 0)) == -1) {
    perror ("*** At socket ()");
    exit (1);
  }
  
  if (connect (sock, (const struct sockaddr*)&remote_addr, slen) == -1) {
    perror ("*** At connect ()");
    exit (1);
  }
  else {
    printf ("*** Call to connect succeed, SOCK:%d\n", sock);
  }
	
  if (setsockopt (sock, SOL_TCP, TCP_NODELAY, (void*)&no_delay, sizeof (no_delay)) == -1) {
    perror ("*** At setsockopt ()");
    exit (1);
  }
  
  if (setsockopt (sock, IPPROTO_TCP, TCP_CONGESTION, tcp_type, strlen (tcp_type)) < 0) {
    perror ("*** At setsockopt ()");
    exit (1);
  }
  
  int flags = fcntl (sock, F_GETFL, 0), ret;
  if (flags == -1) {
    perror ("*** At fcntl ()");
    exit (1);
  } 
  flags |= O_NONBLOCK;
  if ((ret = fcntl (sock, F_SETFL, flags)) == -1) {
    perror ("*** At fcntl ()");
    exit (1);
  } 
  else {
    printf ("Call to set non-blocking succeded");
  }

  return sock;
}

// ** Main *************************************************************************************************************

int main (int argc, char* argv[]) {
  setvbuf (stdout, NULL, _IONBF, 0);
  if (argc < 6) {
    printf ("*** Failure: argc < 5: %i\n", argc);
    exit (0);
  }
  setup_flow_trace ("../");
  __ref = atof(argv[1]);
  char* tcp_type = argv[2];
  f64 tx_time = atof (argv[3]);
  f64 tx_rate = 0;
  if (argc > 6) {
    tx_rate = atof(argv[6]);
  }
  sock_addr remote;
  remote.port = atoi (argv[4]);
  remote.addr = argv[5];
  run_tx_loop (get_sock (&remote, tcp_type), tx_time); 
}

// *********************************************************************************************************************
