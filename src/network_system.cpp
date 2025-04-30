//----------------------------------------------------------------------------------------------------------------------
//
// Network System
// Quanta Sciences, Rama Hoetzlein (c) 2007-2020
//
//----------------------------------------------------------------------------------------------------------------------

#include <assert.h>


#include "network_system.h"

#ifdef __linux__
	#include <net/if.h>
	#include <netinet/in.h>
	#include <netinet/tcp.h> 
	#include <sys/stat.h>
	#include <errno.h>    
#elif _WIN32
	#include <winsock2.h>
#elif __ANDROID__
	#include <net/if.h>
	#include <netinet/in.h>
	#include <netinet/tcp.h> 
#endif

//#undef BUILD_OPENSSL

#ifdef BUILD_OPENSSL
	#include <openssl/opensslv.h>
	#include <openssl/crypto.h>
	#include <openssl/pem.h>
	#include <openssl/err.h>
	#include <openssl/md5.h>
	#include <openssl/ssl.h>	
	#include <openssl/x509v3.h>
#endif

//#define DEBUG_STREAM				// enable this to read/write network stream to disk file

//----------------------------------------------------------------------------------------------------------------------
// TRACING FUNCTIONS
//----------------------------------------------------------------------------------------------------------------------

#define TRACE_FUNCTION_CALLS
#define TRACE_FUNCTION_FLUSH

double NetworkSystem::get_time ( ) 
{
	TimeX current_time;
	current_time.SetTimeNSec ( );
	return current_time.GetElapsedSec ( m_refTime );
}

void NetworkSystem::trace_setup ( const char* trace_file_path )
{
	m_trace = fopen ( trace_file_path, "w" );
	if ( m_trace == 0 ) {
		netPrintf ( PRINT_ERROR, "Could not open trace file: Errno: %d", errno );
		return;
	}
	m_refTime.SetTimeNSec ( );
	#ifdef __linux__
		chmod ( trace_file_path, S_IRWXO ); 
	#endif
}

void NetworkSystem::trace_enter ( const char* function_name ) 
{
	if ( m_trace == 0 ) {
		return;
	}
	str pad ( m_indentCount * 2, ' ' );
	fprintf ( m_trace, "%.9f:i:%s:%s\n", get_time ( ),  pad.c_str ( ), function_name );
	#ifdef TRACE_FUNCTION_FLUSH
		fflush ( m_trace );
	#endif
	m_indentCount++;
}

void NetworkSystem::trace_exit ( const char* function_name )
{
	if ( m_trace == 0 ) {
		return;
	}
	m_indentCount--;
	if ( m_indentCount < 0 ) {
		netPrintf ( PRINT_ERROR, "Bad indent: Call from: %s", function_name );
		m_indentCount = 0;
	}
	str pad ( m_indentCount * 2, ' ' );
	fprintf ( m_trace, "%.9f:o:%s:%s\n", get_time ( ), pad.c_str ( ), function_name );
	#ifdef TRACE_FUNCTION_FLUSH
		fflush ( m_trace );
	#endif
}

void NetworkSystem::net_perf_push ( const char* msg )
{
	#ifdef PROFILE_NET
		PERF_PUSH ( msg );
	#endif
}

void NetworkSystem::net_perf_pop ( )
{
	#ifdef PROFILE_NET
		PERF_POP ( );
	#endif
}

//----------------------------------------------------------------------------------------------------------------------
// -> TRACING HOOKS <-
//----------------------------------------------------------------------------------------------------------------------

#ifdef TRACE_FUNCTION_CALLS
	#define TRACE_SETUP(msg) this->trace_setup(msg)
	#define TRACE_ENTER(msg) this->trace_enter(msg)
	#define TRACE_EXIT(msg) this->trace_exit(msg)
	#define NET_PERF_PUSH(msg) this->net_perf_push(msg)
	#define NET_PERF_POP(msg) this->net_perf_pop()
#else 
	#define TRACE_SETUP(msg) (void)0
	#define TRACE_ENTER(msg) (void)0
	#define TRACE_EXIT(msg) (void)0
	#define NET_PERF_PUSH(msg) (void)0
	#define NET_PERF_POP(msg) (void)0
#endif 

//----------------------------------------------------------------------------------------------------------------------
// -> CROSS-COMPATIBILITY <-
//----------------------------------------------------------------------------------------------------------------------

inline void NetworkSystem::CXSetHostname ( )
{
	TRACE_ENTER ( (__func__) );
	// NOTE: Host may have multiple interfaces, this is just to get one valid local IP address (-Marty)
	struct in_addr addr;
	int ret;
	char name [ 512 ];
	if ( ( ret = gethostname ( name, sizeof ( name ) ) ) != 0 ) {
		netPrintf ( PRINT_ERROR, "Cannot get local host name: Return %d", ret );
	}
	
	#ifdef _WIN32
		struct hostent* phe = gethostbyname ( name );
		if ( phe == 0 ) {
			netPrintf ( PRINT_ERROR, "Bad host lookup in gethostbyname" );
		}
		for ( int i = 0; phe->h_addr_list [ i ] != 0; ++i ) {
			memcpy ( &addr, phe->h_addr_list [ i ], sizeof ( struct in_addr ) );
			m_hostIp = addr.S_un.S_addr;
		}
	#else
		int sock_fd;
		struct ifreq ifreqs [ 20 ];
		struct ifconf ic;
		ic.ifc_len = sizeof ( ifreqs );
		ic.ifc_req = ifreqs;
		sock_fd = socket ( AF_INET, SOCK_DGRAM, 0 );
		if ( sock_fd < 0 ) {
			dbgprintf ( "netSys ERROR: Cannot create socket to get host name.\n" );
		}
		if ( ioctl ( sock_fd, SIOCGIFCONF, &ic ) < 0 ) {
			dbgprintf ( "netSys ERROR: Cannot do ioctl to get host name.\n" );
		}

		dbgprintf ("Network Interfaces:\n");
		for ( int i = 0; i  < ic.ifc_len / sizeof ( struct ifreq ); i++ ) {
			netIP ip;
			ip = (netIP) ((struct sockaddr_in*) &ifreqs[ i ].ifr_addr)->sin_addr.s_addr;
			dbgprintf ( " %s: %s\n", ifreqs[ i ].ifr_name, getIPStr ( ip ).c_str ( ) );
			if ( ifreqs[i].ifr_name [ 0 ] != 'l' ) {  // skip loopback, get first eth0
				m_hostIp = ip;
				break;
			}
		}
		close ( sock_fd );
    #endif
	m_hostName = name;
	TRACE_EXIT ( (__func__) );
}

inline void NetworkSystem::CXSocketApiInit ( )
{
	TRACE_ENTER ( (__func__) );
	#if defined(_MSC_VER) || defined(_WIN32) // Winsock startup
		WSADATA WSAData;
		int status;
		if ( ( status = WSAStartup ( MAKEWORD ( 1,1 ), &WSAData ) ) == 0 ) {
			netPrintf ( PRINT_VERBOSE, "Started Winsock" );
		} else {
			netPrintf ( PRINT_ERROR, "Unable to start Winsock: Return: %d", status );
		}
	#endif
	TRACE_EXIT ( (__func__) );
}

inline void NetworkSystem::CXSocketSetBlockMode ( CX_SOCKET sock, bool block )
{
	TRACE_ENTER ( (__func__) );
	#ifdef _WIN32 // windows
		unsigned long block_mode = block ? 0 : 1;			// inverted on Windows. See ioctlsocket spec.
		ioctlsocket ( sock, FIONBIO, &block_mode ); // FIONBIO = non-blocking mode	
	#else // linux
		int flags = fcntl ( sock, F_GETFL, 0 ), ret;
		if ( flags == -1 ) {
			netPrintf ( PRINT_ERROR, "Failed at fcntl F_GETFL: Return: %d", flags );
			TRACE_EXIT ( (__func__) );
			return;
		}		
		if ( block ) {
			flags &= ~O_NONBLOCK;
		} else {
			flags |= O_NONBLOCK;
		}
		if ( ( ret = fcntl ( sock, F_SETFL, flags ) ) == -1 ) {
			netPrintf ( PRINT_ERROR, "Failed at fcntl F_SETFL: Return: %d", ret );
		} else {
			netPrintf ( PRINT_VERBOSE, "Call to set block mode succeded" );
		}
	#endif
	TRACE_EXIT ( (__func__) );
}

unsigned long NetworkSystem::CXSocketReadBytes ( CX_SOCKET sock_h )
{   
	TRACE_ENTER ( (__func__) );
	unsigned long bytes_avail;
	int ret;
	#ifdef _WIN32 // windows
		if ( ( ret = ioctlsocket ( sock_h, FIONREAD, &bytes_avail ) ) == -1 ) {
			netPrintf ( PRINT_ERROR, "Failed at ioctlsocket FIONREAD: Return: %d", ret );
			bytes_avail = -1;
		} 
	#else		
	    int bytes_avail_int;
		if ( ( ret = ioctl ( sock_h, FIONREAD, &bytes_avail_int ) ) == -1 ) {
			netPrintf ( PRINT_ERROR, "Failed at ioctl FIONREAD: Return: %d", ret );
			bytes_avail = -1;
		} else {
			bytes_avail = (unsigned long) bytes_avail_int;
		}
	#endif    
	TRACE_EXIT ( (__func__) );
	return bytes_avail;
}

bool NetworkSystem::CXSocketIsValid ( CX_SOCKET sock )
{
	// SOCKET is unsigned on windows, signed on linux.
	// Windows checks for INVALID_SOCKET, a very large unsigned int.
	// Linux checks for -1 (signed). See Windows & Linux accept function, return value.
	return (sock != CX_INVALID_SOCK);
}

 bool NetworkSystem::netFuncError ( int ret )
{
	return (ret < 0) || (ret==CX_SOCK_ERROR);	
}

inline bool NetworkSystem::CXSocketBlockError ( )
{
	#ifdef __linux__
		return errno == EAGAIN || errno == EWOULDBLOCK;
    #elif _WIN32
		return WSAGetLastError ( ) == WSAEWOULDBLOCK;
	#else
		return false;
	#endif
}

inline str NetworkSystem::CXGetErrorMsg ( int& error_id )
{
	TRACE_ENTER ( (__func__) );
	#ifdef _WIN32 // get error on windows
		if ( error_id == 0 ) {
			error_id = WSAGetLastError ( ); // windows get last error
		}		
		LPTSTR errorText = NULL;
		DWORD flags = FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS;
		DWORD lang_id = MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT );
		FormatMessage ( flags, NULL, error_id, lang_id, (LPSTR)&errorText, 0, NULL );
		str error_str = str ( errorText );
		LocalFree ( errorText );
	#else // get error on linux/android
		if (error_id == 0) {
			error_id = errno; // linux error code
		}
		char buf [ 2048 ];
		char* error_buf = (char*) strerror_r ( error_id, buf, 2048 );
		str error_str = str ( error_buf );
	#endif	
	TRACE_EXIT ( (__func__) );
	return error_str;
}

inline void NetworkSystem::CXSocketUpdateAddr ( int sock_i, bool src )
{
	TRACE_ENTER ( (__func__) );
	NetSock& s = m_socks [ sock_i ];	   
	int optval = 0, ret;
	int ntype;
	ntype = (src) ? s.src.type : s.dest.type;

	// determine IP to use
	netIP ip;	
	switch (ntype) {
	case NTYPE_BROADCAST:		ip = INADDR_BROADCAST;							optval = 1;	break;
	case NTYPE_ANY:					ip = INADDR_ANY;				 						optval = 0;	break;
	case NTYPE_CONNECT:			ip = (src) ? s.src.ip : s.dest.ip;	optval = 0; break;
	};
	if ( s.src.type != STATE_NONE ) {			
		ret = setsockopt ( s.socket, SOL_SOCKET, SO_BROADCAST,  (const char*) &optval, sizeof ( optval ) );							
	}		
	
	// set connection blocking mode
	CXSocketSetBlockMode (s.socket, s.blocking );	

	if ( src ) {
		// set src side of this connection				
		s.src.setAddress ( AF_INET, ip, s.src.port );			// cross-platform		
	} else {
		// set dest side of this connection		
		s.dest.setAddress ( AF_INET, ip, s.dest.port );				// cross-platform						
	}
	TRACE_EXIT ( (__func__) );
}

inline void NetworkSystem::CXSocketClose ( CX_SOCKET sock_h )
{
	TRACE_ENTER ( (__func__) );
	#ifdef _WIN32
		shutdown ( sock_h, SD_BOTH );					
		closesocket ( sock_h );
	#else
		int ret, err = 1;
		CX_SOCKLEN len = sizeof ( err );
		if ( ( ret = getsockopt ( sock_h, SOL_SOCKET, SO_ERROR, (char*) &err, &len ) ) == -1 ) {
			netPrintf ( PRINT_ERROR , "Failed at getsockopt SO_ERROR: Return: %d", ret );
		}
		if ( err ) {
			errno = err;  
		}
		shutdown ( sock_h, SHUT_RDWR );				
		close ( sock_h );
	#endif
	TRACE_EXIT ( (__func__) );
}

inline str NetworkSystem::CXGetIpStr ( netIP ip )
{
	TRACE_ENTER ( (__func__) );
	char ipname [ 1024 ];
	in_addr addr;
	#ifdef _MSC_VER
		addr.S_un.S_addr = ip;
	#else
		addr.s_addr = ip;
	#endif
	sprintf ( ipname, "%s", inet_ntoa ( addr ) );
	TRACE_EXIT ( (__func__) );
	return str ( ipname );
}

//----------------------------------------------------------------------------------------------------------------------
// -> MAIN CODE <-
//----------------------------------------------------------------------------------------------------------------------

