//----------------------------------------------------------------------------------------------------------------------
// -> COMMENTS <-
//----------------------------------------------------------------------------------------------------------------------
//
// Network System
// Quanta Sciences, Rama Hoetzlein (c) 2007-2020
// Updated: R.Hoetzlein, 2024
//
// Features:
// - Client & Server model
// - Server maintains many client connections (socket list)
// - Buffered event queue
// - User-level callbacks to process event queue
// - Events hold packet payloads
// - Events have 4-byte names for efficient custom protocols
// - Events have attach/get methods to help serialize data
// - Event memory pools to handle many, small events
// - Arbitrary event size, regardless of TCP/IP buffer size
// - Graceful disconnect for unexpected shutdown of client or server
// - Reconnect for clients
// - Verbose error handling
// - C++ class model allows for multiple client/server objects
// - C++ class model with no inheritence (for simplicity)
// - Cross-platform and tested on Windows, Linux and Android
// 
//----------------------------------------------------------------------------------------------------------------------
// -> HEADER <-
//----------------------------------------------------------------------------------------------------------------------

#ifndef DEF_NETWORK_H
#define DEF_NETWORK_H

#include "common_defs.h"
#include "network_socket.h"
#include "event_system.h"
#include "time.h"

#ifdef __ANDROID__
	#include <sys/socket.h>
	#include <arpa/inet.h>
#endif

#include <cstdio>
#include <map>

#define NET_NOT_CONNECTED		11002
#define NET_DISCONNECTED		107

#define SRV_PORT			8000
#define SRV_SOCK			0
#define CLI_PORT			8001
#define CLI_SOCK			1

#define SRV_UDP			0
#define SRV_TCP			1
#define CLI_UDP			2
#define CLI_TCP			3

#define CONN_CLI		0
#define CONN_SRV		1
#define CONN_UDP		0
#define CONN_TCP		1

#define NET_BUFSIZE			1500		// Typical UDP max packet size

#define PRINT_VERBOSE 0
#define PRINT_VERBOSE_HS 1
#define PRINT_ERROR 2
#define PRINT_ERROR_HS 3
#define PRINT_FLOW 4

// -- NOTES --
// IP               = 20 bytes
// UDP = IP + 8     = 28 bytes
// TCP = IP + 28    = 48 bytes
// Event            = 24 bytes (header)
// TCP + Event      = 72 bytes (over TCP)

typedef int (*funcEventHandler) ( Event& e, void* this_ptr  );
typedef std::string str;

class EventPool;

class HELPAPI NetworkSystem {
	
#define VALID_INDEX(index) ((index) >= 0 && (index) < m_socks.size())
	
#ifdef _WIN32
	typedef int socklen_t;
	typedef struct fd_set fd_set;
#endif
	
public:
	NetworkSystem ( const char* trace_file_name = NULL );

	// Network System
	void netInitialize ( );
	void netCreate ( );
	void netDestroy ( );
	void netShowVerbose ( bool v ) { m_printVerbose = v; }
    void netShowFlow ( bool v ) { m_printFlow = v; }	
	void netList ( bool verbose = false ); // list all connections/sockets
	str netPrintAddr ( NetAddr adr );
	
	// Miscellaneous config API
	void netSetSelectInterval ( int time_ms ); 
	
	// Security config API
	bool netSetReconnectInterval ( int time_ms ); 
	bool netSetReconnectLimit ( int limit );
	bool netSetReconnectLimit ( int limit, int sock_i );
	bool netSetSecurityLevel ( int levels );
	bool netSetSecurityLevel ( int levels, int sock_i );
	bool netSetPathToPublicKey ( str path );
	bool netSetPathToPrivateKey ( str path );
	bool netSetPathToCertDir ( str path );
	bool netSetPathToCertFile ( str path );
	
	// Server API
	void netServerStart ( netPort srv_port, int security = NET_SECURITY_UNDEF );
	void netServerAcceptClient ( int sock_i );
	void netServerCheckConnectionHandshakes ( );
	void netServerProcessIO ( );
	void netServerCompleteConnection ( int sock_i );

	// Client API
	void netClientStart ( netPort srv_port, str srv_addr="127.0.0.1" );
	int netClientConnectToServer ( str srv_name, netPort srv_port, bool block = false, int sock_i = -1 );
	void netClientCheckConnectionHandshakes ( );
	void netClientProcessIO ( );
	
	// Client & server common API
	int netCloseConnection ( int sock_i );
	int netCloseAll ( );

	// Event processing
	void netProcessEvents ( Event& e );
	int netProcessQueue ( void );
	void netResetRecvBuf ( );
	void netResizeRecvBuf ( int len );
	void netReceiveData ( int sock_i );
	void netReceiveByInjectedBuf ( int sock_i, char* buf, int buflen );
	void netDeserializeEvents ( int sock_i );
	Event netMakeEvent ( eventStr_t name, eventStr_t sys );	
	bool netSend ( Event& e, int sock=-1 );
	bool netSendLiteral ( str str_lit, int sock_i );
	void netQueueEvent ( Event& e ); // Place incoming event on recv queue
	int netEventCallback ( Event& e ); // Processes network events (dispatch)
	void netSetUserCallback ( funcEventHandler userfunc )	{ m_userEventCallback = userfunc; }
	bool netIsConnectComplete ( int sock_i );
	bool netCheckError ( int result, int sock_i );	
	
	// Accessors
	TimeX		getSysTime ( )				{ return TimeX::GetSystemNSec ( ); }
	str			getHostName ( )				{ return m_hostName; }
	bool		isServer ( )					{ return m_hostType == 's'; }
	bool		isClient ( )					{ return m_hostType == 'c'; }
	bool 		netIsQueueEmpty ( )		{ return m_eventQueue.size ( ) == 0; }
	netIP		getHostIP ( )					{ return m_hostIp; }
	int			getMaxPacketLen ( )		{	return m_maxPacketLen; }
	EventPool*  getNetPool ( )		{ return m_eventPool; }
	
