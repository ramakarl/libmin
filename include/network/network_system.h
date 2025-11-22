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
	bool netServerStart ( netPort srv_port, int security = NET_SECURITY_UNDEF );
	void netServerAcceptClient ( int sock_i );
	void netServerCheckConnectionHandshakes ( );
	void netServerProcessIO ( );
	void netServerCompleteConnection ( int sock_i );

	// Client API
	void netClientStart ( netPort srv_port, str srv_addr="127.0.0.1" );
	int netClientConnectToServer ( str srv_name, netPort srv_port, bool block = false, int sock_i = -1 );
	void netClientCheckConnectionHandshakes ( );
	void netClientProcessIO ( );
	void netClientHandshake ( int sock_i );
	void netClientCompleteConnection( int sock_i );
	
	// Client & server common API
	int netCloseConnection ( int sock_i );
	int netCloseAll ( );

	// Event processing
	void netProcessEvents ( Event& e );
	int netProcessQueue ( void );
	void netResetBufs ();
	void netResetBuf ( char*& buf, char*& ptr, int& len);
	void netExpandBuf ( char*& buf, char*& ptr, int& max, int& len, int new_max );
	void netReceiveData ( int sock_i );
	void netReceiveByInjectedBuf ( int sock_i, char* buf, int buflen );
	void netDeserializeEvents ( int sock_i );
	void netMakeEvent ( Event& e, eventStr_t name, eventStr_t sys );	
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
	netIP		getHostIP()						{ return m_hostIp; }
	str			getHostIPStr()				{ return getIPStr(m_hostIp); }
	bool		isServer ( )					{ return m_hostType == 's'; }
	bool		isClient ( )					{ return m_hostType == 'c'; }
	bool 		netIsQueueEmpty ( )		{ return m_eventQueue.getSize ( ) == 0; }
	
	EventPool*  	getNetPool ( )		{ return m_eventPool; }		
	NetSock*	getSock ( int i );		// socket itself
	str			getSockSrcIP(int i);			// src IP of socket
	str			getSockDestIP ( int i );	// dest IP of socket	
	int			getServerSock ( int i );	// client's socket on server
	str 		getIPStr ( netIP ip );		// return IP as a string
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
	void netReportError ( int result );
	bool netFuncError (int ret );		// check if TCP/IP func return is valid

	// Low level handling of sockets
	void netStartSocketAPI ( );
	void netSetHostname ( );
	int netSocketCreate ( int sock_i );
	int netSocketBind ( int sock_i );	
	int netSocketConnect ( int sock_i );
	int netSocketListen ( int sock_i );
	int netSocketAccept ( int sock_i, CX_SOCKET& tcp_sock, netIP& cli_ip, netPort& cli_port );	
	int netSocketRecv ( int sock_i, char* buf, int buflen ); 
	void netSocketReuse(int sock_i );
	bool netSocketIsConnected ( int sock_i );
	bool netSocketIsSelected ( fd_set* sockSet, int sock_i );
	int netSocketSelect ( fd_set* sockReadSet, fd_set* sockWriteSet );
	void netSendResidualEvent ( int sock_i );

	// Short helpers, used to simplify the program elsewhere
	void sleep_ms ( int time_ms );
	unsigned long get_read_ready_bytes ( CX_SOCKET sock_h );		
	bool valid_socket_index ( int sock_i );
	
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
	void CXSocketSetBlockMode ( CX_SOCKET sock, bool block = false );
	void CXSocketMakeNoDelay ( CX_SOCKET sock );
	unsigned long CXSocketReadBytes ( CX_SOCKET sock );
	bool CXSocketIsValid ( CX_SOCKET sock);			// check if a socket is valid	
	bool CXSocketBlockError ( );
	str CXGetErrorMsg ( int& error_id );
	void CXSocketUpdateAddr ( int sock_i, bool src = true );
	void CXSocketClose ( CX_SOCKET sock_h );
	bool CXSocketWouldBlock (std::string& msg);
	str CXGetIpStr ( netIP ip );

	xlong ComputeChecksum (char* buf, int len);
	
private: // State
	
	// General
	uchar m_hostType;
	str m_hostName;
	netIP m_hostIp;
	int m_readyServices;
	timeval m_rcvSelectTimout;	
	TimeX m_lastClientConnectCheck;
	TimeX m_lastNetProcess;
	int m_processInterval;
	std::vector< NetSock > m_socks;
	
	// Event related
	EventPool* m_eventPool; 
	EventQueue m_eventQueue;
	
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