NetworkSystem::NetworkSystem ( const char* trace_file_name )
{
	m_hostType = ' ';
	m_hostIp = 0;
	m_readyServices = 0;
	m_userEventCallback = 0;
	m_rcvSelectTimout.tv_sec = 0;
	m_rcvSelectTimout.tv_usec = 1e3;
	m_lastClientConnectCheck.SetTimeNSec ( );

	m_security = NET_SECURITY_PLAIN_TCP;
	m_pathPublicKey = str("");
	m_pathPrivateKey = str("");
	m_pathCertDir = str("");
	m_pathCertFile = str("");
	
	m_printVerbose = false;
	m_printFlow = false;
	m_trace = 0;
	m_check = 0;
	m_indentCount = 0;
	
	m_eventPool = 0x0;			// default heap (not accelerated)

	// default timings
	m_reconnectInterval = 5000;		// 5 seconds
	m_reconnectLimit = 10;				// 10x tries
	m_processInterval = 200;	 	  // 200 msec, packet interval

	TimeX curr_time;
	curr_time.SetTimeNSec();
	m_lastClientConnectCheck = curr_time;
	m_lastNetProcess = curr_time;

	netPrintf(PRINT_VERBOSE, "SERIALIZED HEADER SIZE: %d\n", Event::staticSerializedHeaderSize());
	
	if ( trace_file_name != NULL ) {
		TRACE_SETUP (( trace_file_name ));
	} 
}

void NetworkSystem::sleep_ms ( int time_ms ) 
{    
	TRACE_ENTER ( (__func__) ); 
	#ifdef _WIN32
		Sleep( time_ms );
	#else
		sleep( time_ms );
	#endif	
	//TimeX t;
	//t.SleepNSec ( time_ms * 1e6 );  
	TRACE_EXIT ( (__func__) );
}

unsigned long NetworkSystem::get_read_ready_bytes ( CX_SOCKET sock_h ) 
{   
	TRACE_ENTER ( (__func__) ); 
	unsigned long bytes_avail = CXSocketReadBytes ( sock_h );
	TRACE_EXIT ( (__func__) );
	return bytes_avail;
}

void NetworkSystem::CXSocketMakeNoDelay ( CX_SOCKET sock_h ) 
{
	TRACE_ENTER ( (__func__) );
	int no_delay = 1, ret;
	if ( ( ret = setsockopt ( sock_h, IPPROTO_TCP, TCP_NODELAY, (char *) &no_delay, sizeof ( no_delay ) ) ) < 0 ) {
		netPrintf ( PRINT_ERROR,  "    **** Failed at set no delay: Return: %d", ret );
	}  
	else {
		netPrintf ( PRINT_VERBOSE,"    Call to no delay succeded" );
	} 
	TRACE_EXIT ( (__func__) );
} 

bool NetworkSystem::valid_socket_index ( int i ) 
{
	return i >= 0 && i < m_socks.size ( );
}

//----------------------------------------------------------------------------------------------------------------------
//
// -> CLIENT & SERVER SPECIFIC <-
//
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
// -> OPENSSL SERVER <-
//----------------------------------------------------------------------------------------------------------------------

#ifdef BUILD_OPENSSL
	
str NetworkSystem::netGetErrorStringSSL ( int ret, SSL* ssl ) 
{		 
	TRACE_ENTER ( (__func__) );
	str msg = str ( );
	switch ( SSL_get_error ( ssl, ret ) )
	{
		case SSL_ERROR_NONE:
			msg += "TLS/SSL I/O operation completed. "; 
			break;
		case SSL_ERROR_ZERO_RETURN:  
			msg += "TLS/SSL connection has been closed. "; 
			break;
		case SSL_ERROR_WANT_READ: 
			msg += "Read incomplete; call again later. ";
			break;
		case SSL_ERROR_WANT_WRITE: 
			msg += "Write incomplete; call again later. ";
			break;
		case SSL_ERROR_WANT_CONNECT: 
			msg += "Connect incomplete; call again later. ";
			break;
		case SSL_ERROR_WANT_ACCEPT: 
			msg += "Accept incomplete; call again later. "; 
			break;
		case SSL_ERROR_WANT_X509_LOOKUP:  
			msg += "Operation incomplete; SSL_CTX_set_client_cert_cb() callback asked to be called again later. ";
			break;
		case SSL_ERROR_SYSCALL: 
			msg += "Some I/O error occurred. The OpenSSL error queue is here: "; 
			break;
		case SSL_ERROR_SSL: 
			msg += "An SSL library failure occurred, usually a protocol error. The OpenSSL error queue is here: "; 
			break;
		default: 
			msg += "Unknown error"; 
			break;
	};		 
	char buf[ 2048 ]; // append, SSL error queue 
	unsigned long err;
	while ( ( err = ERR_get_error ( ) ) != 0 ) {
		ERR_error_string ( err, buf );
		msg += str ( buf ) + ". ";
	}	 
	ERR_clear_error ( ); 
	TRACE_EXIT ( (__func__) );
	return msg;
}
	
void NetworkSystem::netFreeSSL ( int sock_i ) 
{
	TRACE_ENTER ( (__func__) );
	NetSock& s = m_socks [ sock_i ];
	if ( s.ssl != 0 ) {
		if ( SSL_shutdown ( s.ssl ) == 0 ) {
			SSL_shutdown ( s.ssl );
		} 
		SSL_free ( s.ssl ); 
		s.ssl = 0;
	}
	if ( s.ctx != 0 ) {
		SSL_CTX_free ( s.ctx );
		s.ctx = 0;
	}
	TRACE_EXIT ( (__func__) );
}

void NetworkSystem::netServerSetupHandshakeSSL ( int sock_i ) 
{
	TRACE_ENTER ( (__func__) );
	char msg[ 2048 ];
	NetSock& s = m_socks [ sock_i ];
	CXSocketMakeNoDelay ( s.socket );
	int ret = 0, exp;
	CXSocketSetBlockMode ( s.socket, false);		// non-blocking	

	s.security |= NET_SECURITY_FAIL; 
	s.state = STATE_FAILED; 
	s.lastStateChange.SetTimeNSec ( );

	if ( ( s.ctx = SSL_CTX_new ( TLS_server_method ( ) ) ) == 0 ) {
		netPrintf ( PRINT_ERROR_HS, "Failed at new ssl ctx" );
		netFreeSSL ( s.socket );
		TRACE_EXIT ( (__func__) );
		return;
	}

	exp = SSL_OP_SINGLE_DH_USE;
	if ( ( ( ret = SSL_CTX_set_options ( s.ctx, exp ) ) & exp ) != exp ) {
		netPrintf ( PRINT_ERROR_HS, "Failed at: set ssl option: Return: %d", ret );
		netFreeSSL ( sock_i );
		TRACE_EXIT ( (__func__) );
		return;
	} else {
		netPrintf ( PRINT_VERBOSE_HS, "Call to set ssl option succeded" );
	}

	if ( ( ret = SSL_CTX_set_default_verify_paths ( s.ctx ) ) <= 0 ) { // Set CA veryify locations for trusted certs
		netPrintf ( PRINT_ERROR_HS, "Default verify paths failed: Return: %d", ret );
	} else {
		netPrintf ( PRINT_VERBOSE_HS, "Call to default verify paths succeded" );
	}
	const char* fmt = "Trusted cert paths. CA file = %s, CA dir = %s";
	netPrintf ( PRINT_VERBOSE_HS, fmt, m_pathCertFile.c_str ( ), m_pathCertDir.c_str ( ) );

	if ( ! m_pathCertFile.empty ( ) || ! m_pathCertDir.empty ( ) ) {
		ret = ret = SSL_CTX_load_verify_locations ( s.ctx, m_pathCertFile.c_str ( ) , m_pathCertDir.c_str ( ) );
		if ( ret <= 0 ) {
			netPrintf ( PRINT_ERROR_HS, "Load verify locations failed on cert file: %s", m_pathCertFile.c_str ( ) );
		} else {
			netPrintf ( PRINT_VERBOSE_HS, "Call to load verify locations succeded" );
		}
	}


	SSL_CTX_set_verify ( s.ctx, SSL_VERIFY_PEER, NULL );

	if ( ( ret = SSL_CTX_use_certificate_file ( s.ctx, m_pathPublicKey.c_str ( ), SSL_FILETYPE_PEM ) ) <= 0 ) {
		netPrintf ( PRINT_ERROR_HS, "Use certificate failed on public key: %s", m_pathPublicKey.c_str ( ) );	
		netFreeSSL ( sock_i ); 
		s.lastStateChange.SetTimeNSec ( );
		TRACE_EXIT ( (__func__) );	
		return;
	} else {
		netPrintf ( PRINT_VERBOSE_HS, "Call to use certificate succeded" );
	}

	if ( ( ret = SSL_CTX_use_PrivateKey_file ( s.ctx, m_pathPrivateKey.c_str ( ), SSL_FILETYPE_PEM ) ) <= 0 ) {			
		netPrintf ( PRINT_ERROR_HS, "Use private key failed on %s", m_pathPrivateKey.c_str ( ) );	
		netFreeSSL ( sock_i ); 
		TRACE_EXIT ( (__func__) );
		return;
	} else {
		netPrintf ( PRINT_VERBOSE_HS, "Call to use private key succeded" );
	}

	s.ssl = SSL_new ( s.ctx );
	long lret = SSL_set_mode ( s.ssl, SSL_MODE_ENABLE_PARTIAL_WRITE );
	if ( lret & SSL_MODE_ENABLE_PARTIAL_WRITE == 0 ) {
		std::cout << "SSL_MODE_ENABLE_PARTIAL_WRITE = 0" << std::endl;
		exit (0);
	}
	
	if ( ( ret = SSL_set_fd ( s.ssl, s.socket ) ) <= 0 ) {
		str msg = netGetErrorStringSSL ( ret, s.ssl );
		netPrintf ( PRINT_ERROR_HS, "Failed at set ssl fd: Return: %d: %s", ret, msg.c_str ( ) );
		netFreeSSL ( sock_i ); 
		TRACE_EXIT ( (__func__) );
		return;
	} else {
		netPrintf ( PRINT_VERBOSE_HS, "Call to set ssl fd succeded" );
	}
	
	s.security &= ~NET_SECURITY_FAIL;
	s.state = STATE_HANDSHAKE;
	s.lastStateChange.SetTimeNSec ( );

	TRACE_EXIT ( (__func__) );
}
	      
void NetworkSystem::netServerAcceptSSL ( int sock_i ) 
{ 
	TRACE_ENTER ( (__func__) );
	NetSock& s = m_socks[ sock_i ];	   
	
	int ret = SSL_accept ( s.ssl ); // SSL accept 
	if ( ret < 0 ) { 
		ret = netNonFatalErrorSSL ( sock_i, ret ); // ret = 2, if want read/write
	}
	
	if ( ret <= 0 ) { // SSL fatal error		
		str msg = netGetErrorStringSSL ( ret, s.ssl );
		netPrintf ( PRINT_ERROR_HS, "SSL_accept failed (1): Return: %d: %s", ret, msg.c_str ( ) );
		netFreeSSL ( sock_i );
		s.security |= NET_SECURITY_FAIL; // Handshake failed
		netManageHandshakeError ( sock_i, "SSL accept failed" );
	} else if (ret == 2) { // SSL non-fatal. Want_read or Want_write
		str msg = netGetErrorStringSSL ( ret, s.ssl );
		netPrintf(PRINT_VERBOSE_HS, "Non-blocking to ssl accept want read/write: %d: %s", ret, msg.c_str ( ) );
		netPrintf(PRINT_VERBOSE_HS, "Ready for safe transfer: %d", SSL_is_init_finished ( s.ssl ) );
	} else if (ret == 1) { // SSL connection complete.
		netPrintf ( PRINT_VERBOSE_HS, "Call to ssl accept succeded" );
		netPrintf ( PRINT_VERBOSE_HS, "Ready for safe transfer: %d", SSL_is_init_finished ( s.ssl ) );
		netServerCompleteConnection ( sock_i ); // Handshake succeeded. Complete connection.
	}
	
	TRACE_EXIT ( (__func__) );
}
	
#endif

//----------------------------------------------------------------------------------------------------------------------
// -> GENERAL SERVER <-
//----------------------------------------------------------------------------------------------------------------------

bool NetworkSystem::netServerStart ( netPort srv_port, int security )
{
//	m_socks.push_back( NetSock() ); printf ( "SOCKS: %d\n", m_socks.size() );   //-- heap corruption test

	TRACE_ENTER ( (__func__) );
	
	m_hostType = 's';
	netPrintf ( PRINT_VERBOSE, "Start Server:" );

	// Get host name (this machine)	
	netIP 	server_ip = getHostIP();
	str	server_name = getHostName();
	
	// Create new listening socket
	NetAddr addr1 ( NTYPE_ANY, server_name, server_ip, srv_port );
	NetAddr addr2 ( NTYPE_BROADCAST, "", 0, srv_port );
	int srv_sock_i = netAddSocket ( NET_SRV, NET_TCP, STATE_START, false, addr1, addr2 ), ret;
	NetSock& s = m_socks[ srv_sock_i ];

  // Set socket options
	CX_SOCKOPT opt = 1;
	ret = setsockopt(s.socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(CX_SOCKOPT));
	if ( netFuncError(ret) ) {	
		netPrintf ( PRINT_ERROR, "Fail to set SO_REUSEADDR. Ret: %d", ret );
		return false;
	}
	if ( security != NET_SECURITY_UNDEF ) {
		m_socks[ srv_sock_i ].security = security;
	}
	
	// Bind & Listen
	ret = netSocketBind ( srv_sock_i );
	if (netFuncError(ret)) {
		netPrintf(PRINT_ERROR, "Server fail to bind listen sock.");
		return false;
	}
	ret = netSocketListen ( srv_sock_i );	
	if (netFuncError(ret)) {
		netPrintf(PRINT_ERROR, "Server fail to listen on sock.");
		return false;
	}

	// Start accept handshake
	m_socks[ srv_sock_i ].state = STATE_HANDSHAKE;

	if ( security == NET_SECURITY_UNDEF ) {
		if ( ( m_security > NET_SECURITY_PLAIN_TCP ) && ( m_security & NET_SECURITY_PLAIN_TCP ) ) {
			netServerStart ( --srv_port, NET_SECURITY_PLAIN_TCP );
		}
	}


	TRACE_EXIT ( (__func__) );

	return true;
}

