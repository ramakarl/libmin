// *********************************************************************************************************************
// 
//  - Name: app-tcp-rx.c
//  - Created on: Oct 10, 2018
//  - Author: Marcus Pieskä
// 
// *********************************************************************************************************************

#include <netinet/tcp.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h> 
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
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
  __t_flow = setup_trace (trace_base, "tcp-app-rx-flow");	
}

f64 get_time (void) {
  struct timespec t;
  clock_gettime (CLOCK_REALTIME, &t);
  return t.tv_sec + t.tv_nsec / 1.0e9 - __ref;
}

// ** Run RX loop ******************************************************************************************************

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

void run_rx_loop (int sock, f64 tx_time) {
  struct timespec ref_time, curr_time, start_time, rx_time;
  pkt_struct rx_pkt;
  pkt_struct ref_pkt;
  ref_pkt.seq_nr = 1;
  int pkt_size = init_buf (ref_pkt.buf, PKT_SIZE) + sizeof(int);
  int size = 0, in_sock, missing_pkts = 0, late_pkts = 0;
  f64 time_spent;

  struct timespec sleep_period;
  sleep_period.tv_sec = 0;
  sleep_period.tv_nsec = PERIOD;
  clock_gettime (CLOCK_REALTIME, &ref_time);
  clock_gettime (CLOCK_REALTIME, &start_time);
  
  printf ("*** Entering TCP RX loop\n");
	  
  while (time_spent < tx_time) {
    ioctl (sock, FIONREAD, &in_sock);
    while (in_sock >= pkt_size && (size = read (sock, (void*)&rx_pkt, pkt_size)) > 0) {
      in_sock -= pkt_size;
      clock_gettime (CLOCK_REALTIME, &rx_time);
      
      int outcome = memcmp (&ref_pkt, &rx_pkt, pkt_size);
      fprintf (__t_flow, "%.3f:%u:%u:%d\n", get_time (), rx_pkt.seq_nr, size, outcome);
      fflush (__t_flow);
      
      if (rx_pkt.seq_nr > ref_pkt.seq_nr) {
        missing_pkts += rx_pkt.seq_nr - ref_pkt.seq_nr;
        ref_pkt.seq_nr = rx_pkt.seq_nr + 1;
      }
      else if (rx_pkt.seq_nr < ref_pkt.seq_nr) {
        late_pkts++;
      }
      else {
        ref_pkt.seq_nr = rx_pkt.seq_nr + 1;
      }
    }

    clock_gettime (CLOCK_REALTIME, &curr_time); 	
	time_spent = curr_time.tv_sec - start_time.tv_sec;
	time_spent += (curr_time.tv_nsec - start_time.tv_nsec) / 1.0e9;
    nanosleep (&sleep_period, 0);
  }
}

// ** Setup TCP sock ***************************************************************************************************

int get_sock (sock_addr* config, char* tcp_type) { 
  struct sockaddr_in local_addr;
  int slen = sizeof (local_addr), sock = 0;
 
  printf ("*** Creating TCP socket with:\n");
  printf ("    TCP type: %s\n", tcp_type);
  printf ("    Local port: %u\n", config->port);
  printf ("    Local address: %s\n\n", config->addr);
 
  memset ((char*)&local_addr, 0, sizeof (local_addr)); 
  local_addr.sin_family = AF_INET;
  local_addr.sin_port = htons (config->port);
  local_addr.sin_addr.s_addr = inet_addr (config->addr);


  if ((sock = socket (PF_INET, SOCK_STREAM, 0)) == -1) {
    printf ("*** Call to socket FAILED, SOCK:%d\n", sock);
    perror ("*** At socket ()");
    exit (1);
  }
  else {
    printf ("*** Call to socket succeed, SOCK:%d\n", sock);
  }
  
  if (bind (sock, (struct sockaddr*)&local_addr, slen) == -1) {
    printf ("*** Call to bind FAILED, SOCK:%d\n", sock);
    perror ("*** At bind ()");
    exit (1);
  }
  else {
    printf ("*** Call to bind succeed, SOCK:%d\n", sock);
  }
  
  if (listen (sock, 1) == -1)  {
    printf ("*** Call to listen FAILED, SOCK:%d\n", sock);
    perror ("*** At listen ()");
    exit (1);
  }
  else {
    printf ("*** Call to listen succeed, SOCK:%d\n", sock);
  }
  
  if ((sock = accept (sock, 0, 0)) == -1) {
    printf ("*** Call to accept FAILED, SOCK:%d\n", sock);
    perror ("*** At accept ()");
    exit (1);
  }
  else {
    printf ("*** Call to accept succeed, SOCK:%d\n", sock);
  }
  
  if (fcntl (sock, F_SETFL, fcntl (sock, F_GETFL, 0) | O_NONBLOCK) == -1) {
    printf ("*** Call to fcntl FAILED, SOCK:%d\n", sock);
    perror ("*** At fcntl ()");
    exit (1);
  }
  else {
    printf ("*** Call to fcntl succeed, SOCK:%d\n", sock);
  } 
  
  if (setsockopt (sock, IPPROTO_TCP, TCP_CONGESTION, tcp_type, strlen (tcp_type)) < 0) {
    printf ("*** Failed to set TCP cc, SOCK:%d\n", sock);
    perror ("*** At setsockopt ()");
    exit (1);
  }
  else {
    printf ("*** Call to setsockopt succeed, SOCK:%d\n", sock);
  } 
 
  printf ("*** TCP sock created, SOCK:%d\n", sock);

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
  sock_addr local;
  local.port = atoi (argv[4]);
  local.addr = argv[5];
  run_rx_loop (get_sock (&local, tcp_type), tx_time);
}

// *********************************************************************************************************************
