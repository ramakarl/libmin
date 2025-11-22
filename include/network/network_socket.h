//---------------------------------------------------------------------
//
// Network System
// Quanta Sciences, Rama Hoetzlein (c) 2007-2020
// 
//---------------------------------------------------------------------

#ifndef DEF_NET_SOCK
	#define DEF_NET_SOCK

	#include <vector>	

  #ifdef _WIN32
    #include <winsock2.h>			// Winsock Ver 2.0
		#include <ws2tcpip.h>
		#pragma comment( lib, "ws2_32.lib")
		#define CX_SOCKET		SOCKET
		#define CX_SOCKLEN		int		
		#define CX_SOCKOPT		char
		#define CX_SOCK_ERROR		SOCKET_ERROR
		#define CX_INVALID_SOCK		INVALID_SOCKET

	#elif __ANDROID__
		#include <sys/socket.h>			// Non-windows Platforms (Linux, Cygwin)
		#include <netinet/in.h>
		#include <arpa/inet.h>
		#include <sys/time.h>
		#include <sys/types.h>
		#include <netdb.h>
		#include <unistd.h>
		#include <fcntl.h>
		#include <errno.h>
		#include <sys/ioctl.h>
		#define CX_SOCKET		int
		#define CX_SOCKLEN		socklen_t		
		#define CX_SOCKOPT		int		
		#define CX_SOCK_ERROR		-1		
		#define CX_INVALID_SOCK		-1

  #elif __linux__
    #include <sys/socket.h>			// Non-windows Platforms (Linux, Cygwin)
		#include <netinet/in.h>
		#include <arpa/inet.h>
		#include <sys/time.h>
		#include <sys/types.h>
		#include <netdb.h>
		#include <unistd.h>
		#include <fcntl.h>
		#include <errno.h>
		#include <sys/ioctl.h>
		#define CX_SOCKET		int
		#define CX_SOCKLEN		socklen_t		
		#define CX_SOCKOPT		int		
		#define CX_SOCK_ERROR		-1		// check: result < SOCK_ERROR
		#define CX_INVALID_SOCK		-1
  #endif

	#ifdef BUILD_OPENSSL
		#include <openssl/bio.h>					// MP: Same cross-platform ? Tentative: Yes
		#include <openssl/ssl.h>
		#include <openssl/err.h>
	#endif

	#include "event_system.h"

	#define NET_ERR						-1
	
	#define NET_CLI						0 // sides
	#define	NET_SRV						1	
	
	#define NET_TCP						0 // modes
	#define NET_UDP						1	
	
	#define NET_SECURITY_UNDEF			0 // security types
	#define NET_SECURITY_FAIL				1 
	#define NET_SECURITY_PLAIN_TCP	2
	#define NET_SECURITY_OPENSSL		4
	#define NET_SECURITY_DTLS				8
	
	// connection types
	#define NTYPE_ANY						0					// localhost, any interface	
	#define NTYPE_BROADCAST			1
	#define NTYPE_SEARCH				2	
	#define NTYPE_CONNECT				3					// specific IP, connect mode

	#define STATE_NONE					0 // stats
	#define STATE_START					1
	#define STATE_HANDSHAKE			2	
	#define STATE_CONNECTED			3
	#define STATE_FAILED				4
	#define STATE_TERMINATED		5
	

	// Network Address Abstraction
	struct HELPAPI NetAddr {
	public:
    		NetAddr ( int t, std::string n, netIP i, int p ) { name = n; type = t; setAddress(AF_INET, i, p);}
		NetAddr ()  { name = ""; type = STATE_NONE; setAddress(AF_INET, 0, 0); }

		void setAddress ( int inet, unsigned long i, unsigned short p ) 
		{
			ip = i;
			port = p;
			// addr struct
			addr.sin_family = inet;
			addr.sin_port = htons(p);
#ifdef _WIN32
			addr.sin_addr.s_addr = i;			
#elif __ANDROID__
			addr.sin_addr.s_addr = i;
#elif __linux__
			addr.sin_addr.s_addr = i;
#endif						
			memset( addr.sin_zero, 0, sizeof(addr.sin_zero ));
		}

		std::string		name;
		char			type;			// type (any, broadcast, search, connect)
		int			sock;
		int			port;
		netIP			ip;
		sockaddr_in		addr;
	};

	// Network Socket Abstraction
	struct HELPAPI NetSock {
		NetSock()	{txBuf=0;txPtr=0;rxBuf=0;rxPtr=0;pktBuf=0;pktPtr=0;}
	
		std::string 		srvAddr;
		int 			srvPort;	
		char			side;			// side (client, server)
		char			mode;			// mode (TCP, UDP)		
		char			state;			// stat (off, connected)
		timeval			timeout;		
		NetAddr			src;			// source socket (ip, port, name, sockID)		
		NetAddr			dest;			// dest socket (ip, port, name, sockID)	
		CX_SOCKET		socket;			// hard socket
		bool			blocking;		// is blocking
		bool			broadcast;		// is broadcast
		int 			security; 		// indicates the security level; e.g., OpenSSL
		int 			reconnectLimit;  	// limits the number of reconnection attempts 
		int 			reconnectBudget; 	// remaining allowed reconnect attempts
		TimeX 			lastStateChange; 	// for tracking when timeouts should occur
		
		// Outgoing buffers
		char*			txBuf;					// transmit buffer (per socket)
		char*			txPtr;				
		int			txPktSize;
		int			txLen;					// transmit so far
		int			txMax;					// transmit max (expandable)

		// Incoming buffers
		char*			rxBuf;					// receive buffer (per socket)
		char*			rxPtr;				
		int			rxLen;					// recv so far
		int			rxMax;					// recv max (expandable)		

		// Incoming packets & event
		int			eventLen;
		Event*			event;					// deserialized event	
		char*			pktBuf;					// current packet
		char*			pktPtr;					// packet offset
		int			pktLen;
		int			pktMax;
		int			pktCounter;		
		
		#ifdef BUILD_OPENSSL
			SSL_CTX 	*ctx;			// MP: Need to read up on these before commenting; Same cross-platform ? Tentative: Yes
			SSL 			*ssl;			// MP:
			BIO 			*bio;			// MP:
		#endif	
	};


#endif