void NetworkSystem::netServerAcceptClient ( int sock_i )
{
	TRACE_ENTER ( (__func__) );
	/* int srv_sock_svc = netFindSocket ( NET_SRV, NET_TCP, NTYPE_ANY ); // MP: Check that this is OK
	if ( srv_sock_svc == -1 ) {
		netPrintf ( PRINT_ERROR, "Unable to find server listen socket" );
	} */

	str srv_name;
	srv_name = m_socks[ sock_i ].src.name;
	netPort srv_port;
	srv_port = m_socks[ sock_i ].src.port;
	int security_level = m_socks[ sock_i ].security;			// server security level
	netIP cli_ip = 0;
	netPort cli_port = 0;

	// TCP Accept
	CX_SOCKET sock_h;		// New literal socket
	int result = netSocketAccept ( sock_i, sock_h, cli_ip, cli_port );
	if ( result < 0 ) {		
		// Accept error.
		netManageHandshakeError ( sock_i, "connection not accepted" );		
		TRACE_EXIT ( (__func__) );
		return;
	} else if ( result==0 ) {
		// Waiting. Not yet accepted.

	} else if (result > 0) {
		// Add socket for client
		netIP srv_ip = m_hostIp; // Listen/accept on ANY address (0.0.0.0), final connection needs the server IP
		NetAddr addr1 ( NTYPE_CONNECT, srv_name, srv_ip, srv_port );
		NetAddr addr2 ( NTYPE_CONNECT, "", cli_ip, cli_port );
		int cli_sock_i = netAddSocket ( NET_SRV, NET_TCP, STATE_START, false, addr1, addr2 ); // Create new socket

		// Set socket origin & info
		NetSock& s = m_socks[ cli_sock_i ];
		CXSocketSetBlockMode ( sock_h, false);  // non-blocking
		s.security = security_level;						// security level
		s.socket = sock_h;											// assign literal socket
		s.dest.ip = cli_ip;											// assign client IP
		s.dest.port = cli_port;									// assign client port
		s.state = STATE_START;
		s.lastStateChange.SetTimeNSec ( );
		
		// Start of handshake
		if (security_level & NET_SECURITY_OPENSSL) {
			#ifdef BUILD_OPENSSL
				netPrintf(PRINT_VERBOSE, "HANDSHAKE OpenSSL: %s", OPENSSL_VERSION_TEXT); // Openssl version 
			#endif	
		} else {
			netPrintf(PRINT_VERBOSE, "HANDSHAKE TCP/IP");
		}

		// OpenSSL handshake or TCP complete
		if ( s.security & NET_SECURITY_OPENSSL ) {		
			#ifdef BUILD_OPENSSL
				netServerSetupHandshakeSSL ( cli_sock_i );
				if ( s.security & NET_SECURITY_FAIL ) {
					netManageHandshakeError ( sock_i, "SSL handshake failed");
				}
			#endif	
		} else if ( s.security & NET_SECURITY_PLAIN_TCP ) { 		
			netServerCompleteConnection ( cli_sock_i );
		}
	}
	TRACE_EXIT ( (__func__) ); 	
} 
	
void NetworkSystem::netServerCompleteConnection ( int sock_i )
{
	TRACE_ENTER ( (__func__) );
	int srv_sock_svc = netFindSocket ( NET_SRV, NET_TCP, NTYPE_ANY );
	if ( srv_sock_svc == -1 ) {
	   netPrintf ( PRINT_ERROR_HS, "Unable to find server listen socket" );
	}
	netPort srv_port;
	srv_port = m_socks[ srv_sock_svc ].src.port;
	NetSock& s = m_socks [ sock_i ];	

	assert(s.side != NET_CLI);

	// Send first event to client
	Event e; 
	netMakeEvent (e, 'sOkT', 0 );
	e.attachInt64 ( s.dest.ip );	// Client IP
	e.attachInt64 ( s.dest.port );	// Client port assigned by server!
	e.attachInt64 ( m_hostIp );		// Server IP
	e.attachInt64 ( srv_port );		// Server port
	e.attachInt ( sock_i );			// Connection ID (goes back to the client)
	netSend ( e, sock_i );			// Send TCP connected event to client

	netPrintf(PRINT_VERBOSE, "  Sent sOkT event to client." );

	// Send verify event to server
	Event ue ( 120, 'app ', 'sOkT', 0, m_eventPool, "netSrvCompl" ); // Inform the user-app (server) of the event	
	ue.attachInt ( sock_i );
	ue.attachInt ( -1 ); // cli_sock not known
	ue.startRead ( );
	(*m_userEventCallback) ( ue, this ); // Send to application

	// Last step. Set socket as CONNECTED.
	// (we assume the netSend of 'sOkT' succeeded)
	s.state = STATE_CONNECTED;
	s.lastStateChange.SetTimeNSec();

	// Accept succeeded
	bool ssl = (s.security & NET_SECURITY_OPENSSL) == NET_SECURITY_OPENSSL;
	netPrintf(PRINT_VERBOSE, "SUCCESS %s: Server %s:%d, Accepted %s:%d", ssl ? "OpenSLL" : "TCP", getIPStr(m_hostIp).c_str(), s.src.port, getIPStr(s.dest.ip).c_str(), s.dest.port);
	netList();
	
	TRACE_EXIT ( (__func__) );
}

void NetworkSystem::netServerCheckConnectionHandshakes ( ) 
{
	TimeX current_time;
	current_time.SetTimeNSec();
		
	for ( int sock_i = 0; sock_i < (int) m_socks.size ( ); sock_i++ ) { 
		NetSock& s = m_socks[ sock_i ];

		if (current_time.GetElapsedSec(s.lastStateChange) > 1.0) {

			s.lastStateChange = current_time;

			// OpenSSL
			if (s.security & NET_SECURITY_OPENSSL) {
				if (s.state==STATE_HANDSHAKE) {
					netManageHandshakeError(sock_i, "server SSL timeout");
				}
			}

			// TCP/IP protocols
			if ( s.security & NET_SECURITY_PLAIN_TCP ) {
				// Check listening socket
				if ( s.state == STATE_HANDSHAKE && s.src.type == NTYPE_ANY ) {
					//printf ( "Listening: %d\n", sock_i );
					netServerAcceptClient(sock_i);
				}			
			}
		}
		
		
	}
}

void NetworkSystem::netServerProcessIO ( )
{
	TimeX current_time;
	current_time.SetTimeNSec();
	if (current_time.GetElapsedMSec(m_lastNetProcess) < m_processInterval) return;
	m_lastNetProcess = current_time;

	TRACE_ENTER ( (__func__) );
	fd_set sockReadSet;
	fd_set sockWriteSet;
	int rcv_events = netSocketSelect ( &sockReadSet, &sockWriteSet );

	NET_PERF_PUSH ( "findsocks" );

	for ( int sock_i = 0; sock_i < (int) m_socks.size ( ); sock_i++ ) { 
		NetSock& s = m_socks[ sock_i ];

		if ( netSocketIsSelected ( &sockReadSet, sock_i ) ) {
			
			// OpenSSL
			if (s.security & NET_SECURITY_OPENSSL) {				
				#ifdef BUILD_OPENSSL
					if (s.state == STATE_HANDSHAKE && s.src.type == NTYPE_CONNECT) {
						netServerAcceptSSL(sock_i);								// SSL accept, has STATE_HANDSHAKE. (NTYPE_CONNECT because TCP accept completed)
					}
				#endif
			} 			

			// All protocols
			if (s.src.type == NTYPE_CONNECT) {				
				// Receive pending data
				netReceiveData (sock_i);
			}
		}
		if ( netSocketIsSelected ( &sockWriteSet, sock_i ) ) {
			// Send pending data
			netSendResidualEvent ( sock_i );
		}
	}
	NET_PERF_POP ( );
	TRACE_EXIT ( (__func__) );
}


//----------------------------------------------------------------------------------------------------------------------
// -> OPENSSL CLIENT <-
//----------------------------------------------------------------------------------------------------------------------

#ifdef BUILD_OPENSSL
	
void NetworkSystem::netClientSetupHandshakeSSL ( int sock_i ) 
{ 
	char msg[2048];

	TRACE_ENTER ( (__func__) );
	NetSock& s = m_socks[ sock_i ];
	if ( s.ctx != 0 ) {
		netFreeSSL ( sock_i ); 
		netPrintf ( PRINT_VERBOSE_HS, "    Handshake SSL reusing socket. Call to free old context made (1)" );
	}
	
	int ret = 0, exp;
	CXSocketMakeNoDelay ( s.socket );
	CXSocketSetBlockMode ( s.socket, false);
	s.security |= NET_SECURITY_FAIL; // Assume failure until end of this function
	s.state = STATE_FAILED; 
	s.lastStateChange.SetTimeNSec ( );
	
	#if OPENSSL_VERSION_NUMBER < 0x10100000L // Version 1.1
		SSL_load_error_strings ( );	 
		SSL_library_init ( );
	#else // version 3.0+
		OPENSSL_init_ssl ( OPENSSL_INIT_LOAD_SSL_STRINGS, NULL );
	#endif

	//s.bio = BIO_new_socket ( s.socket, BIO_NOCLOSE );

	s.ctx = SSL_CTX_new ( TLS_client_method ( ) );
	if ( ! s.ctx ) {
		netPrintf ( PRINT_ERROR_HS, "Failed at: new ctx" );
		netFreeSSL ( sock_i );
		TRACE_EXIT ( (__func__) );
		return;
	} else {
		netPrintf ( PRINT_VERBOSE_HS, "Call to ctx succeded" );
	}

	// Use TLS 1.2+ only, since we have custom client-server protocols
	SSL_CTX_set_min_proto_version ( s.ctx, TLS1_2_VERSION );
	SSL_CTX_set_max_proto_version ( s.ctx, TLS1_3_VERSION );
	SSL_CTX_set_verify ( s.ctx, SSL_VERIFY_PEER, NULL );

	if ( !SSL_CTX_load_verify_locations( s.ctx, m_pathPublicKey.c_str ( ), NULL ) ) {
		str msg = netGetErrorStringSSL ( ret, s.ssl );
		netPrintf ( PRINT_ERROR_HS, "Load verify failed on public key: %s", msg.c_str ( ) );
		netFreeSSL ( sock_i );
		TRACE_EXIT ( (__func__) );
		return;
	} else {
		netPrintf ( PRINT_VERBOSE_HS, "Call to load verify locations succeded" );
	}		

	s.ssl = SSL_new ( s.ctx );
	long lret = SSL_set_mode ( s.ssl, SSL_MODE_ENABLE_PARTIAL_WRITE );
	if ( lret & SSL_MODE_ENABLE_PARTIAL_WRITE == 0 ) {
		std::cout << "SSL_MODE_ENABLE_PARTIAL_WRITE = 0" << std::endl;
		exit (0);
	}
	
	if ( ! s.ssl ) {
		str msg = netGetErrorStringSSL ( ret, s.ssl );
		netPrintf ( PRINT_ERROR_HS, "Failed at new ssl: %s", msg.c_str ( ) );
		netFreeSSL ( sock_i ); 
		TRACE_EXIT ( (__func__) );
		return;
	} else {
		netPrintf ( PRINT_VERBOSE_HS, "Call to ssl succeded" );
	}	

	if ( ( ret = SSL_set_fd ( s.ssl, s.socket ) ) != 1 ) {
		str msg = netGetErrorStringSSL ( ret, s.ssl );
		netPrintf ( PRINT_ERROR_HS, "Failed at set fd failed: Return: %d: %s", ret, msg.c_str ( ) );
		netFreeSSL ( sock_i );
		TRACE_EXIT ( (__func__) ); 	
		return;
	} else {
		netPrintf ( PRINT_VERBOSE_HS, "Call to ssl set fd succeded" );
	}	

	s.security &= ~NET_SECURITY_FAIL;
	s.state = STATE_HANDSHAKE;
	s.lastStateChange.SetTimeNSec ( );
	TRACE_EXIT ( (__func__) );
}	

void NetworkSystem::netClientConnectSSL ( int sock_i )
{
	TRACE_ENTER ( (__func__) );
	int exp;
	NetSock& s = m_socks[ sock_i ];

	// check if finished (on this socket)
	int finished = SSL_is_init_finished(s.ssl);
	if ( finished ) return;

	// otherwise try SSL connect
	int ret = SSL_connect ( s.ssl );
	if ( ret < 0 ) {
		ret = netNonFatalErrorSSL ( sock_i, ret ); // ret = 2, want read/write
	}

	if ( ret <= 0 ) { // SSL connect error.
		str msg = netGetErrorStringSSL ( ret, s.ssl );
		netPrintf(PRINT_ERROR_HS, "Call to ssl connect failed: Return: %d: %s", ret, msg.c_str ( ) );
		netFreeSSL ( sock_i );
		s.security |= NET_SECURITY_FAIL; // Handshake error
		netManageHandshakeError ( sock_i, "SSL connect failed");

	} else if ( ret == 2 ) { // SSL connect non-fatal. Want_read/write.
		str msg = netGetErrorStringSSL ( ret, s.ssl );
		netPrintf ( PRINT_VERBOSE_HS, "Non-blocking ssl connect want read/write: %d: %s", ret, msg.c_str ( ) );
		netPrintf ( PRINT_VERBOSE_HS, "Ready for safe transfer: %d", SSL_is_init_finished ( s.ssl ) );
	
	} else if ( ret == 1 ) { // SSL connect succeeded.
		netPrintf ( PRINT_VERBOSE_HS, "Call to ssl connect succeded." );
		netPrintf ( PRINT_VERBOSE_HS, "Waiting for sOkT event from server." );
		
		// Note: We DO NOT set state=CONNECTED here yet.
		// Must wait for sOkT event which contains the server srv_sock ID.
	}
		
	TRACE_EXIT ( (__func__) );
}

#endif

//----------------------------------------------------------------------------------------------------------------------
// -> GENERAL CLIENT <-
//----------------------------------------------------------------------------------------------------------------------

