//---------------------------------------------------------------------
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
// - Internal event pooling to handle many, small events
// - Arbitrary event size, regardless of TCP/IP buffer size
// - Graceful disconnect for unexpected shutdown of client or server
// - Reconnect for clients
// - Verbose error handling
// - C++ class model allows for multiple client/server objects
// - C++ class model with no inheritence for simplicity
// - Cross-platform and tested on Windows, Linux and Android
// 
//---------------------------------------------------------------------

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

	#define NET_BUFSIZE			65535		// Typical UDP max packet size

	// -- NOTES --
	// IP               = 20 bytes
	// UDP = IP + 8     = 28 bytes
	// TCP = IP + 28    = 48 bytes
	// Event            = 24 bytes (header)
	// TCP + Event      = 72 bytes (over TCP)

	typedef int (*funcEventHandler) ( Event& e, void* this_ptr  );

	class EventPool;

	class HELPAPI NetworkSystem {
	public:
		NetworkSystem ();

		// Network System
		void netInitialize ();
		void netCreate ();
		void netDestroy ();
		void netDebug(bool v)		{ mbDebugNet = v; mbVerbose = v;  }
		void netVerbose (bool v)	{ mbVerbose = v; }
		void netPrint (bool verbose=false);
		std::string netPrintAddr ( NetAddr adr );

		// Server - Network API
		void netStartServer ( netPort srv_port);
		void netServerListen ( int sock );

		// Client - Network API
		void netStartClient ( netPort srv_port);
		int  netClientConnectToServer ( std::string srv_name, netPort srv_port, bool blocking = false );
		int  netCloseConnection ( int localsock );
		int  netCloseAll ();

		// Event processing
		void netProcessEvents ( Event& e );
		int  netProcessQueue (void);					// Process input queue.
		int  netRecieveData ();
		Event netMakeEvent ( eventStr_t name, eventStr_t sys );
		bool netSend ( Event& e );
		bool netSend ( Event& e, int mode, int sock );	// Send over network
		bool netSendLiteral(std::string str, int sock);
		void netQueueEvent ( Event& e );				// Place incoming event on recv queue
		int  netEventCallback ( Event& e);				// Processes network events (dispatch)
		void netSetUserCallback (funcEventHandler userfunc )	{ mUserEventCallback = userfunc; }
		bool netIsConnected (int sock);					// confirm connected
		bool netCheckError ( int result, int sock );
		int netError ( std::string msg, int error_id=0);

		// Sockets - abtract functions
		int netAddSocket ( int side, int mode, int status, bool block, NetAddr src, NetAddr dest );
		int netFindSocket ( int side, int mode, int type );
		int netFindSocket ( int side, int mode, NetAddr dest );
		int netFindOutgoingSocket ( bool bTcp );
		int netTerminateSocket ( int sock );
		NetSock& getSock (int sock)					{ return mSockets[sock]; }
		std::string getSocketIP(int sock)		{ return getIPStr( mSockets[sock].dest.ipL ); }

		// Sockets - socket-specific low-level functions
		void netStartSocketAPI ();
		void netGetHostname ();
		int netUpdateSocket ( int sock );
		int netSocketBind ( int sock );						// low-level bind()
		int netSocketConnect ( int sock );					// low-level connect()
		int netSocketListen ( int sock );					// low-level listen()
		int netSocketAccept ( int sock,  SOCKET& tcp_sock, netIP& cli_ip, netPort& cli_port  );	// low-level accept()
		int netSocketRecv ( int sock, char* buf, int buflen, int& recvlen); // low-level recv()
		bool netIsError ( int result );			// socket-specific error check
		void netReportError ( int result );
		int	netGetServerSocket ( int sock )	{ return (sock >= mSockets.size()) ? -1 : mSockets[sock].dest.sock; }

		bool netIsQueueEmpty() { return (mEventQueue.size()==0); }

		// Accessors
		TimeX				getSysTime()		{ return TimeX::GetSystemNSec(); }
		std::string	getHostName ()	{ return mHostName; }
		netIP				getHostIP ()		{ return mHostIP; }
		std::string getIPStr ( netIP ip );									// return IP as a string
		netIP				getStrToIP ( std::string name );
		int					getMaxPacketLen ()	{ return mMaxPacketLen; }
		EventPool*  getPool()						{ return mEventPool; }

	public:

		EventPool*					mEventPool;				// Event Memory Pool
		EventQueue					mEventQueue;			// Network Event queue

		eventStr_t					mHostType;
		std::string					mHostName;				// Host info
		netIP						mHostIP;
		int							mReadyServices;

		std::vector< NetSock >		mSockets;				// Socket list

		funcEventHandler			mUserEventCallback;		// User event handler

		// Incoming event data
		int							mDataLen;
		int							mEventLen;
		Event						mEvent;					// Incoming event

		// Network buffers
		int							mBufferLen;
		char*						mBufferPtr;
		char						mBuffer[NET_BUFSIZE];
		int							mMaxPacketLen;

		// Debugging
		int							mCheck;
		bool						mbDebugNet;
		bool						mbVerbose;

        #ifdef _WIN32
            struct fd_set			sock_set;
        #elif __ANDROID__
            fd_set				    sock_set;
        #elif __linux__
            fd_set				    sock_set;
        #endif
	};

	extern NetworkSystem* net;

#endif