	NetSock*	getSock ( int sock_i )			{ return VALID_INDEX(sock_i) ? &m_socks[ sock_i ] : 0; }
	str			getSockIP ( int sock_i )		{ return VALID_INDEX(sock_i) ? getIPStr ( m_socks[ sock_i ].dest.ipL ) : ""; }
	int			getServerSock ( int sock_i )	{ return VALID_INDEX(sock_i) ? m_socks[ sock_i ].dest.sock : -1; }
	
	str 		getIPStr ( netIP ip ); // return IP as a string
	netIP		getStrToIP ( str name );

protected:
	str netPrintf ( int flag, const char* fmt, ... );

private: // MP: Move this stuff
	funcEventHandler m_userEventCallback; // User event handler

private: // Functions

	// Handling non-blocking OpenSSL handshake
	#ifdef BUILD_OPENSSL
		void netFreeSSL ( int sock_i ); 
		str netGetErrorStringSSL ( int ret, SSL* ssl );
		int netNonFatalErrorSSL ( int sock_i, int ret ); 
		void netServerSetupHandshakeSSL ( int sock_i ); 
		void netServerAcceptSSL ( int sock_i );
		void netClientSetupHandshakeSSL ( int sock_i ); 
		void netClientConnectSSL ( int sock_i );		
    #endif

	// Abtract socket functions
	int netAddSocket ( int side, int mode, int state, bool block, NetAddr src, NetAddr dest );
	int netFindSocket ( int side, int mode, int type );
	int netFindSocket ( int side, int mode, int state, NetAddr dest );
	int netFindOrCreateSocket(str srv_name, netPort srv_port, netIP srv_ip, bool block );
	int netFindOutgoingSocket ( bool bTcp );
	int netManageHandshakeError ( int sock_i, std::string reason );
	int netManageTransmitError ( int sock_i, std::string reason, int force = 0 );
	int netDeleteSocket ( int sock_i, int force=0 );
	netIP netResolveServerIP(str name, netPort port);
	bool netIsError ( int result );	// Socket-specific error check
	void netReportError ( int result );

	// Low level handling of sockets
	void netStartSocketAPI ( );
	void netSetHostname ( );
	int netSocketAdd ( int sock_i );
	int netSocketBind ( int sock_i );	
	int netSocketConnect ( int sock_i );
	int netSocketListen ( int sock_i );
	int netSocketAccept ( int sock_i,  SOCKET& tcp_sock, netIP& cli_ip, netPort& cli_port );	
	int netSocketRecv ( int sock_i, char* buf, int buflen, int& recvlen); 
	void netSocketReuse(int sock_i );
	bool netSocketIsConnected ( int sock_i );
	bool netSocketIsSelected ( fd_set* sockSet, int sock_i );
	int netSocketSelect ( fd_set* sockReadSet, fd_set* sockWriteSet );
	void netSendResidualEvent ( int sock_i );

	// Short helpers, used to simplify the program elsewhere
	void sleep_ms ( int time_ms );
	unsigned long get_read_ready_bytes ( SOCKET sock_h );		
	bool invalid_socket_index ( int sock_i );
	
	// Handling tracing and logging
	double get_time ( );
	void trace_setup ( const char* function_name );
	void trace_enter ( const char* function_name );
	void trace_exit ( const char* function_name );
	void net_perf_push ( const char* msg );
	void net_perf_pop ( );
	
	// Cross-platform socket interactions
	void CXSetHostname ( );
	void CXSocketApiInit ( );
	void CXSocketMakeBlock ( SOCKET sock_h, bool block = false );
	void CXSocketMakeNoDelay ( SOCKET sock_h );
	unsigned long CXSocketReadBytes ( SOCKET sock_h );
	int CXSocketIvalid ( SOCKET sock_h );
	int CXSocketError ( SOCKET sock_h );
	bool CXSocketBlockError ( );
	str CXGetErrorMsg ( int& error_id );
	void CXSocketUpdateAddr ( int sock_i, bool src = true );
	void CXSocketClose ( SOCKET sock_h );
	bool CXIsConnectError ( std::string& msg );
	str CXGetIpStr ( netIP ip );
	
private: // State
	
	// General
	uchar m_hostType;
	str m_hostName;
	netIP m_hostIp;
	int m_readyServices;
	timeval m_rcvSelectTimout;	
	TimeX m_lastClientConnectCheck;
	std::vector< NetSock > m_socks;
	
	// Event related
	EventPool* m_eventPool; 
	EventQueue m_eventQueue;

	// Incoming event data
	int	m_dataLen;
	int	m_eventLen;
	Event m_event; // Incoming event

	// Network buffers
	int m_packetLen;
	int m_packetCounter;
	char* m_packetPtr;
	char m_packetBuf[ NET_BUFSIZE ];
	char* m_recvPtr;
	char* m_recvBuf;
	int m_recvLen, m_recvMax;
	int	m_maxPacketLen;
	
	// Debug and trace related
	int	m_check;
	int m_indentCount;
	bool m_printVerbose;
	bool m_printFlow;
	FILE* m_trace;
	TimeX m_refTime;
	
	// Security related
	int m_security;
	int m_reconnectInterval;
	int m_reconnectLimit;
	str m_pathPublicKey;
	str m_pathPrivateKey;
	str m_pathCertDir;
	str m_pathCertFile;
};

extern NetworkSystem* net;

#endif 

//----------------------------------------------------------------------------------------------------------------------
// -> END <-
//----------------------------------------------------------------------------------------------------------------------