void NetworkSystem::netClientStart ( netPort cli_port, str srv_addr )
{

	TRACE_ENTER ( (__func__) );
	
	eventStr_t sys = 'net '; 
	m_hostType = 'c';																	// Network System is running in client mode
	netPrintf ( PRINT_VERBOSE, "Start Client:" );

	struct HELPAPI NetAddr netAddr = NetAddr ( );			// Start a TCP default socket on Client (for reference)
	netAddr.setAddress ( AF_INET, inet_addr( srv_addr.c_str () ), cli_port );	
	netAddSocket ( NET_CLI, NET_TCP, STATE_NONE, false, NetAddr ( NTYPE_ANY, m_hostName, m_hostIp, cli_port ), netAddr );

	TRACE_EXIT ( (__func__) );
}


netIP NetworkSystem::netResolveServerIP ( str srv_name, netPort srv_port )
{
	netIP srv_ip;

	int dots = 0; // Check server name for dots
	for (int n = 0; n < srv_name.length(); n++) {
		if (srv_name.at(n) == '.') dots++;
	}
	if (srv_name.compare("localhost") == 0) { // Derver is localhost
		srv_ip = m_hostIp;
	}
	else if (dots == 3) { // Three dots, translate srv_name to literal IP		
		srv_ip = getStrToIP(srv_name);
	}
	else { // Fewer dots, lookup host name resolve the server address and port
		addrinfo* pAddrInfo;
		char portname[64];
		sprintf(portname, "%d", srv_port);
		int result = getaddrinfo(srv_name.c_str(), portname, 0, &pAddrInfo);
		if (result != 0) {
			netPrintf(PRINT_ERROR_HS, "Unable to resolve server: %s: Return: %d", srv_name.c_str(), result);
			TRACE_EXIT((__func__));
			return -1;
		}
		char ipstr[INET_ADDRSTRLEN];
		for (addrinfo* p = pAddrInfo; p != NULL; p = p->ai_next) { // Translate addrinfo to IP string
			struct in_addr* addr;
			if (p->ai_family == AF_INET) {
				struct sockaddr_in* ipv = (struct sockaddr_in*)p->ai_addr;
				addr = &(ipv->sin_addr);
			}
			else {
				struct sockaddr_in6* ipv6 = (struct sockaddr_in6*)p->ai_addr;
				addr = (struct in_addr*)&(ipv6->sin6_addr);
			}
			inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);
		}
		srv_ip = getStrToIP(ipstr);
	}

	return srv_ip;
}

int NetworkSystem::netFindOrCreateSocket (str srv_name, netPort srv_port, netIP srv_ip, bool block )
{
	int cli_sock_i;	
	str cli_name = "";
	netIP cli_ip = m_hostIp;

	netPort cli_port = srv_port + 1;			// client port

	// Find source socket service by type
	int cli_sock_svc_i = netFindSocket(NET_CLI, NET_TCP, NTYPE_ANY); // Find a local TCP socket service
	if (cli_sock_svc_i != -1) {
		cli_name = m_socks[cli_sock_svc_i].src.name;				
	}

	// Find socket to specific server & port (only one per client)
	NetAddr srv_addr = NetAddr(NTYPE_CONNECT, srv_name, srv_ip, srv_port); 
	cli_sock_i = netFindSocket(NET_CLI, NET_TCP, STATE_NONE, srv_addr);

	if (cli_sock_i == NET_ERR) {
		// Add new socket
		NetAddr cli_addr = NetAddr(NTYPE_CONNECT, cli_name, cli_ip, cli_port);
		cli_sock_i = netAddSocket(NET_CLI, NET_TCP, STATE_NONE, block, cli_addr, srv_addr);
		if (cli_sock_i == NET_ERR) {
			netPrintf(PRINT_ERROR_HS, "Unable to add socket");
			TRACE_EXIT((__func__));
			return -1;
		}
	}

	// STATE_NONE - indicates a client socket ready to connect (triggers netClientConnectToServer and STATE_START)

	return cli_sock_i;
}	


int NetworkSystem::netClientConnectToServer ( str srv_name, netPort srv_port, bool block, int cli_sock_i )
{
	TRACE_ENTER ( (__func__) );
	
	netIP srv_ip;
	int ret;

	// Reuse or create a client socket
	if ( ! valid_socket_index( cli_sock_i ) ) {
		
		// Resolve server name/port to server IP
		srv_ip = netResolveServerIP ( srv_name, srv_port );
		
		// Create new socket if needed
		cli_sock_i = netFindOrCreateSocket ( srv_name, srv_port, srv_ip, block );
	}

	// Return if already connected (likely waiting for sOkT from server)
	NetSock& s = m_socks[ cli_sock_i ];
	if ( s.state == STATE_CONNECTED ) {
		TRACE_EXIT((__func__));
		return cli_sock_i;
	}

	// Set socket opts
	CX_SOCKOPT opt = 1;	
	s.srvAddr = srv_name;
	s.srvPort = srv_port; 
	ret = setsockopt(s.socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(CX_SOCKOPT) );
	if ( netFuncError(ret) ) {
		netPrintf ( PRINT_ERROR_HS, "Failed to set SO_REUSEADDR: Return: %d", ret );
	}

	// Start of handshake
	if (s.security & NET_SECURITY_OPENSSL) {
		#ifdef BUILD_OPENSSL
			netPrintf(PRINT_VERBOSE, "HANDSHAKE OpenSSL: %s", OPENSSL_VERSION_TEXT);
		#endif	
	} else {
		netPrintf(PRINT_VERBOSE, "HANDSHAKE TCP/IP");
	}	
	s.state = STATE_START;					// no longer in reuse STATE_NONE (stops triggering of reconnect)

	// TCP connect here
	ret = netSocketConnect ( cli_sock_i );
	if ( ret < 0 ) {
		// Connect error.
		netManageHandshakeError ( cli_sock_i, "TCP handshake failed" );		
	
	} else if (ret == 0) {
		// Waiting to connect. Start TCP handshake. 



	} else if (ret > 0 ) {

		// TCP connected ok.
		if (s.security & NET_SECURITY_OPENSSL) {
			#ifdef BUILD_OPENSSL
				netClientSetupHandshakeSSL(cli_sock_i);			// state may change to STATE_HANDSHAKE
				if (s.security & NET_SECURITY_FAIL) {
					netManageHandshakeError(cli_sock_i, "SSL handshake failed");
				}
			#endif
		} else if (s.security & NET_SECURITY_PLAIN_TCP) {
			netClientCompleteConnection(cli_sock_i);
		}
	}

	TRACE_EXIT ( (__func__) );
	return cli_sock_i; // Return socket for this connection
}

void NetworkSystem::netClientHandshake ( int sock_i )
{
	// TCP/IP non-blocking handshake
  // - connect was called. we run select to wait for server.
	// - there must be a delay before selecting after connect. this is implemented by the caller.

	NetSock* s = &m_socks[sock_i];
	fd_set writefds;
	FD_ZERO (&writefds);
	FD_SET (s->socket, &writefds);
	timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 500;
  
	int ret = select(s->socket + 1, NULL, &writefds, NULL, &timeout);
	if (ret > 0 && FD_ISSET( s->socket, &writefds)) {
		// connection complete				
		m_socks[ sock_i ].state = STATE_CONNECTED;
		netPrintf(PRINT_VERBOSE_HS, "Client connect complete. \n");		
	} else {
		// non-fatal. waiting for complete.
		netPrintf(PRINT_VERBOSE_HS, "Client waiting for server. \n");		
	}
}


void NetworkSystem::netClientCheckConnectionHandshakes ( )
{
	TRACE_ENTER ( (__func__) );
	TimeX current_time;
	current_time.SetTimeNSec ( );	
	if ( current_time.GetElapsedMSec ( m_lastClientConnectCheck ) > m_reconnectInterval ) {
		m_lastClientConnectCheck.SetTimeNSec ( );

		for ( int sock_i = 1; sock_i < (int) m_socks.size ( ); sock_i++ ) {
			NetSock& s = m_socks[ sock_i ];
			
			if ( s.security & NET_SECURITY_OPENSSL) {				
				// OpenSSL - retry connect during handshake
				#ifdef BUILD_OPENSSL					
				if ( s.state == STATE_HANDSHAKE ) {
					netClientConnectSSL(sock_i);	// This call is MORE important than the others
				}
				#endif
			} else if (s.security & NET_SECURITY_PLAIN_TCP) {
				// TCP/IP - retry connect during handshake
				if (s.state == STATE_HANDSHAKE) {
					netClientHandshake ( sock_i );
				}
			}
			
			// All protocols
			if (s.state == STATE_START || s.state == STATE_HANDSHAKE ) {
				// Check for timeout
				TimeX current_time;
				current_time.SetTimeNSec();
				if (current_time.GetElapsedSec(s.lastStateChange) > 5.0) {
					netManageHandshakeError(sock_i, "client timed out");
				}
			} else if (s.state == STATE_NONE && s.reconnectBudget > 0 ) {	
				// Auto-reconnect if desired
				s.reconnectBudget--;
				netClientConnectToServer ( s.srvAddr, s.srvPort, false, sock_i ); // If disconnected, try and reconnect
			}			
		}
	}

	TRACE_EXIT ( (__func__) );
}
	
void NetworkSystem::netClientProcessIO ( )
{
	TimeX current_time;
	current_time.SetTimeNSec();
	if (current_time.GetElapsedMSec(m_lastNetProcess) < m_processInterval) return;
	m_lastNetProcess = current_time;

	TRACE_ENTER ( (__func__) );
	fd_set sockReadSet;
	fd_set sockWriteSet;
	int rcv_events = netSocketSelect ( &sockReadSet, &sockWriteSet );
	NET_PERF_PUSH ( "findsocks" );
	
	for ( int sock_i = 0; sock_i < (int) m_socks.size ( ); sock_i++ ) { 		
		if ( netSocketIsSelected ( &sockReadSet, sock_i ) ) {			
			// Receive any pending data
			netReceiveData(sock_i);
		}
		if ( netSocketIsSelected ( &sockWriteSet, sock_i ) ) {
			// Send any pending data
			netSendResidualEvent( sock_i );
		}
	}	
	NET_PERF_POP ( );
	TRACE_EXIT ( (__func__) );
}

//----------------------------------------------------------------------------------------------------------------------
//
// -> CLIENT & SERVER COMMON FUNCTIONS <-
//
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
// -> CLIENT & SERVER <-
//----------------------------------------------------------------------------------------------------------------------

bool NetworkSystem::netIsConnectComplete ( int sock_i )
{
	TRACE_ENTER ( (__func__) );
	bool outcome = valid_socket_index(sock_i) && m_socks[ sock_i ].state == STATE_CONNECTED;
	TRACE_EXIT ( (__func__) );
	return outcome;
}

int NetworkSystem::netCloseAll ( )
{
	TRACE_ENTER ( (__func__) );
	for ( int n = 0; n < m_socks.size ( ); n++ ) {
		netCloseConnection ( n );
	}
	netList ( );
	TRACE_EXIT ( (__func__) );
	return 1;
}

int NetworkSystem::netCloseConnection ( int sock_i )
{
	TRACE_ENTER ( (__func__) );
	if ( sock_i < 0 || sock_i >= m_socks.size ( ) ) {
		TRACE_EXIT ( (__func__) );
		return 0;
	}
	NetSock& s = m_socks[ sock_i ];
	if ( s.side == NET_CLI ) {		

		Event ce;
		netMakeEvent ( ce, 'cEXT', 'net ' );
		ce.attachUInt ( m_socks [ sock_i ].dest.sock );
		ce.attachUInt ( sock_i ); 
		netSend ( ce );
		netProcessQueue ( ); 	

	} else { 

		int dest_sock;
		dest_sock = s.dest.sock;
		Event se;
		netMakeEvent ( se, 'sEXT', 'net ' );
		se.attachUInt ( s.dest.sock ); 
		se.attachUInt ( sock_i ); 
		netSend ( se );
		netProcessQueue ( );		
	}
	netManageTransmitError ( sock_i, "closed" ); // Terminate local socket	 

	TRACE_EXIT ( (__func__) );
	return 1;
}

#ifdef BUILD_OPENSSL

int NetworkSystem::netNonFatalErrorSSL ( int sock, int ret ) 
{
	TRACE_ENTER ( (__func__) );
	int err = SSL_get_error ( m_socks [ sock ].ssl, ret ), code;
	if ( err == SSL_ERROR_WANT_READ ) {
		TRACE_EXIT ( (__func__) );
		return 2;	// ret value to use for non-fatal want read/write
	} else if ( err == SSL_ERROR_WANT_WRITE ) {
		TRACE_EXIT ( (__func__) );
		return 2;	// ret value to use for non-fatal want read/write
	}
	TRACE_EXIT ( (__func__) );
	return ret;		// pass-thru other ret error values
}

#endif

//----------------------------------------------------------------------------------------------------------------------
// -> CORE CODE <-
//----------------------------------------------------------------------------------------------------------------------


void NetworkSystem::netClientCompleteConnection(int sock_i)
{
	// We do not mark the client connection fully complete until client has 
	// received 'sOkT' event. See netProcessEvents.
	//
	m_socks[ sock_i ].state = STATE_HANDSHAKE;
}

void NetworkSystem::netProcessEvents ( Event& e )
{
	TRACE_ENTER ( (__func__) );
	switch ( e.getName ( ) ) {
		case 'sOkT': {
			// Server sent OK for this client. Connection complete.			
			netIP cli_ip = e.getInt64 ( );		// Client IP completed
			netPort cli_port = e.getInt64 ( );  // Client port
			netIP srv_ip = e.getInt64 ( );		// Server IP
			int srv_port = e.getInt64 ( );		// Server port
			int srv_sock = e.getInt ( );		// Server sock which maintains this client

			int cli_sock = e.getSrcSock();		// Client sock which received accept (srcsock, not in payload)
	
			// Verify client and server IPs
			netIP srv_ip_chk = e.getSrcIP ( ); // source IP from the socket event came on
			netIP cli_ip_chk = m_socks[ cli_sock ].src.ip; // original client IP

			// Mark the client socket as CONNECTED.
			// Update with server socket & client port.
			m_socks[cli_sock].state = STATE_CONNECTED; // mark connected
			m_socks[cli_sock].lastStateChange.SetTimeNSec();
			m_socks[cli_sock].dest.sock = srv_sock; // assign server socket
			m_socks[cli_sock].src.port = cli_port; // assign client port from server			

			// Connection complete
			bool ssl = m_socks[cli_sock].security & NET_SECURITY_OPENSSL;
			netPrintf(PRINT_VERBOSE, "SUCCESS %s. Client %s:%d (sock %d), To Server: %s:%d (sock %d)", ssl ? "OpenSSL" : "TCP", getIPStr(cli_ip).c_str(), cli_port, cli_sock, getIPStr(srv_ip).c_str(), srv_port, srv_sock);
			netList();

			Event ce ( 120, 'app ', 'sOkT', 0, m_eventPool, "netProc" ); // Inform the user-app (client) of the event
			ce.attachInt ( srv_sock );
			ce.attachInt ( cli_sock );		
			ce.startRead ( );

			(*m_userEventCallback) ( ce, this ); // Send to application			

			break;
		} 
		case 'cEXT': { 
			// Client has exited from this server.
			int local_sock_i = e.getUInt ( ); // Socket to close
			int remote_sock = e.getUInt ( ); // Remote socket
			if ( valid_socket_index(local_sock_i)) {	
				netIP cli_ip;
				cli_ip = m_socks[ local_sock_i ].dest.ip;
				netPrintf ( PRINT_VERBOSE_HS, "SRV: Client %s closed OK", getIPStr ( cli_ip ).c_str ( ) );
				netManageTransmitError ( local_sock_i, "client closed ok");
			}
			netList ( );
			break;
		}
	}
	TRACE_EXIT ( (__func__) );
}

void NetworkSystem::netInitialize ( )
{
	TRACE_ENTER ( (__func__) );
	m_check = 0;
	netPrintf ( PRINT_VERBOSE, "Network Initialize" );
	m_eventPool = 0x0; // No event pooling
	netStartSocketAPI ( ); 
	netSetHostname ( ); 
	TRACE_EXIT ( (__func__) );
}

int NetworkSystem::netAddSocket ( int side, int mode, int state, bool block, NetAddr src, NetAddr dest )
{
	TRACE_ENTER ( (__func__) );

	NetSock s;
	s.side = side;
	s.mode = mode;
	s.state = state;
	s.lastStateChange.SetTimeNSec ( );
	s.src = src;
	s.dest = dest;
	s.socket = 0;
	s.timeout.tv_sec = 0; 
	s.timeout.tv_usec = 0;
	s.blocking = block;
	s.broadcast = 0;
	s.security = m_security; 
	s.reconnectBudget = s.reconnectLimit = m_reconnectLimit;  

	#ifdef BUILD_OPENSSL
		s.ctx = 0;
		s.ssl = 0;
		s.bio = 0;
	#endif

	// inital packet buf
	s.pktMax = 8192;			// fixed size
	s.pktBuf = (char*) malloc ( s.pktMax );		
	s.pktPtr = s.pktBuf;
	s.pktLen = 0;
	s.pktCounter = 0;

	// initial rx buf
	s.rxMax = 8192;			// expandable
	s.rxBuf = (char*) malloc(s.rxMax);
	s.rxPtr = s.rxBuf;
	s.rxLen = 0;

	// initial tx buf
	s.txMax = 8192;			// expandable
	s.txBuf = (char*) malloc(s.txMax);
	s.txPtr = s.txBuf;
	s.txLen = 0;	
	s.txPktSize = 0;

	// socket recv event
	s.event = new Event ( 'net ', 'Psox' );

	int n = m_socks.size ( );
	m_socks.push_back ( s );

	netSocketCreate ( n );
	
	TRACE_EXIT ( (__func__) );
	return n;
}

void NetworkSystem::netSocketReuse ( int sock_i )
{
	// Several steps must occur to allow socket reuse.
	
	// retain the srv & dest IP and ports
	NetSock& s = m_socks[sock_i];

	// indicate reuse (ready to restart)
	s.state = STATE_NONE;			

	// reset reconnect timer
	s.lastStateChange.SetTimeNSec();			

	// free old SSL context
	#ifdef BUILD_OPENSSL
		if (s.ctx != 0) {
			netFreeSSL( sock_i );
			netPrintf(PRINT_VERBOSE, "    Freeing old socket context (2)");
		}
	#endif	

	// close the socket
	CXSocketClose( s.socket );
	s.socket = 0;
	
	// create a new socket with same addr
	netSocketCreate ( sock_i );

	// reset socket buffers
	netResetBuf ( s.rxBuf, s.rxPtr, s.rxLen );
	netResetBuf ( s.txBuf, s.txPtr, s.txLen );

	// note: don't try and reconnect here. let the reconnect counter do it.
}

void NetworkSystem::netResetBufs()
{
	// reset all socket buffers
	for (int i = 0; i < m_socks.size(); i++) {
		if (m_socks[i].state != STATE_TERMINATED ) {
			NetSock& s = m_socks[i];
			netResetBuf(s.rxBuf, s.rxPtr, s.rxLen);
			netResetBuf(s.txBuf, s.txPtr, s.txLen);
		}
	}
}

int NetworkSystem::netManageHandshakeError ( int sock_i, std::string reason ) 
{
	TRACE_ENTER ( (__func__) );
	NetSock& s = m_socks[ sock_i ];
	int security_fail = s.security;		// record the security levels at failure
	int outcome = 0;
	
	netPrintf ( PRINT_VERBOSE, "    Handshake Error: %s. ", reason.c_str() );

  // Fallback only if: client side, and reconnect budget depleted, and fallback to lower security level allowed.
	bool fallback_allowed = (s.security & NET_SECURITY_OPENSSL) && (s.security & NET_SECURITY_PLAIN_TCP);
	if ( s.reconnectBudget == 0 && s.side == NET_CLI && fallback_allowed) {

		// Client try fallback to plain TCP
		s.security = NET_SECURITY_PLAIN_TCP;
		s.srvPort -= 1;									// TCP ports
		s.dest.port -= 1;
		s.state = STATE_NONE;						// indicate ready to restart
		s.reconnectBudget = s.reconnectLimit;		// reset the reconnect budget for TCP

		netSocketReuse( sock_i );						// reuse socket. don't try and reconnect here. 

		netPrintf(PRINT_VERBOSE, "    Fallback to TCP. Trying server=%s:%d", s.srvAddr.c_str(), s.srvPort );				
	
  }	else {
		bool force_del = (s.side != NET_CLI);		// only retain client sockets
		outcome = netDeleteSocket ( sock_i, force_del );
	}
	// Handshake failed
	bool ssl = (security_fail & NET_SECURITY_OPENSSL);
	netPrintf( PRINT_VERBOSE, "FAILED %s.", ssl ? "OpenSSL" : "TCP" );

	TRACE_EXIT ( (__func__) );
	return outcome;
}


int NetworkSystem::netManageTransmitError ( int sock_i, std::string reason, int force )
{
	TRACE_ENTER ( (__func__) );
	NetSock& s = m_socks[ sock_i ];
	int outcome = 0;

	// Check if error occurred during handshake or start connect
	if (s.state != STATE_CONNECTED) {		
		outcome = netManageHandshakeError(sock_i, reason);
		TRACE_EXIT((__func__));
		return outcome;
	}
	
	if (m_printVerbose) {	
		// display as error, but managed, so only when verbose
		netPrintf( PRINT_ERROR, "    Manage error: %s", reason.c_str());
	}

	// Error during transmission
	if ( m_hostType == 'c' && s.reconnectBudget > 0 ) {
		
		netSocketReuse( sock_i );			// reuse socket. don't try and reconnect here. 		

	} else {
		
		outcome = netDeleteSocket ( sock_i, 1 );
	}
	TRACE_EXIT ( (__func__) );
	return outcome;
}

// Terminate Socket
// Note: This does not erase the socket from std::vector because we don't want to
// shift around the other socket IDs. Instead it disables the socket ID, making it available
// to another client later. Only the very last socket could be actually removed from list.


int NetworkSystem::netDeleteSocket ( int sock_i, int force )
{
	TRACE_ENTER ( (__func__) );
	if ( !valid_socket_index(sock_i) ) {	
		TRACE_EXIT ( (__func__) );
		return 0;
	}
	NetSock& s = m_socks[ sock_i ];	
	s.lastStateChange.SetTimeNSec();
	bool wasConnected = (s.state == STATE_CONNECTED);

	// Reuse or delete the socket
	//
	if ( s.side==NET_CLI && s.state != STATE_CONNECTED && force == 0 ) {
		
		// retain socket (client only)
		netPrintf(PRINT_VERBOSE_HS, "Retained socket: %d", sock_i);
		s.state = STATE_NONE;					// ready to start again (but do not start here)

} else { 

		// terminate socket
		netPrintf(PRINT_VERBOSE_HS, "Terminating socket: %d", sock_i);
		CXSocketClose ( s.socket );
		s.state = STATE_TERMINATED;
		// remove sockets at end of list
		// --- FOR NOW, THIS IS NECESSARY ON CLIENT (which may have only 1 socket),
		// BUT IN FUTURE CLIENTS SHOULD BE ABLE TO HAVE ANY NUMBER OF PREVIOUSLY TERMINATED SOCKETS
		if ( m_socks.size ( ) > 0 ) {
			while ( m_socks[ m_socks.size() -1 ].state == STATE_TERMINATED ) {
				m_socks.erase ( m_socks.end ( ) -1 );
			}
		}
	}
	
	// Inform app of socket removal
	if ( wasConnected ) {
		if ( m_hostType == 's' ) {
			Event se (120, 'app ', 'cFIN', 0, m_eventPool );
			se.attachInt ( sock_i );
			se.startRead ( );
			(*m_userEventCallback) (se, this); // Send to application
		} else {
			Event ce (120, 'app ', 'sFIN', 0, m_eventPool );
			ce.attachInt ( sock_i );
			ce.startRead ( );
			(*m_userEventCallback) (ce, this); // Send to application
		}
	}

	TRACE_EXIT ( (__func__) );
	return 1;
}

// Handle incoming events, Client or Server
// this function dispatches either to the application-level callback,
// or the the network system event handler based on the Event system target
int NetworkSystem::netEventCallback ( Event& e )
{
	TRACE_ENTER ( (__func__) );
	eventStr_t	sys = e.getTarget ();				// target system

	// Application should handle event
	if ( sys != 'net ' ) {								// not intended for network system
		if ( m_userEventCallback != 0x0 ) {				// pass user events to application
			TRACE_EXIT ( (__func__) );
			return (*m_userEventCallback) ( e, this );
		}
	}
	// Network system should handle event
	netProcessEvents ( e );
	TRACE_EXIT ( (__func__) );
	return 0; // only return >0 on user event completion
}

void NetworkSystem::netReportError ( int result )
{
	TRACE_ENTER ( (__func__) );
	// create a network error event and set it to the user
	Event e;
	netMakeEvent ( e, 'nerr', 'net ' );
	e.attachInt ( result );
	e.startRead();
	(*m_userEventCallback) ( e, this );
	TRACE_EXIT ( (__func__) );
}

//----------------------------------------------------------------------------------------------------------------------
// -> PRIMARY ENTRY POINT <-
//----------------------------------------------------------------------------------------------------------------------

int NetworkSystem::netProcessQueue ( void )
{
	// TRACE_ENTER ( (__func__) );	
	if ( m_socks.size ( ) > 0 ) {
		if ( m_hostType == 'c' ) {
			netClientCheckConnectionHandshakes ( );
			netClientProcessIO ( );
		} else {
			netServerCheckConnectionHandshakes ( );
			netServerProcessIO ( );
		}
	}
	int iOk = 0; // Handle incoming events on queue
	Event* e;
	
	while ( m_eventQueue.getSize ( ) > 0 ) {

		m_eventQueue.PopFront ( e );
		iOk += netEventCallback ( *e );		// count each user event handled ok				
		
		e->consume ();
		delete e;
	}
	// TRACE_EXIT ( (__func__) );
	return iOk;
}

// Expand any buffer (tx, rx, or pkt)
//
void NetworkSystem::netExpandBuf (char*& buf, char*& ptr, int& max, int& len, int new_max)
{
	assert ( len <= max );
	if (new_max > max) {
		char* new_buf = (char*) malloc ( new_max );
		memcpy ( new_buf, buf, len );
		free ( buf );
		buf = new_buf;
		ptr = buf + len;
		max = new_max;
	}
}

void NetworkSystem::netResetBuf ( char*& buf, char*& ptr, int& len )
{
	len = 0;
	ptr = buf;
}

xlong NetworkSystem::ComputeChecksum ( char* buf, int len )
{
	xlong sum = 0;
	for (int n=0; n < len; n++) {
		sum = sum + xlong(* (unsigned char*) buf);
		buf++;
	}	
	return sum;	
}

void NetworkSystem::netDeserializeEvents(int sock_i)
{
	TRACE_ENTER ( (__func__) );
	NetSock& s = m_socks[ sock_i ];
	int header_sz = Event::staticSerializedHeaderSize();

	// Consumer pattern:
	// - retrieve entire event from stream directly if possible (for performance)
	// - if not, consume input stream into temp buffer	
	// - build complete event and clear temp when complete	
	// Notes:
	//  bufferLen = length of data remaining on this call (decreases as consumed)
	//  eventLen  = total length of expected event (including header) 
	//  recvLen   = partial length currently received (over multiple calls to this func), when 0 = start new event
	//  recvMax   = maximum length of temp buffer, may dynamic resize for large events	

	netPrintf ( PRINT_FLOW, "PKT #%d, %d bytes.", s.pktCounter, s.pktLen );	
	
	s.pktCounter++;
	s.pktPtr = s.pktBuf;			// recv packet itself is atomic, start at beginning

	xlong chksum = 0;

	while ( s.pktLen > 0 ) {
		if ( s.rxLen == 0 && s.pktLen >= header_sz ) { // Check for new or partial event
			
			// Start of new event, retrieve total event length from encoded header
			s.eventLen = *((int*)(s.pktPtr + Event::staticOffsetLenInfo())) + Event::staticSerializedHeaderSize ( );

			if ( s.pktLen >= s.eventLen ) {
				// Create event; no name/target. will be set during deserialize		
				eventStr_t name = *(eventStr_t*) (s.pktPtr + Event::staticOffsetLenInfo() + 4);
				new_event ( *s.event, s.eventLen - Event::staticSerializedHeaderSize ( ), 'app ', name, 0, m_eventPool, "netRecv" );
				s.event->rescope ( "nets" );									// belongs to network now
				s.event->setSrcSock ( sock_i );								// tag event /w socket
				s.event->setSrcIP( m_socks[ sock_i ].src.ip );				// recover sender address from socket
				
				// Deserialize directly from input buffer (for performance)				
				s.event->deserialize ( s.pktPtr, s.eventLen );		// deserialize					
				netQueueEvent ( *s.event );								// queue event (consumed later)				

				// Checksum [debugging] - determine if send/recv buffers (events) match byte-for-byte
				if (m_printFlow) {
					chksum = ComputeChecksum(s.pktPtr, s.eventLen);
				}
				netPrintf ( PRINT_FLOW, "RX %d/%d bytes (rxLen=%d), %s --> RECV  chksum=%lld", s.pktLen, s.eventLen, s.rxLen, s.event->getNameStr ( ).c_str(), chksum );	

				s.pktLen -= s.eventLen;								// consume event size in bytes
				s.pktPtr += s.eventLen;
				s.eventLen = 0;											// reset event size (recvLen remains 0)
			}
			else { 
				// Store partial event in recv buffer					
				netExpandBuf (s.rxBuf, s.rxPtr, s.rxMax, s.rxLen, s.rxLen + s.pktLen );				
				memcpy ( s.rxPtr, s.pktPtr, s.pktLen );		// transfer into recv buffer
				s.rxPtr += s.pktLen;											// advance recv buffer
				s.rxLen += s.pktLen;
				s.pktPtr += s.pktLen;											// consume remaining buffer len bytes
				netPrintf(PRINT_FLOW, "RX %d/%d bytes (rxLen=%d), %s", s.pktLen, s.eventLen, s.rxLen, s.event->getNameStr().c_str());
				s.pktLen = 0;
			}

		} else { 
			// Continuation of event. Store additional data in recv buffer						
			netExpandBuf(s.rxBuf, s.rxPtr, s.rxMax, s.rxLen, s.rxLen + s.pktLen);
			memcpy ( s.rxPtr, s.pktPtr, s.pktLen );			// transfer into recv buffer
			s.rxPtr += s.pktLen;								// advance recv buffer
			s.rxLen += s.pktLen;			
			s.pktPtr += s.pktLen;								// consume remaining buffer len bytes
			netPrintf(PRINT_FLOW, "RX %d/%d bytes (rxLen=%d), %s", s.pktLen, s.eventLen, s.rxLen, s.event->getNameStr().c_str());
			s.pktLen = 0;

			if (s.rxLen >= header_sz && s.eventLen == 0 )  {
				s.eventLen = *((int*)(s.rxBuf + Event::staticOffsetLenInfo())) + Event::staticSerializedHeaderSize();				
			}
		}

		// Check for possibly multiple complete events on recv buffer
		while ( s.rxLen >= s.eventLen && s.eventLen > 0 ) {

			// Create event; no name/target. will be set during deserialize	
			eventStr_t name = *(eventStr_t*) (s.pktPtr + Event::staticOffsetLenInfo() + 4);
			new_event ( *s.event, s.eventLen - Event::staticSerializedHeaderSize ( ), 'net ', name, 0, m_eventPool, "netRecv" );
			s.event->rescope ( "nets" );						// belongs to network now
			s.event->setSrcSock ( sock_i );					// tag event /w socket
			s.event->setSrcIP ( m_socks[ sock_i ].src.ip );		// recover sender address from socket
				
			// Deserialize event from recv buf			
			s.event->deserialize ( s.rxBuf, s.eventLen );	// deserialize			
			netQueueEvent ( *s.event );							// queue event (consumed later)			
			
			// Checksum [debugging] - determine if send/recv buffers (events) match byte-for-byte
			if ( m_printFlow ) {
				chksum = ComputeChecksum(s.rxBuf, s.eventLen);
			}
			// Reduce recv buffer
			s.rxLen -= s.eventLen;									// consume event bytes in recv buffer
			memmove ( s.rxBuf, s.rxBuf + s.eventLen, s.rxLen);		// must us an overlap-safe memory copy (not memcpy), to shift the data back
			s.rxPtr = s.rxBuf + s.rxLen;						// reset to beginning of recv						

			netPrintf(PRINT_FLOW, "RX %d/%d bytes (rxLen=%d), %s --> RECV  chksum=%lld", s.eventLen, s.eventLen, s.rxLen, s.event->getNameStr().c_str(), chksum );
			s.eventLen = 0;													// reset event len

			// Check for additional event(s)
			if (s.rxLen > header_sz) {
				s.eventLen = *((int*)(s.rxBuf + Event::staticOffsetLenInfo())) + Event::staticSerializedHeaderSize();	
			}
		}
	}
	TRACE_EXIT ( (__func__) );
} 

// -- Original deserialize func (NOT CORRECT)
//
/* void NetworkSystem::netDeserializeEvents(int sock_i)
{
	m_packetPtr = &m_packetBuf[0];
	bool bDeserial;

	while (m_packetLen > 0) {
		if (m_event.isEmpty()) { // Check the type of incoming socket
			if (m_socks[sock_i].blocking) {
				// Blocking socket. NOT an Event socket. Attach arbitrary data onto a new event.
				m_eventLen = m_packetLen;
				m_event = new_event(m_eventLen + 128, 'app ', 'HTTP', 0, m_eventPool);
				m_event.rescope("nets");
				m_event.attachInt(m_packetLen); // attachInt+Buf = attachStr
				m_event.attachBuf(m_packetPtr, m_packetLen);
				m_dataLen = m_event.mDataLen;
			}
			else {
				// Non-blocking socket. Receive a complete Event.
				// directly read length-of-event info from incoming data (m_dataLen value)
				m_dataLen = *((int*)(m_packetPtr + Event::staticOffsetLenInfo()));

				// compute total event length, including header
				m_eventLen = m_dataLen + Event::staticSerializedHeaderSize();

				// Event is allocated with no name/target as this will be set during deserialize
				m_event = new_event(m_dataLen, 0, 0, 0, m_eventPool);
				m_event.rescope("nets"); // Belongs to network now

				// check for serialize issue
				if (m_packeten < Event::staticSerializedHeaderSize()) {
					netPrintf(PRINT_ERROR, "Serialize issue. Buffer len %d less than event header %d. CORRUPT AFTER THIS POINT!", m_packetLen, Event::staticSerializedHeaderSize() );
				}

				// Deserialize of actual buffer length (EventLen or packetLen)
				m_event.deserialize(m_packetPtr, imin(m_eventLen, m_packetLen)); // Deserialize header				
			}
			m_event.setSrcSock(sock_i);		// <--- tag event /w socket
			m_event.setSrcIP(m_socks[sock_i].src.ip); // recover sender address from socket
			bDeserial = true;

		}
		else { // More data for existing Event..
			bDeserial = false;
		}

		// BufferLen = actual bytes received at this time (may be partial)
		// EventLen = size of event in *network*, serialized event including data payload
		//    packetLen > eventLen      multiple events
		//    packetLen = eventLen      one event, or end of event
		//    packetLen < eventLen 			part of large event

		if (m_packetLen >= m_eventLen) { // One event, multiple, or end of large event..
			if (!bDeserial) { // Not start of event, attach more data				
				m_event.attachBuf(m_packetPtr, m_packetLen);
			}
			// End of event
			m_packetLen -= m_eventLen; // Advance buffer
			m_packetPtr += m_eventLen;
			
			// debugging
			int hsz = Event::staticSerializedHeaderSize();
			netPrintf(PRINT_VERBOSE, "RX %d bytes, %s", m_event.mDataLen + hsz, m_event.getNameStr().c_str());
			if ( m_event.mDataLen + hsz != m_eventLen ) {
				netPrintf(PRINT_ERROR, "Serialize issue. Event length %d != expected %d.", m_event.mDataLen + hsz, m_eventLen);
			}

			// Reset event length
			m_eventLen = 0;
		
			netQueueEvent(m_event);						
			free_event(m_event);
		}
		else { // Partial event..
			if (!bDeserial) { // Not start of event, attach more data				
				m_event.attachBuf(m_packetPtr, m_packetLen);
			}
			m_eventLen -= m_packetLen;
			m_packetPtr += m_packetLen;
			m_packetLen = 0;
		}
	}	
} */


void NetworkSystem::netReceiveByInjectedBuf(int sock_i, char* buf, int buflen )
{
	TRACE_ENTER((__func__));

	// This function receives an input stream from a specified buffer,
	// INSTEAD of the TCP/IP network. Used primarily for testing, it 
	// allows for the injection of a custom event stream to test deserialization.
	// Events will be pushed to the network queue for app processing as if they came from a network.
	// See also: netReceiveData
	
	// inject buffer
	NetSock& s = m_socks[sock_i];
	if ( buflen > s.pktMax ) {
		netPrintf ( PRINT_ERROR, "Injected packet too large. %d > %d max", buflen, s.pktMax );
		exit(-77);
	}
	memcpy ( s.pktBuf, buf, buflen );
	s.pktLen = buflen;

	// Deserialize events from input stream
	netDeserializeEvents ( sock_i );

	TRACE_EXIT((__func__));	
}

//----------------------------------------------------------------------------------------------------------------------
// -> RECIEVE CODE <-
//----------------------------------------------------------------------------------------------------------------------

void NetworkSystem::netReceiveData ( int sock_i )
{
	TRACE_ENTER ( (__func__) );
	NetSock& s = m_socks[ sock_i ];	
	int result = 1;

	while ( result > 0 ) {

		result = netSocketRecv ( sock_i, s.pktBuf, s.pktMax );
		
		if ( result < 0 ) {
			// recv error
			netManageTransmitError ( sock_i, "recv error" );			
			TRACE_EXIT ( (__func__) );
			return;

		} else if ( result > 0 ) {
			// received bytes. deserialize.
			s.pktLen = result; 
			assert ( result <= s.pktMax );
			netDeserializeEvents(sock_i);
		}

		#ifdef DEBUG_STREAM
			if ( m_packetLen > 0 ) { // Write TCP/IP stream to disk, with packet sizes
				FILE* fp1 = fopen ( "packet_stream.raw", "ab" );
				fwrite ( m_packetBuf, m_packetLen, 1, fp1 );
				fclose ( fp1 );
				FILE* fp2 = fopen ( "packet_sizes.txt", "at" );
				fprintf ( fp2, "%d\n", m_packetLen );
				fclose ( fp2 );
			}
		#endif			
	}
	// done when result = 0

	TRACE_EXIT ( (__func__) );	
}

//----------------------------------------------------------------------------------------------------------------------
// -> Send CODE <-
//----------------------------------------------------------------------------------------------------------------------

void NetworkSystem::netQueueEvent ( Event& e )
{
	TRACE_ENTER ( (__func__) );

	// persistent event
	Event* eq = new Event;
	eq->acquire ( e );				// eq now owns the data
	eq->persist ();					// persist beyond scope of this func
	eq->rescope ( "nets" );

	m_eventQueue.Push ( eq );	// data payload is owned by queued event

	TRACE_EXIT ( (__func__) );
}

void NetworkSystem::netMakeEvent ( Event& e, eventStr_t name, eventStr_t sys )
{
	TRACE_ENTER ( (__func__) );

	new_event ( e, 120, sys, name, 0, m_eventPool, "netMake"  );
	e.setSrcIP ( m_hostIp );	// default to local IP if protocol doesn't transmit sender
	e.setTarget ( 'net ' );		// all network configure events have a 'net ' target name
	e.setName ( name );
	e.startWrite ();			

	TRACE_EXIT ( (__func__) );	
}

int NetworkSystem::netFindSocket ( int side, int mode, int type )
{
	TRACE_ENTER ( (__func__) );
	for ( int n = 0; n < m_socks.size ( ); n++ ) { // Find socket by mode & type
		if ( m_socks[ n ].mode == mode && m_socks[ n ].side == side && (m_socks[ n ].src.type==type || type==NTYPE_ANY) ) {
			TRACE_EXIT ( (__func__) );
			return n;
		}
	}
	TRACE_EXIT ( (__func__) );
	return -1;
}

int NetworkSystem::netFindSocket ( int side, int mode, int state, NetAddr dest )
{
	TRACE_ENTER ( (__func__) );
	for ( int n = 0; n < m_socks.size ( ); n++ ) { // Find socket with specific destination
		if ( m_socks[ n ].mode == mode && m_socks[ n ].side == side && m_socks[ n].state == state ) {
			if ( m_socks[ n ].dest.type == dest.type &&  m_socks[ n ].dest.ip == dest.ip && m_socks[ n ].dest.port == dest.port ) {
				TRACE_EXIT ( (__func__) );
				return n;
			}
		}
	}
	TRACE_EXIT ( (__func__) );
	return -1;
}

int NetworkSystem::netFindOutgoingSocket ( bool bTcp )
{
	TRACE_ENTER ( (__func__) );
	for ( int n=0; n < m_socks.size ( ); n++ ) { // Find first fully-connected outgoing socket
		if ( m_socks[ n ].mode==NET_TCP && m_socks[ n ].state == STATE_CONNECTED ) {
			TRACE_EXIT ( (__func__) );
			return n;
		}
	}
	TRACE_EXIT ( (__func__) );
	return -1;
}

str NetworkSystem::netPrintAddr ( NetAddr adr )
{
	TRACE_ENTER ( (__func__) );
	char buf[128];
	str type;
	switch ( adr.type ) {
	case NTYPE_ANY:			type = "any  ";	break;
	case NTYPE_BROADCAST:		type = "broad";	break;
	case NTYPE_SEARCH:		type = "srch";	break;
	case NTYPE_CONNECT:		type = "conn";	break;
	};
	sprintf ( buf, "%s,%s:%d", type.c_str(), getIPStr(adr.ip).c_str(), adr.port );
	TRACE_EXIT ( (__func__) );
	return buf;
}

void NetworkSystem::netList ( bool verbose )
{
	TRACE_ENTER ( (__func__) );
	if ( m_printVerbose || verbose ) { // Print the network
		str side, mode, stat, src, dst, msg, secur;
		if ( int(m_socks.size()) < 0) {
			dbgprintf ( "ERROR: Corruption in m_socks.size(): %d\n", m_socks.size() );
			exit(-11);
		}
		dbgprintf ( "\n------ NETWORK SOCKETS %d. MyIP: %s, %s\n", m_socks.size(), m_hostName.c_str ( ), getIPStr ( m_hostIp ).c_str ( ) );
		for ( int n = 0; n < m_socks.size (); n++ ) {
			side = ( m_socks[n].side == NET_CLI ) ? "cli" : "srv";
			secur = (m_socks[n].security & NET_SECURITY_OPENSSL) ? "ssl" : "tcp";			// future: udp should made a security level, remove s.mode variable.
			stat == "";
			switch ( m_socks[n].state ) {
			case STATE_NONE:			stat = "off      ";	break;
			case STATE_START:			stat = "start    "; break;
			case STATE_HANDSHAKE:	stat = "handshake"; break;
			case STATE_CONNECTED:	stat = "connected"; break;
			case STATE_TERMINATED:stat = "terminatd"; break;
			};
			src = netPrintAddr ( m_socks[n].src );
			dst = netPrintAddr ( m_socks[n].dest );
			msg = "";
			if ( m_socks[n].side==NET_CLI && m_socks[n].state == STATE_CONNECTED ) msg = "<-- to Server";
			if ( m_socks[n].side==NET_SRV && m_socks[n].state == STATE_CONNECTED ) msg = "<-- to Client";
			if ( m_socks[n].side==NET_SRV && m_socks[n].src.type == NTYPE_ANY) msg = "<-- Server Listening Port";
			dbgprintf ( "%d: %s %s %s src[%s] dst[%s] %s\n", n, side.c_str(), secur.c_str(), stat.c_str(), src.c_str(), dst.c_str(), msg.c_str() );
		}
		dbgprintf ( "------\n");
	}
	TRACE_EXIT ( (__func__) );
}

//----------------------------------------------------------------------------------------------------------------------
// -> LOW-LEVEL WRAPPER <-
//----------------------------------------------------------------------------------------------------------------------

void NetworkSystem::netStartSocketAPI ( )
{
	TRACE_ENTER ( (__func__) );
	CXSocketApiInit ( );
	TRACE_EXIT ( (__func__) );
}

void NetworkSystem::netSetHostname ()
{
	TRACE_ENTER ( (__func__) );
	CXSetHostname ( );
	netPrintf ( PRINT_VERBOSE, "  Local Host: %s, %s:%s", m_hostName.c_str ( ), getIPStr ( m_hostIp ).c_str ( ) );
	TRACE_EXIT ( (__func__) );
}

bool NetworkSystem::netSendLiteral ( str str_lit, int sock_i )
{
	TRACE_ENTER ( (__func__) );
	int len = str_lit.length ( ), result;
	char* buf = (char*) malloc ( str_lit.length ( ) + 1 );
	strcpy ( buf, str_lit.c_str ( ) );	
	
	NetSock& s = m_socks[ sock_i ]; // Send over socket
	if ( s.mode == NET_TCP ) {
		if ( s.security == NET_SECURITY_PLAIN_TCP || s.state < STATE_HANDSHAKE ) {
			result = send ( s.socket, buf, len, 0 ); // TCP/IP
		} else {
			#ifdef BUILD_OPENSSL
				if ( ( result = SSL_write ( s.ssl, buf, len ) ) <= 0 ) {	
					if ( netNonFatalErrorSSL ( sock_i, result ) ) { 
						TRACE_EXIT ( (__func__) );
						return SSL_ERROR_WANT_WRITE;
					} else {
						str msg = netGetErrorStringSSL ( result, s.ssl );
						netPrintf ( PRINT_ERROR, "Failed at ssl write (1): Returned: %d: %s", result, msg.c_str ( ) );
					}
				}
			#endif
		} 
	}
	else {
		int addr_size;
		addr_size = sizeof ( s.dest.addr );
		result = sendto ( s.socket, buf, len, 0, (sockaddr*)&s.dest.addr, addr_size ); // UDP
	}
	free( buf );
	TRACE_EXIT ( (__func__) );
	return CXSocketBlockError ( ) || netCheckError ( result, sock_i );		
}

bool NetworkSystem::netCheckError ( int result, int sock_i )
{
	TRACE_ENTER ( (__func__) );
	if ( !CXSocketIsValid ( m_socks[ sock_i ].socket ) ) {
		netManageTransmitError ( sock_i, "unexpected error" );		// Peer has shutdown (unexpected shutdown)
		netPrintf ( PRINT_ERROR, "Unexpected shutdown" );
		TRACE_EXIT ( (__func__) );
		return false;
	}
	TRACE_EXIT ( (__func__) );
	return true; // TODO: Check this; treat as benign error if there is a tail to send
}

void NetworkSystem::netSendResidualEvent ( int sock_i )
{
	TRACE_ENTER ( (__func__) );
	NetSock& s = m_socks[ sock_i ];	
	int result = 0;

	if ( s.security == NET_SECURITY_PLAIN_TCP || s.state < STATE_HANDSHAKE ) {
		result = send ( s.socket, s.txBuf, s.txLen, 0 ); // TCP/IP
	} else {
		s.txLen = 0;
		TRACE_EXIT ( (__func__) );
		return;
		//result = SSL_write ( s.ssl, s.txBuf + s.txSoFar, remaining );
	}
	
	if ( result > 0 ) {
		s.txLen -= result;
		int remain = s.txLen;
		memmove ( s.txBuf, s.txBuf + result, remain );		// overlapping move
		s.txPtr = s.txBuf + remain;
		if ( result != s.txLen ) {
			netPrintf ( PRINT_FLOW, "TX %d/%d (txLen=%d)", result, remain, s.txLen );
		} else {
			netPrintf ( PRINT_FLOW, "TX %d/%d (txLen=%d) - DONE", result, remain, s.txLen );			
			s.txLen = s.txPktSize = 0;
		} 
	} 
	TRACE_EXIT ( (__func__) );
}

bool NetworkSystem::netSend ( Event& e, int sock_i )
{
	TRACE_ENTER ( (__func__) );
	
	// caller may wish to send on any outgoing socket
	if ( sock_i == -1 ) { 
		sock_i = netFindOutgoingSocket ( true );
		if ( sock_i == -1 ) 						{ TRACE_EXIT ( (__func__) ); return false; }		
	}
	// check valid socket idx
	if (!valid_socket_index(sock_i)) 		{ TRACE_EXIT ( (__func__) ); return false; }

	// get socket
	NetSock& s = m_socks[ sock_i ];
	
	// cannot send on a listening socket
	if ( m_socks[ sock_i ].src.type == NTYPE_ANY) 	{ TRACE_EXIT ( (__func__) ); return false; }

	// make sure we have a transmission buffer
	if ( m_socks[ sock_i ].txLen > 0 ) 		{ TRACE_EXIT ( (__func__) ); return false; }	

	// make sure we have an event data buffer
	int result;
	e.rescope ( "nets" );
	if ( e.mData == 0x0 ) 							{ TRACE_EXIT ( (__func__) ); return false; }
	
	// event retains is persist/consume status
	//  (will pass thru send back to caller)
	//
	e.serialize ();		// Prepare serialized buffer	
	char* buf = e.getSerializedData ( );
	int event_len = e.getSerializedLength ( );

	// Checksum [debugging] - determine if send/recv buffers match
	xlong chksum = 0;
	if ( m_printFlow ) {
		chksum = ComputeChecksum( buf, event_len );		
	}

	netPrintf ( PRINT_FLOW, "TX %d bytes, %s --> SENDING  chksum=%lld", e.getSerializedLength (), e.getNameStr().c_str(), chksum );

	if ( m_socks[ sock_i ].mode == NET_TCP ) { // Send over socket
		if ( s.security == NET_SECURITY_PLAIN_TCP || s.state < STATE_HANDSHAKE ) {

			result = send ( s.socket, buf, event_len, 0 ); // TCP/IP

			if ( result > 0 ) {			
				// bytes sent
				if ( result == event_len ) {
					// full event sent					
				} else {
					// partial event sent, transmit more later
					int remain = event_len - result;
					netExpandBuf (s.txBuf, s.txPtr, s.txMax, s.txLen, s.txLen + remain);
					memcpy ( s.txPtr, buf + result, remain);
					s.txLen += remain;					
					s.txBuf[ event_len ] = '\0';
					netPrintf ( PRINT_FLOW, "TX %d/%d, %d remain (txLen=%d)", result, event_len, remain, s.txLen );
				}
				
				// done
				TRACE_EXIT ( (__func__) );
				return true;
			}
			
		} else {
			#ifdef BUILD_OPENSSL
				
				fd_set sockSet;
				struct timeval tv = { 0, 0 };
				int fd = SSL_get_fd ( s.ssl );
				FD_ZERO ( &sockSet );
				FD_SET ( fd, &sockSet );	
				if ( select ( fd + 1, NULL, &sockSet, NULL, &tv ) < 0 ) {
					TRACE_EXIT ( (__func__) );
					return false;
				}
				
				result = SSL_write ( s.ssl, buf, event_len );

				if ( result > 0 ) {
					// bytes sent
					if ( result == event_len ) {
						// full event sent
					} else if ( result < event_len ) {	
						// partial event sent, transmit more later
						netExpandBuf (s.txBuf, s.txPtr, s.txMax, s.txLen, event_len + 1);
						s.txLen = result;
						s.txPktSize = result;
						memcpy ( s.txBuf, buf, result );
						// s.txBuf[ event_len ] = '\0';					
						netPrintf ( PRINT_VERBOSE, "1 Partial TX: %d < %d", result, event_len);					
						std::cin.get();						
					}	else {
						netPrintf(PRINT_ERROR, "Unexpected TX: %d > %d", result, event_len );
						exit(-77);
					}
					
					TRACE_EXIT ( (__func__) );
					return true;

				} else {
					// no bytes sent. check why.
					if ( netNonFatalErrorSSL ( sock_i, result ) ) { 
						str msg = netGetErrorStringSSL ( result, s.ssl );
						s.txLen = 0;
						netPrintf ( PRINT_ERROR, "Non fatal SSL error: Return: %d: %s", result, msg.c_str ( ) );
						TRACE_EXIT ( (__func__) );
						return false;						
					} else {
						str msg = netGetErrorStringSSL ( result, s.ssl );
						netPrintf ( PRINT_ERROR, "1 Failed ssl write (2): Return: %d: %s", result, msg.c_str ( ) );
					}
				} 
			#endif
		}  
	} else {
		int addr_size;
		addr_size = sizeof( m_socks[ sock_i ].dest.addr );
		result = sendto ( s.socket, buf, event_len, 0, (sockaddr*) &s.dest.addr, addr_size ); // UDP
	}
	
	// if we got here, send failed
	TRACE_EXIT ( (__func__) );
	return false;	
}

// create a transport socket
//
int NetworkSystem::netSocketCreate ( int sock_i )
{
	TRACE_ENTER ( (__func__) );
	NetSock& s = m_socks[ sock_i ];	    
	
	// note: STATE_NONE must be allowed here. indicates client socket being reused. 
	if ( s.socket == 0 ) {
		if ( s.mode == NET_TCP ) {
			s.socket = socket ( AF_INET, SOCK_STREAM, IPPROTO_TCP ); 
		} else {
			s.socket = socket ( AF_INET, SOCK_DGRAM, IPPROTO_UDP ); 
		}
	}
	CXSocketUpdateAddr ( sock_i, true );
	CXSocketUpdateAddr ( sock_i, false );

	TRACE_EXIT ( (__func__) );
	return 1;
}

int NetworkSystem::netSocketBind ( int sock_i )
{
	TRACE_ENTER ( (__func__) );
	NetSock* s = &m_socks [ sock_i ];
	int addr_size;
	addr_size = sizeof ( s->src.addr );
	netPrintf ( PRINT_VERBOSE, "Bind: %s, port %i", ( s->side == NET_CLI ) ? "cli" : "srv", s->src.port );
	int ret = 0;
	ret = bind ( s->socket, (sockaddr*) &s->src.addr, addr_size );
	if ( netFuncError(ret) ) {
		netPrintf ( PRINT_ERROR, "Cannot bind to source: Return: %d", ret );
	}
	TRACE_EXIT ( (__func__) );
	return ret;
}

// CXSocketWouldBlock
// call after 'connect' to determine if error is ok.
// Non-errors
// - would block (again) - non-blocking socket incomplete
// - is conn - already connected
// - in progress - blocking connect is in progress
// - already - non-blocking connect in progress
bool NetworkSystem::CXSocketWouldBlock ( std::string& msg )
{
	bool ok = false;

	#ifdef _WIN32 
		// windows (winsock)
		int e = WSAGetLastError ();
		msg = CXGetErrorMsg ( e );
		if (e==WSAEWOULDBLOCK || e == WSAEISCONN || e==WSAEINPROGRESS || e == WSAEALREADY ) {
			ok = true;
		}
	#else
		int e = errno;
		msg = strerror(e);		
		if (e == EWOULDBLOCK || e == EAGAIN || e==EINPROGRESS || e==EALREADY || errno == EISCONN) {
			ok = true;
		}
	#endif	
	return ok;
}

int NetworkSystem::netSocketConnect ( int sock_i )
{
	std::string msg;
	TRACE_ENTER ( (__func__) );
	NetSock* s = &m_socks[ sock_i ];
	int addr_size;
	addr_size = sizeof ( s->dest.addr );

	netPrintf ( PRINT_VERBOSE_HS, "Trying connect. Client %s:%d  -> Srv %s:%d", getIPStr(s->src.ip).c_str(), s->src.port, getIPStr (s->dest.ip ).c_str ( ), s->dest.port );

	int ret = connect (s->socket, (sockaddr*)&s->dest.addr, addr_size);

	if ( netFuncError(ret) ) {

		if ( CXSocketWouldBlock (msg) ) {
			// connection in progress. wait for it.
			// check for complete connection using select		
			ret = 0;				
		} else {
			// fatal error
			netManageHandshakeError ( sock_i, "		 Connect fatal error. " + msg );			
		}		
	}

	TRACE_EXIT ( (__func__) );
	return ret;
}

int NetworkSystem::netSocketListen ( int sock_i )
{
	TRACE_ENTER ( (__func__) );
	NetSock& s = m_socks [ sock_i ];
//	netPrintf ( PRINT_VERBOSE, "Listen: ip %s, port %i", getIPStr(s.src.ip).c_str(), s.src.port );
	int ret = listen ( s.socket, SOMAXCONN );
	if ( netFuncError( ret ) ) {
		netPrintf ( PRINT_ERROR, "TCP listen error: Return: %d", ret );
	}
	TRACE_EXIT ( (__func__) );
	return ret;
}

int NetworkSystem::netSocketAccept ( int sock_i, CX_SOCKET& tcp_sock, netIP& cli_ip, netPort& cli_port )
{
	TRACE_ENTER ( (__func__) );
	std::string msg;
	NetSock& s = m_socks [ sock_i ];
	struct sockaddr_in sin;
	int addr_size = sizeof ( sin );

	tcp_sock = accept ( s.socket, (sockaddr*) &sin, (socklen_t *) (&addr_size) );

	if ( !CXSocketIsValid ( tcp_sock ) ) {
		if ( CXSocketWouldBlock( msg ) ) {
			// No error. Non-blocking wait.
			TRACE_EXIT((__func__));
			return 0;
		}	else {
			// Accept had real error.
			netPrintf(PRINT_ERROR_HS, "TCP accept error: Return: %d", tcp_sock);
			TRACE_EXIT((__func__));
			return -1;
		}		
	}

	// Accept completed.
	cli_ip = sin.sin_addr.s_addr;		// IP address of connecting client
	cli_port = sin.sin_port;				// Accepting TCP does not know/care what the client port is
	TRACE_EXIT ( (__func__) );

	return 1;
}

int NetworkSystem::netSocketRecv ( int sock_i, char* buf, int bufmax )
{
	TRACE_ENTER ( (__func__) ); // Return value: success = 0, or an error number; on success recvlen = bytes recieved
	CX_SOCKLEN addr_size;
	int result;
	NetSock& s = m_socks [ sock_i ];
	std::string msg;

	if ( s.src.type != NTYPE_CONNECT ) {
		TRACE_EXIT ( (__func__) );
		return 0; // Only recv on connection sockets
	}
	
	addr_size = sizeof ( s.src.addr );
	if ( s.mode == NET_TCP ) {
		if ( s.security == NET_SECURITY_PLAIN_TCP || s.state < STATE_HANDSHAKE ) { 

			result = recv ( s.socket, buf, bufmax, 0 );	// TCP/IP
			if ( netFuncError(result) ) {
				TRACE_EXIT((__func__));
				if ( CXSocketWouldBlock(msg) ) {					
					return 0;			// pending socket. no error.
				} else {					
					return -1;		// actual error
				}
			}

		} else {
			#ifdef BUILD_OPENSSL
				result = SSL_read(s.ssl, buf, bufmax);
				if ( result <= 0 ) {
					if ( netNonFatalErrorSSL ( sock_i, result ) ) { 
						TRACE_EXIT ( (__func__) );
						return SSL_ERROR_WANT_READ;
					} else {
						str msg = netGetErrorStringSSL ( result, s.ssl );
						netPrintf ( PRINT_ERROR, "Failed at ssl read: Returned: %d: %s", result, msg.c_str ( ) );
					}
				}
			#endif
		}
	} else {
		result = recvfrom ( s.socket, buf, bufmax, 0, (sockaddr*) &s.src.addr, &addr_size ); // UDP
	}

	TRACE_EXIT ( (__func__) );
	return result;		// bytes read
}

bool NetworkSystem::netSocketIsConnected ( int sock_i )
{
    TRACE_ENTER ( (__func__) );
    NetSock& s = m_socks[ sock_i ];
    fd_set sockSet;
    FD_ZERO ( &sockSet );
    FD_SET ( s.socket, &sockSet );
    struct timeval tv = { 0, 0 };
    CX_SOCKOPT opt = -1;
		// Use select and result from getsockopt to check if connection is done
    if ( select ( s.socket + 1, NULL, &sockSet, NULL, &tv ) > 0 ) { 
        CX_SOCKLEN len = sizeof ( CX_SOCKOPT );
        getsockopt ( s.socket, SOL_SOCKET, SO_ERROR, &opt, &len );
    }
    TRACE_EXIT ( (__func__) );
    return opt == 0;			// connected ==> opt=0 means no SO_ERROR 
}

bool NetworkSystem::netSocketIsSelected ( fd_set* sockSet, int sock_i )
{
	if ( !valid_socket_index ( sock_i ) ) return false;

	NetSock& s = m_socks[ sock_i ];
	if ( s.security == NET_SECURITY_PLAIN_TCP || s.state < STATE_HANDSHAKE ) { 
		return FD_ISSET ( s.socket, sockSet );
	} 
	#ifdef BUILD_OPENSSL
		if ( s.ssl ) {
			return FD_ISSET ( SSL_get_fd ( s.ssl ), sockSet );
		}
	#else
		return false;
	#endif
	return false;
}

int NetworkSystem::netSocketSelect ( fd_set* sockReadSet, fd_set* sockWriteSet ) 
{
	TRACE_ENTER ( (__func__) );
	if ( m_socks.size ( ) == 0 ) {
		TRACE_EXIT ( (__func__) );
		return 0;
	}

	int result, maxfd =- 1;
	NET_PERF_PUSH ( "socklist" );
	FD_ZERO ( sockReadSet );
	FD_ZERO ( sockWriteSet );
	for ( int n = 0; n < (int) m_socks.size ( ); n++ ) { // Get all sockets that are Enabled or Connected
		NetSock& s = m_socks[ n ];
		if ( s.state != STATE_NONE && s.state != STATE_TERMINATED && s.state != STATE_FAILED ) {
			if ( s.security == NET_SECURITY_PLAIN_TCP || s.state < STATE_HANDSHAKE ) { 
				FD_SET ( s.socket, sockReadSet );
				if ( s.txLen > 0 ) {
					FD_SET ( s.socket, sockWriteSet );
				}
				if ( (int) s.socket > maxfd ) maxfd = s.socket;
			} else { 
				#ifdef BUILD_OPENSSL
					int fd = SSL_get_fd ( s.ssl );
					FD_SET ( fd, sockReadSet );	
					if ( s.txLen > 0 ) {
						FD_SET ( fd, sockWriteSet );	
					}
					if ( (int) fd > maxfd ) maxfd = fd;
				#endif
			}
		}
	}
	NET_PERF_POP ( );
	
	if ( ++maxfd == 0 ) {
		TRACE_EXIT ( (__func__) );
		return 0; // No sockets
	}

	NET_PERF_PUSH ( "select" );
	timeval tv;
    tv.tv_sec = m_rcvSelectTimout.tv_sec;
	tv.tv_usec = m_rcvSelectTimout.tv_usec;
	result = select ( maxfd, sockReadSet, sockWriteSet, NULL, &tv ); // Select all sockets that have changed
	NET_PERF_POP ( );
	TRACE_EXIT ( (__func__) );
	return result;
}

str NetworkSystem::netPrintf ( int flag, const char* fmt_raw, ... )
{
	std::string srvcli = isServer() ? "netS> " : "netC> ";

	if ( ( flag == PRINT_VERBOSE || flag == PRINT_VERBOSE_HS ) && ! m_printVerbose ) {
		return str("");
	}
	if ( flag == PRINT_FLOW && ! m_printFlow ) {
		return str("");
	}

	str tag;
  char buffer[ 2048 ];
  if ( flag == PRINT_ERROR_HS ) {
		tag = "    ";
		flag = PRINT_ERROR;
	} else if ( flag == PRINT_VERBOSE_HS ) {
		tag = "    ";
		flag = PRINT_VERBOSE;
	} else {
		tag = "";
	}
	
    va_list args;
    va_start ( args, fmt_raw );
    vsnprintf ( buffer, sizeof ( buffer ), fmt_raw, args );
    va_end ( args );
    str msg = str ( buffer ) + "\n";
	
  switch ( flag ) {
		case PRINT_VERBOSE:
			if ( m_printVerbose ) {
				msg = srvcli + tag + msg;
				dbgprintf ( msg.c_str ( ) );
			}
			break;
		case PRINT_FLOW:
			if ( m_printFlow ) {
				msg = tag + msg;
				dbgprintf ( msg.c_str ( ) );
			}
			break;
		case PRINT_ERROR:
			int error_id = 0;			// request last err
			str error_str = CXGetErrorMsg ( error_id );
			str delim = tag +  "=================================================\n";
			msg = delim + srvcli + tag + str("ERROR: ") + msg + ": " + error_str + "\n" + delim;
			dbgprintf ( msg.c_str ( ) );
			break;
	}
	return msg;
}

NetSock* NetworkSystem::getSock ( int i )		{ return valid_socket_index(i) ? &m_socks[ i ] : 0; }
str	 NetworkSystem::getSockSrcIP(int i)			{ return valid_socket_index(i) ? getIPStr(m_socks[i].src.ip) : ""; }
str	 NetworkSystem::getSockDestIP ( int i ) { return valid_socket_index(i) ? getIPStr(m_socks[i].dest.ip ) : "";}
int	 NetworkSystem::getServerSock( int i)		{ return valid_socket_index(i) ? m_socks[ i ].dest.sock : -1; }


str NetworkSystem::getIPStr ( netIP ip )
{
	TRACE_ENTER ( (__func__) );
	str ipstr = CXGetIpStr ( ip );
	TRACE_EXIT ( (__func__) );
	return ipstr;
}

netIP NetworkSystem::getStrToIP ( str name )
{
	TRACE_ENTER ( (__func__) );
	char ipname [ 1024 ];
	strcpy ( ipname, name.c_str ( ) );
	TRACE_EXIT ( (__func__) );
	return inet_addr ( ipname );
}

//----------------------------------------------------------------------------------------------------------------------
//
// -> PUBLIC CONFIG API <-
//
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
// -> MISCELLANEOUS CONFIG API <-
//----------------------------------------------------------------------------------------------------------------------

void NetworkSystem::netSetSelectInterval ( int time_ms ) 
{
	m_rcvSelectTimout.tv_sec = time_ms / 1000;
	m_rcvSelectTimout.tv_usec = ( time_ms % 1000 ) * 1000; 
}

//----------------------------------------------------------------------------------------------------------------------
// -> SECURITY CONFIG API <-
//----------------------------------------------------------------------------------------------------------------------

bool NetworkSystem::netSetReconnectInterval ( int time_ms )
{
	if ( m_hostType == 's' ) {
		return false;
	}
	m_reconnectInterval = time_ms;
	return true;
}

bool NetworkSystem::netSetReconnectLimit ( int limit )
{
	m_reconnectLimit = limit;
	return true;
}

bool NetworkSystem::netSetReconnectLimit ( int limit, int sock_i )
{
	if ( !valid_socket_index ( sock_i ) ) {
		return false;
	}
	m_socks[ sock_i ].reconnectLimit = limit;
	m_socks[ sock_i ].reconnectBudget = limit;
	return true;
}

//----------------------------------------------------------------------------------------------------------------------

bool NetworkSystem::netSetSecurityLevel ( int levels )
{
	m_security = 0;
	if ( levels & NET_SECURITY_PLAIN_TCP ) {
		m_security |= NET_SECURITY_PLAIN_TCP;
	}
	if ( levels & NET_SECURITY_OPENSSL ) {
		m_security |= NET_SECURITY_OPENSSL;
	}
	return ( m_security | levels ) == m_security;
}

bool NetworkSystem::netSetSecurityLevel ( int levels, int sock_i )
{
	m_security = 0;
	if ( levels & NET_SECURITY_PLAIN_TCP ) {
		m_socks[ sock_i ].security |= NET_SECURITY_PLAIN_TCP;
	}
	if ( levels & NET_SECURITY_OPENSSL == NET_SECURITY_OPENSSL ) {
		m_socks[ sock_i ].security |= NET_SECURITY_OPENSSL; 
	}
	return ( m_socks[ sock_i ].security | levels ) == m_security;
}

//----------------------------------------------------------------------------------------------------------------------

bool NetworkSystem::netSetPathToPublicKey ( str path )
{	
	char msg[ 2048 ];
	str found_path;
	if ( ! getFileLocation ( path, found_path ) ) {
		sprintf ( msg, "Public key not found: %s", path.c_str ( ) );
		netPrintf ( PRINT_ERROR, msg );
		return false;
	}
	m_pathPublicKey = found_path;	
	return true;
}

bool NetworkSystem::netSetPathToPrivateKey ( str path )
{
	char msg[ 2048 ];
	str found_path;
	if ( ! getFileLocation ( path, found_path ) ) {
		sprintf ( msg, "Private key not found: %s", path.c_str ( ) );
		netPrintf ( PRINT_ERROR, msg );
		return false;	
	}
	m_pathPrivateKey = found_path;	
	return true;
}

bool NetworkSystem::netSetPathToCertDir ( str path )
{
	char buf[ 2048 ];
	strncpy ( buf, path.c_str ( ), 2048 );
	addSearchPath ( buf );
	m_pathCertDir = path;
	return true;
}

bool NetworkSystem::netSetPathToCertFile ( str path )
{
	char msg[ 2048 ];
	str found_path;
	if ( ! getFileLocation ( path, found_path ) ) {
		sprintf ( msg, "Cert file not found: %s\n", path.c_str ( ) );
		netPrintf ( PRINT_ERROR, msg );
		return false;	
	}
	m_pathCertFile = found_path;		
	return true;
}

//----------------------------------------------------------------------------------------------------------------------
// -> END <-
//----------------------------------------------------------------------------------------------------------------------
