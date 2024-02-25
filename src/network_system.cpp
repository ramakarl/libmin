//---------------------------------------------------------------------
//
// Network System
// Quanta Sciences, Rama Hoetzlein (c) 2007-2020
//
//---------------------------------------------------------------------

#include <assert.h>
#include "network_system.h"

#ifdef __linux__
  #include <net/if.h>
  #include <netinet/in.h>
#elif __ANDROID__
  #include <net/if.h>
  #include <netinet/in.h>
#endif

NetworkSystem* net;



NetworkSystem::NetworkSystem ()
{
	mHostType = '    ';
	mHostIP = 0;
	mReadyServices = 0;
	mUserEventCallback = 0;
	mbVerbose = false;
	mbDebugNet = false;
}

//--------------------------------------------------- NETWORK SERVER
//

void NetworkSystem::netStartServer ( netPort srv_port )
{
	// Start a TCP listen socket on Client
	if (mbVerbose) dbgprintf ( "Start Server:\n");
	mHostType = 'srv ';
	netIP srv_anyip = inet_addr ("0.0.0.0");

	int srv_sock = netAddSocket ( NET_SRV, NET_TCP, NET_ENABLE, false, 
									NetAddr(NET_ANY, mHostName, srv_anyip, srv_port),
		                            NetAddr(NET_BROADCAST, "", 0, srv_port ) );

	const char reuse = 1;
	if ( setsockopt( mSockets[srv_sock].socket, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int)) < 0)		
		if (mbVerbose) dbgprintf ( "Error: Setting server socket as SO_REUSEADDR.\n" );

	netSocketBind ( srv_sock );

	netSocketListen ( srv_sock );	
}

void NetworkSystem::netServerListen ( int sock )
{
	int srv_sock_svc = netFindSocket ( NET_SRV, NET_TCP, NET_ANY );
	NetSock srv = getSock(srv_sock_svc);
	std::string srv_name = srv.src.name;
	netPort srv_port = srv.src.port;

	netIP cli_ip = 0;
	netPort cli_port = 0;

	// Accept TCP on a service socket (open port)

	SOCKET newSOCK;			// new literal socket

	int result = netSocketAccept ( srv_sock_svc, newSOCK, cli_ip, cli_port );
	if ( result < 0 ) {
		if (mbVerbose) dbgprintf ( "Connection not accepted.\n");
		return;
	}
	// Get server IP. Listen/accept happens on ANY address (0.0.0.0)
	// we want the literal server IP for final connection
	netIP srv_ip = mHostIP;

	// Create new socket
	int srv_sock_tcp = netAddSocket ( NET_SRV, NET_TCP, NET_CONNECT, false,
										NetAddr(NET_CONNECT, srv_name, srv_ip, srv_port), 
										NetAddr(NET_CONNECT, "", cli_ip, cli_port) );

	NetSock& s = mSockets[srv_sock_tcp];
	s.socket = newSOCK;			// assign literal socket
	s.dest.ipL = cli_ip;		// assign client IP
	s.dest.port = cli_port;		// assign client port
	s.status = NET_CONNECTED;	// connected

	// Send TCP connected event to client
	Event e;
	e = netMakeEvent ( 'sOkT', 0 );
	e.attachInt64 ( cli_ip );			// client IP
	e.attachInt64 ( cli_port );			// client port assigned by server!
	e.attachInt64 ( srv_ip );			// server IP
	e.attachInt64 ( srv_port );			// server port
	e.attachInt ( srv_sock_tcp );		// connection ID (goes back to the client)
	netSend ( e, NET_CONNECT, srv_sock_tcp );

	// Inform the user-app (server) of the event
	Event ue = new_event ( 120, 'app ', 'sOkT', 0, mEventPool );	
	ue.attachInt ( srv_sock_tcp );
	ue.attachInt ( -1 );										// cli_sock not known
	ue.startRead ();
	(*mUserEventCallback) ( ue, this );		// send to application

	if (mbVerbose) {
		dbgprintf("  %s %s: Accepted ip %s, port %i on port %d\n", (s.side == NET_CLI) ? "Client" : "Server", getIPStr(srv_ip).c_str(), getIPStr(s.dest.ipL).c_str(), s.dest.port, s.src.port);
		netPrint();
	}
}


//----------------------------------------------------------- NETWORK CLIENT
//
void NetworkSystem::netStartClient ( netPort cli_port )
{
	// Network System is running in client mode
	eventStr_t sys = 'net ';
	mHostType = 'cli ';
	if (mbVerbose) dbgprintf ( "Start Client:\n");

	// Start a TCP listen socket on Client
	netAddSocket ( NET_CLI, NET_TCP, NET_OFF, false, 
					NetAddr(NET_ANY, mHostName, mHostIP, cli_port), NetAddr() );
}
int NetworkSystem::netClientConnectToServer(std::string srv_name, netPort srv_port, bool blocking )
{
	NetSock cs;
	std::string cli_name;
	netIP cli_ip, srv_ip;
	int cli_port, cli_sock_svc, cli_sock_tcp, cli_sock;

	// check server name for dots
	int dots = 0;
	for (int n = 0; n < srv_name.length(); n++)
		if (srv_name.at(n) == '.') dots++;

	if (srv_name.compare("localhost") == 0) {
		// server is localhost
		srv_ip = mHostIP;
	} else if (dots == 3) {
		// three dots, translate srv_name to literal IP		
		srv_ip = getStrToIP(srv_name);
	} else {
		// fewer dots, lookup host name
		// resolve the server address and port
		addrinfo* pAddrInfo;
		char portname[64];
		sprintf(portname, "%d", srv_port);
		int result = getaddrinfo ( srv_name.c_str(), portname, 0, &pAddrInfo );
		if (result != 0) {
			if (mbVerbose) dbgprintf("ERROR: Unable to resolve server name %s.\n", srv_name.c_str() );
			return NET_ERR;
		}	
		// translate addrinfo to IP string
		char ipstr[INET_ADDRSTRLEN];
		for (addrinfo* p = pAddrInfo; p != NULL; p = p->ai_next) {
			struct in_addr* addr;
			if (p->ai_family == AF_INET) {
				struct sockaddr_in* ipv = (struct sockaddr_in*)p->ai_addr;
				addr = &(ipv->sin_addr);
			}
			else {
				struct sockaddr_in6* ipv6 = (struct sockaddr_in6*)p->ai_addr;
				addr = (struct in_addr*) & (ipv6->sin6_addr);
			}
			inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);
		}		
		srv_ip = getStrToIP(ipstr);
	}

	// find a local TCP socket service
	cli_sock_svc = netFindSocket ( NET_CLI, NET_TCP, NET_ANY );
	cs			= getSock(cli_sock_svc);
	cli_name	= cs.src.name;
	cli_ip		= mHostIP;
	cli_port	= cs.src.port;

	// find or create a socket
	cli_sock_tcp = netFindSocket ( NET_CLI, NET_TCP, NetAddr(NET_CONNECT, srv_name, srv_ip, srv_port) );

	if ( cli_sock_tcp == NET_ERR ) {
		// not yet connected

		// create a socket to server (not yet on)
		cli_sock_tcp = netAddSocket ( NET_CLI, NET_TCP, NET_ENABLE, blocking, 
							NetAddr( NET_CONNECT, cli_name, cli_ip, cli_port), NetAddr(NET_CONNECT, srv_name, srv_ip, srv_port) );

		if (cli_sock_tcp == NET_ERR ) {
			if (mbVerbose) dbgprintf ( "ERROR: Unable to add socket.\n");
			return NET_ERR;
		}
	}
	// reuse address
	const char reuse = 1;
	if ( setsockopt( mSockets[cli_sock_tcp].socket, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int)) < 0)
		if (mbVerbose) dbgprintf ( "Error: Setting server socket as SO_REUSEADDR.\n" );

	// try to connect
	if ( !netIsConnected ( cli_sock_tcp ) ) {
		int result = netSocketConnect ( cli_sock_tcp );
		if (result !=0 ) netReportError ( result );
	}
	return cli_sock_tcp;		// return socket for this connection
}

int NetworkSystem::netCloseAll ()
{
	for (int n=0; n < mSockets.size(); n++) {
		netCloseConnection ( n );
	}
	if (mbVerbose) netPrint();

	return 1;
}

int NetworkSystem::netCloseConnection ( int sock )
{
	if ( sock < 0 || sock >= mSockets.size() ) return 0;

	if ( mSockets[sock].side==NET_CLI ) {
		if ( mSockets[sock].mode==NET_CONNECT ) {
			// client inform server we're done		
			Event e = netMakeEvent ( 'sExT', 'net ' );
			e.attachUInt ( mSockets[sock].dest.sock );		// server (remote) socket
			e.attachUInt ( sock );							// client (local) socket
			netSend ( e );
			netProcessQueue ();				// process queue once to flush it
		}
	} else {
		// server inform client we're done
		if ( mSockets[sock].mode==NET_CONNECT ) {
			int dest_sock = mSockets[sock].dest.sock;
			Event e = netMakeEvent ( 'cExT', 'net ' );
			e.attachUInt ( mSockets[sock].dest.sock );	// client (remote) socket
			e.attachUInt ( sock );						// server (local) socket
			netSend ( e );
			netProcessQueue ();				// process queue once to flush it
		}
	}

	// terminate local socket
	netTerminateSocket ( sock );	

	return 1;
}

// Process network-related events
void NetworkSystem::netProcessEvents ( Event& e )
{
	switch ( e.getName() ) {
	case 'sOkT': {				// received OK from server. connection complete.

		// Client received accept from server
		int cli_sock = e.getSrcSock();

		// Get connection data from Event
		netIP cli_ip = e.getInt64();
		netPort cli_port = e.getInt64();
		netIP srv_ip = e.getInt64();		// server given in Event payload
		int srv_port = e.getInt64();
		int srv_sock = e.getInt();

		// Update client socket with server socket & client port
		mSockets[cli_sock].status = NET_CONNECTED;		// mark connected
		mSockets[cli_sock].dest.sock = srv_sock;		// assign server socket
		mSockets[cli_sock].src.port = cli_port;			// assign client port from server

		// Verify client and server IPs
		netIP srv_ip_chk = e.getSrcIP();		// source IP from the socket event came on
		netIP cli_ip_chk = mSockets[cli_sock].src.ipL;	// original client IP

		/*
		if ( srv_ip != srv_ip_chk ) {	// srv IP from event. srvchk IP from packet origin
			dbgprintf ( "NET ERROR: srv %s and srvchk %s IP mismatch.\n", getIPStr(srv_ip).c_str(), getIPStr(srv_ip_chk).c_str() );
			exit(-1);
		}
		if ( cli_ip != cli_ip_chk ) {	// cli IP from event. clichk IP from original request
			dbgprintf ( "NET ERROR: cli %s and clichk %s IP mismatch.\n", getIPStr(cli_ip).c_str(), getIPStr(cli_ip_chk).c_str() );
			exit(-1);
		}
		*/

		// Inform the user-app (client) of the event
		Event e = new_event ( 120, 'app ', 'sOkT', 0, mEventPool );
		e.attachInt ( srv_sock );
		e.attachInt ( cli_sock );		
		e.startRead ();
		(*mUserEventCallback) ( e, this );		// send to application

		if (mbVerbose) {
			dbgprintf("  Client:   Linked TCP. %s:%d, sock: %d --> Server: %s:%d, sock: %d\n", getIPStr(cli_ip).c_str(), cli_port, cli_sock, getIPStr(srv_ip).c_str(), srv_port, srv_sock);
			netPrint();
		}

		} break;

	case 'sExT': {			// server recv, Exit TCP from client. sEnT

		int local_sock = e.getUInt ();		// socket to close
		int remote_sock = e.getUInt ();		// remote socket

		netIP cli_ip = mSockets[local_sock].dest.ipL;
		if (mbVerbose) dbgprintf ( "  Server: Client closed ok. %s\n", getIPStr(cli_ip).c_str() );

		netTerminateSocket ( local_sock );

		if (mbVerbose) netPrint ();

		} break;
	}
}



//---------------------------------------------- NETWORK CORE - Client & Server
//
void NetworkSystem::netInitialize ()
{
	mCheck = 0;

	if (mbVerbose) dbgprintf ( "Network Initialize.\n");

	// Create an Event Memory Pool
	//mEventPool = new EventPool();
	mEventPool = 0x0;			// NO EVENT POOLING

	// Low-level API
	netStartSocketAPI ();

	// Low-level gethostname
	netGetHostname ();
}

// Add socket (abstracted)
int NetworkSystem::netAddSocket ( int side, int mode, int status, bool block, NetAddr src, NetAddr dest )
{
	NetSock s;
	s.sys = 'net ';
	s.side = side;
	s.mode = mode;
	s.status = status;
	s.src = src;
	s.dest = dest;
	s.socket = 0;
	s.timeout.tv_sec = 0; s.timeout.tv_usec = 0;
	s.blocking = block;
	s.broadcast = 1;

	int n = mSockets.size();
	mSockets.push_back ( s );
	netUpdateSocket ( n );

	return n;
}


#ifdef __linux__
	int clearSocketError ( int fd ) {
	   int err = 1;
	   socklen_t len = sizeof err;
	   if (-1 == getsockopt(fd, SOL_SOCKET, SO_ERROR, (char *)&err, &len))
		  printf ( "getSO_ERROR" );
	   if (err)
		  errno = err;              // set errno to the socket SO_ERROR
	   return err;
	}
#endif

// Terminate Socket
// Note: This does not erase the socket from std::vector because we don't want to
// shift around the other socket IDs. Instead it disables the socket ID, making it available
// to another client later. Only the very last socket could be actually removed from list.

int NetworkSystem::netTerminateSocket ( int sock )
{
	if ( sock < 0 || sock >= mSockets.size() ) return 0;

	if (mbVerbose) dbgprintf ( "netTerminating: %d\n", sock );

	if ( mSockets[sock].status != NET_CONNECT && mSockets[sock].status != NET_CONNECTED ) return 0;

	// close the socket
	NetSock* s = &mSockets[sock];

	#ifdef _WIN32
		shutdown ( s->socket, SD_BOTH );					
		closesocket ( s->socket );
	#else
		clearSocketError ( s->socket );
		shutdown ( s->socket, SHUT_RDWR );				
		close ( s->socket );
	#endif

	// mark as terminated
	mSockets[sock].status = NET_TERMINATED;

	// remove sockets at end of list
	// --- FOR NOW, THIS IS NECESSARY ON CLIENT (which may have only 1 socket),
	// BUT IN FUTURE CLIENTS SHOULD BE ABLE TO HAVE ANY NUMBER OF PREVIOUSLY TERMINATED SOCKETS
	if ( mSockets.size() > 0 ) {
		while ( mSockets[ mSockets.size()-1 ].status == NET_TERMINATED )
			mSockets.erase ( mSockets.end()-1 );
	}
	
	// inform the app
	Event e = new_event(120, 'app ', 'cFIN', 0, mEventPool);
	e.attachInt(sock);
	e.startRead();
	(*mUserEventCallback) (e, this);		// send to application

	return 1;
}


// Handle incoming events, Client or Server
// this function dispatches either to the application-level callback,
// or the the network system event handler based on the Event system target
int NetworkSystem::netEventCallback ( Event& e )
{
	eventStr_t	sys = e.getTarget ();				// target system

	// Application should handle event
	if ( sys != 'net ' ) {								// not intended for network system
		if ( mUserEventCallback != 0x0 )				// pass user events to application
			return (*mUserEventCallback) ( e, this );
	}
	// Network system should handle event
	netProcessEvents ( e );

	return 0;		// only return >0 on user event completion
}

void NetworkSystem::netReportError ( int result )
{
	// create a network error event and set it to the user
	Event e = netMakeEvent ( 'nerr', 'net ' );
	e.attachInt ( result );
	e.startRead();
	(*mUserEventCallback) ( e, this );
}

// Process Queue
int NetworkSystem::netProcessQueue (void)
{
	// Recieve incoming data
	netRecieveData ();

	// Handle incoming events on queue
	int iOk = 0;

	Event e;

	while ( mEventQueue.size() > 0 ) {
		e = mEventQueue.front ();
		e.startRead ();
		iOk += netEventCallback ( e );	// count each user event handled ok
		mEventQueue.pop ();				// pop causes event & payload deletion!
		e.bOwn = false;
	}
	return iOk;
}

// Receive Data
int NetworkSystem::netRecieveData ()
{
	if ( mSockets.size() == 0 ) return 0;

	bool bDeserial;
	int event_alloc;
	int curr_socket;
	int result, maxfd=-1;

	// Get all sockets that are Enabled or Connected
	FD_ZERO (&sock_set);
	for (int n=0; n < (int) mSockets.size(); n++) {
		if ( mSockets[n].status != NET_OFF && mSockets[n].status != NET_TERMINATED ) {		// look for NET_ENABLE or NET_CONNECT
			FD_SET (mSockets[n].socket, &sock_set);
			if ( (int) mSockets[n].socket > maxfd ) maxfd = mSockets[n].socket;
		}
	}
	maxfd++;
	if ( maxfd == 0 ) return 0; // no sockets
	//if ( sock_set.fd_count == 0 ) return 0;		// no sockets

	// Select all sockets that have changed
	result = select ( maxfd, &sock_set, NULL, NULL, &mSockets[0].timeout );

	if (result <0 ) {
		// Select failed. Report net error
		netReportError ( result );
		return 0;
	}
	// Select ok.
	// Find next updated socket
	curr_socket = 0;
	for (; curr_socket != (int) mSockets.size() && !FD_ISSET( mSockets[curr_socket].socket, &sock_set); )
		curr_socket++;

	// Check on valid socket. Silent error if not.
	if (curr_socket >= mSockets.size())
		return 0;

	// Listen for TCP connections on socket
	if ( mSockets[curr_socket].src.type == NET_ANY ) {
		netServerListen ( curr_socket );
	}

	// Receive incoming data on socket
	result = netSocketRecv ( curr_socket, mBuffer, NET_BUFSIZE-1, mBufferLen );
	if ( result!=0 || mBufferLen==0 ) {
		netReportError ( result );		// Recv failed. Report net error
		return 0;
	}

	// Data packet found. mBufferLen > 0
	mBufferPtr = &mBuffer[0];

	while ( mBufferLen > 0 ) {

		if ( mEvent.isEmpty() ) {

			// Check the type of incoming socket
			if (mSockets[curr_socket].blocking) {

				// Blocking socket. NOT an Event socket.
				// Attach arbitrary data onto a new event.
				mEventLen = mBufferLen;
				mEvent = new_event(mEventLen + 128, 'app ', 'HTTP', 0, mEventPool);
				mEvent.rescope("nets");
				mEvent.attachInt(mBufferLen);				// attachInt+Buf = attachStr
				mEvent.attachBuf(mBufferPtr, mBufferLen);
				mDataLen = mEvent.mDataLen;

			} else {
				// Non-blocking socket. Receive a complete Event.
				// directly read length-of-event info from incoming data (mDataLen value)
				mDataLen = *((int*) (mBufferPtr + Event::staticOffsetLenInfo() ));

				// compute total event length, including header
				mEventLen = mDataLen + Event::staticSerializedHeaderSize();

				if ( mDataLen == 0 ) {					
					// dbgprintf ( "WARNING: Received event with 0 payload.\n");
				}
				// Event is allocated with no name/target as this will be set during deserialize
				mEvent = new_event( mDataLen, 0, 0, 0, mEventPool);
				mEvent.rescope("nets");		// belongs to network now
				// Deserialize of actual buffer length (EventLen or BufferLen)
				mEvent.deserialize(mBufferPtr, imin(mEventLen, mBufferLen));	// Deserialize header
			}
			mEvent.setSrcSock(curr_socket);		// <--- tag event /w socket
			mEvent.setSrcIP(mSockets[curr_socket].src.ipL); // recover sender address from socket
			bDeserial = true;
		} else {
			// More data for existing Event..
			bDeserial = false;
		}

		// BufferLen = actual bytes received at this time (may be partial)
		// EventLen = size of event in *network*, serialized event including data payload
		//    bufferLen > eventLen      multiple events
		//    bufferLen = eventLen      one event, or end of event
		//    bufferLen < eventLen 		part of large event

		if ( mBufferLen >= mEventLen ) {

			// One event, multiple, or end of large event..
			if ( !bDeserial )		// not start of event, attach more data
				mEvent.attachBuf ( mBufferPtr, mBufferLen );
			mBufferLen -= mEventLen;			// advance buffer
			mBufferPtr += mEventLen;
			mEventLen = 0;
			int hsz = Event::staticSerializedHeaderSize();
			if ( mbDebugNet ) {
				if (mbVerbose) dbgprintf( "recv: %d bytes, %s\n", mEvent.mDataLen + hsz, mEvent.getNameStr().c_str() );
			}
			// confirm final size received matches indicated payload size
			if ( mEvent.mDataLen != mDataLen ) {
				if (mbVerbose) dbgprintf( "ERROR: Event recv length %d does not match expected %d.\n", mEvent.mDataLen + hsz, mEventLen + hsz);
			}
			netQueueEvent ( mEvent );
			delete_event ( mEvent );

		} else {
			// Partial event..
			if ( !bDeserial )		// not start of event, attach more data
				mEvent.attachBuf ( mBufferPtr, mBufferLen );
			mEventLen -= mBufferLen;
			mBufferPtr += mBufferLen;
			mBufferLen = 0;
		}
	}	// end while

	return mBufferLen;
}

// Put event onto Event Queue
void NetworkSystem::netQueueEvent ( Event& e )
{
	Event eq;
	eq = e;						// eq now owns the data
	eq.rescope ( "nets" );		
	mEventQueue.push ( eq );	// data payload is owned by queued event

	eq.bOwn = false;			// local ref no longer owns payload
	e.bOwn = false;				// source ref no longer owns payload
}

// Sent Event over network
bool NetworkSystem::netSend ( Event& e )
{
	// find a fully-connected socket
	int sock = netFindOutgoingSocket ( true );
	if (sock==-1) { 
		if (mbVerbose) dbgprintf ("Unable to find outgoing socket.\n");
		netReportError ( 111 );		// return disconnection error
		return false; 
	}

	//dbgprintf ( "%s send: name %s, len %d (%d data)\n", nameToStr(mHostType).c_str(), nameToStr(e->getName()).c_str(), e->getEventLength(), e->getDataLength() );

	netSend ( e, NET_CONNECT, sock );

	return true;
}

Event NetworkSystem::netMakeEvent ( eventStr_t name, eventStr_t sys )
{
	Event e = new_event ( 120, sys, name, 0, mEventPool  );
	e.setSrcIP ( mHostIP );		// default to local IP if protocol doesn't transmit sender
	e.setTarget ( 'net ' );		// all network configure events have a 'net ' target name
	e.setName ( name );
	e.startWrite ();
	e.bOwn = false;		// dont kill on destructor
	return e;
}

// Find socket by mode & type
int NetworkSystem::netFindSocket ( int side, int mode, int type )
{
	for (int n=0; n < mSockets.size(); n++)
		if ( mSockets[n].mode == mode && mSockets[n].side == side && mSockets[n].src.type==type )
			return n;
	return -1;
}

// Find socket with specific destination
int NetworkSystem::netFindSocket ( int side, int mode, NetAddr dest )
{
	for (int n=0; n < mSockets.size(); n++)
		if ( mSockets[n].mode == mode && mSockets[n].side == side && mSockets[n].dest.type==dest.type &&
			 mSockets[n].dest.ipL == dest.ipL && mSockets[n].dest.port == dest.port )
				return n;
	return -1;
}

// Find first fully-connected outgoing socket
int NetworkSystem::netFindOutgoingSocket ( bool bTcp )
{
	for (int n=0; n < mSockets.size(); n++) {
		if ( mSockets[n].mode==NET_TCP && mSockets[n].status==NET_CONNECTED ) {
			return n;
		}
	}
	return -1;
}
// Returnt true if any complete connection is valid
bool NetworkSystem::netIsConnected (int sock)
{
	if (sock < 0 || sock >= mSockets.size()) return false;
	if ( mSockets[sock].status == NET_CONNECTED )
		return true;
	return false;
}

std::string NetworkSystem::netPrintAddr ( NetAddr adr )
{
	char buf[128];
	std::string type;
	switch ( adr.type ) {
	case NET_ANY:			type = "any  ";	break;
	case NET_BROADCAST:		type = "broad";	break;
	case NET_SEARCH:		type = "srch";	break;
	case NET_CONNECT:		type = "conn";	break;
	};
	sprintf ( buf, "%s,%s:%d", type.c_str(), getIPStr(adr.ipL).c_str(), adr.port );
	return buf;
}

// Print the network
void NetworkSystem::netPrint ()
{
	if (!mbVerbose) return;

	std::string side, mode, stat, src, dst, msg;

	dbgprintf ( "\n------ NETWORK SOCKETS. MyIP: %s, %s\n", mHostName.c_str(), getIPStr(mHostIP).c_str() );
	for (int n=0; n < mSockets.size(); n++) {
		side = (mSockets[n].side==NET_CLI) ? "cli" : "srv";
		mode = (mSockets[n].mode==NET_TCP) ? "tcp" : "udp";
		switch ( mSockets[n].status ) {
		case NET_OFF:		stat = "off      ";	break;
		case NET_ENABLE:	stat = "enable   "; break;
		case NET_CONNECTED:	stat = "connected"; break;
		case NET_TERMINATED: stat = "terminatd";	break;
		};
		src = netPrintAddr ( mSockets[n].src );
		dst = netPrintAddr ( mSockets[n].dest );
		msg = "";
		if (mSockets[n].side==NET_CLI && mSockets[n].status==NET_CONNECTED )
			msg = "<-- Direct Client-to-Server";
		if (mSockets[n].side==NET_SRV && mSockets[n].status==NET_CONNECTED )
			msg = "<-- Direct Server-to-Client";
		if (mSockets[n].side==NET_SRV && mSockets[n].status==NET_ENABLE && mSockets[n].src.ipL == 0 )
			msg = "<-- Server Listening Port";

		dbgprintf ( "%d: %s %s %s src[%s] dst[%s] %s\n", n, side.c_str(), mode.c_str(), stat.c_str(), src.c_str(), dst.c_str(), msg.c_str() );
	}
	dbgprintf ( "------\n");
}


//----------------------------------------------- NETWORK LOW-LEVEL SOCKETS
// This section provides platform-specific
// wrappers around the socket functions.
//

// start a low-level socket API
void NetworkSystem::netStartSocketAPI ()
{
	FD_ZERO (&sock_set);

#ifdef _MSC_VER
	// Winsock startup
	WSADATA WSAData;
	int status;
	if ((status = WSAStartup(MAKEWORD(1,1), &WSAData)) == 0) {
		if (mbVerbose) dbgprintf ( "  Started Winsock.\n");
	} else {
		if (mbVerbose) dbgprintf ( "  ERROR: Unable to start Winsock.\n");
	}

#else   // ANDROID and linux


        int sock;
        struct sockaddr_in serv_addr;
        char c;

        sock = (socket(AF_INET, SOCK_STREAM, 0));

        if ( sock == -1 ) {
	      dbgprintf ( "  ERROR: Unable to create sockets.\n");
        }

	// BSD Sockets
	dbgprintf ( "  Started BSD sockets.\n");
#endif
}

// get hostname
void NetworkSystem::netGetHostname ()
{
	struct hostent *phe;
	struct in_addr addr;
	char name[512];


	if ( gethostname(name, sizeof(name)) != 0 ) {
		if (mbVerbose) dbgprintf ( "  net", "Cannot get local host name." );
	}
   #ifdef _WIN32
	//----- NOTE: Host may have multiple interfaces (-Marty)
	// This is just to get one valid local IP address
	phe = gethostbyname( name );
	if (phe == 0) {
		if (mbVerbose) dbgprintf ( "  ERROR: Bad host lookup." );
	}
	for (int i = 0; phe->h_addr_list[i] != 0; ++i) {
		memcpy(&addr, phe->h_addr_list[i], sizeof(struct in_addr));
                mHostIP = addr.S_un.S_addr;
	}
   #else
        int sock;
	struct ifreq ifreqs[20];
	struct ifconf ic;

	ic.ifc_len = sizeof (ifreqs);
	ic.ifc_req = ifreqs;

	sock = socket( AF_INET, SOCK_DGRAM, 0);
	if ( sock<0 ) {
	  dbgprintf ("  ERROR: Cannot create socket to get host name.\n");
	}
	if (ioctl( sock, SIOCGIFCONF, &ic) < 0 ) {
	  dbgprintf ("  ERROR: Cannot do ioctl to get host name.\n");
	}

       	for (int i=0; i  < ic.ifc_len / sizeof(struct ifreq); i++) {
	  netIP ip = (netIP) ((struct sockaddr_in*) &ifreqs[i].ifr_addr)->sin_addr.s_addr;
	  dbgprintf ( " %s: %s\n", ifreqs[i].ifr_name, getIPStr(ip).c_str() );
	  if ( ifreqs[i].ifr_name[0] != 'l' ) {  // skip loopback, get first eth0
	    mHostIP = ip;
	    break;
	  }
	}
   #endif

	mHostName = name;
	if (mbVerbose) dbgprintf ( "  Local Host: %s, %s\n", mHostName.c_str(), getIPStr(mHostIP).c_str() );

}

bool NetworkSystem::netSendLiteral ( std::string str, int sock )
{
	int len = str.length();
	char* buf = (char*) malloc(str.length() + 1);
	strcpy(buf, str.c_str());	
	
	// send over socket
	int result;
	NetSock& s = mSockets[sock];
	if (mSockets[sock].mode == NET_TCP) {
		result = send(s.socket, buf, len, 0);		// TCP/IP
	}
	else {
		int addr_size = sizeof(mSockets[sock].dest.addr);
		result = sendto(s.socket, buf, len, 0, (sockaddr*)&s.dest.addr, addr_size);		// UDP
	}
	free(buf);

	// error checking
	#ifdef _MSC_VER
		if (result == SOCKET_ERROR) netError("Net Send Literal error");
	#else
		if (result < 0) netError("Net Send error");
	#endif

	return true;
}

// socket send()
bool NetworkSystem::netSend ( Event& e, int mode, int sock )
{
	if ( sock==0 ) {	// caller wishes to send on any outgoing socket
		sock = netFindOutgoingSocket ( true );
		if ( sock==-1 ) return false;
	}
	int result;
	e.rescope ( "nets" );

	if ( e.mData==0x0 ) return false;

	// prepare serialized buffer
	e.serialize();
	char* buf = e.getSerializedData();
	int len = e.getSerializedLength();
	if ( mbDebugNet ) {
		if (mbVerbose) dbgprintf( "send: %d bytes, %s\n", e.getSerializedLength(), e.getNameStr().c_str() );
	}

	// send over socket
	NetSock& s = mSockets[sock];
	if ( mSockets[sock].mode==NET_TCP ) {
		result = send ( s.socket, buf, len, 0 );		// TCP/IP
	} else {
		int addr_size = sizeof( mSockets[sock].dest.addr );
		result = sendto ( s.socket, buf, len, 0, (sockaddr*) &s.dest.addr, addr_size);		// UDP
	}
	// error checking
	#ifdef _MSC_VER
		if ( result == SOCKET_ERROR ) netError ( "Net Send error");
	#else
		if ( result < 0 ) netError ( "Net Send error" );
	#endif

	return true;
}

// update socket. handles all platform-specific address translation
int NetworkSystem::netUpdateSocket ( int sock )
{
	int ret;
	NetSock& s = mSockets[sock];	    
	unsigned long ioval = (s.blocking ? 0 : 1);   // 0=blocking, 1=non-blocking
	int optval;

	if ( s.status==NET_OFF ) return 0;

	//---- create socket
	if ( s.socket==0 ) {
		if ( s.mode==NET_TCP ) {
			s.socket = socket ( AF_INET, SOCK_STREAM, IPPROTO_TCP);		// TCP socket
		} else {
			s.socket = socket ( AF_INET, SOCK_DGRAM, IPPROTO_UDP);		// UDP socket
		}
	}

	//---- source address update
#ifdef _WIN32
	//---- Windows
	switch (s.src.type) {
	case NET_BROADCAST:	s.src.ip.S_un.S_addr = htonl(INADDR_BROADCAST);	optval = 1;	break;
	case NET_ANY:		s.src.ip.S_un.S_addr = htonl(INADDR_ANY); optval = 0;		break;
	case NET_CONNECT:	s.src.ip.S_un.S_addr = s.src.ipL; optval = 0;				break;
	};
	if ( s.src.type != NET_OFF ) {
		if ( s.broadcast )
			ret = setsockopt ( s.socket, SOL_SOCKET, SO_BROADCAST,  (const char*) &optval, sizeof(optval));		

		ioctlsocket ( s.socket, FIONBIO, &ioval);	// FIONBIO = non-blocking mode		
	}
#else
	//------ Linux/Other
	switch (s.mode) {
	case NET_BROADCAST: s.src.ip.s_addr =  htonl(INADDR_BROADCAST); optval = 1;	break;
	case NET_ANY:		s.src.ip.s_addr = htonl(INADDR_ANY); optval = 0;		break;
	case NET_CONNECT:	s.src.ip.s_addr = s.src.ipL; optval = 0;				break;
	}
	if ( s.src.type != NET_OFF ) {
		ret = setsockopt ( s.socket, SOL_SOCKET, SO_BROADCAST,  (const char*) &optval, sizeof(optval));
		//if ( ret < 0 ) netError ( "Cannot set socket opt" );
		ret = ioctl ( s.socket, FIONBIO, &ioval);
		//if ( ret < 0 ) netError ( "Cannot set socket ctrl" );
	}
#endif
	s.src.addr.sin_family = AF_INET;
	s.src.addr.sin_port = htons( s.src.port );
	s.src.addr.sin_addr = s.src.ip;
	memset (s.src.addr.sin_zero, 0, sizeof (s.src.addr.sin_zero));

	//---- dest address update
#ifdef _WIN32
	//---- Windows
	switch (s.dest.type) {
	case NET_BROADCAST:	s.dest.ip.S_un.S_addr = htonl(INADDR_BROADCAST); optval = 1;	break;
	case NET_ANY:		s.dest.ip.S_un.S_addr = htonl(INADDR_ANY); optval = 0;		break;
	case NET_CONNECT:	s.dest.ip.S_un.S_addr = s.dest.ipL; optval = 0;				break;
	};
	if ( s.dest.type != NET_OFF ) {
		if (s.broadcast)
			ret = setsockopt ( s.socket, SOL_SOCKET, SO_BROADCAST,  (const char*) &optval, sizeof(optval));		

		ioctlsocket ( s.socket, FIONBIO, &ioval);	 // FIONBIO = non-blocking mode		
	}
#else
	//------ Linux/Other
	switch (s.dest.type) {
	case NET_BROADCAST: s.dest.ip.s_addr =  htonl(INADDR_BROADCAST); optval = 1;	break;
	case NET_ANY:		s.dest.ip.s_addr = htonl(INADDR_ANY); optval = 0;		    break;
	case NET_CONNECT:	s.dest.ip.s_addr = s.dest.ipL; optval = 0;				    break;
	}
	if ( s.dest.type != NET_OFF ) {
		ret = setsockopt ( s.socket, SOL_SOCKET, SO_BROADCAST,  (const char*) &optval, sizeof(optval));
		//if ( ret < 0 ) netError ( "Cannot set socket opt" );

		ret = ioctl ( s.socket, FIONBIO, &ioval);
		//if ( ret < 0 ) netError ( "Cannot set socket ctrl" );
	}
#endif
	s.dest.addr.sin_family = AF_INET;
	s.dest.addr.sin_port = htons( s.dest.port );
	s.dest.addr.sin_addr = s.dest.ip;
	memset (s.dest.addr.sin_zero, 0, sizeof (s.dest.addr.sin_zero));

	return 1;
}

// socket connect()
int NetworkSystem::netSocketConnect ( int sock )
{
	NetSock* s = &mSockets[sock];
	int addr_size = sizeof ( s->dest.addr );
	int result;

	if (mbVerbose) dbgprintf ("  %s connect: ip %s, port %i\n", (s->side==NET_CLI) ? "cli" : "srv", getIPStr(s->dest.ipL).c_str(), s->dest.port);

	result = connect ( s->socket, (sockaddr*) &s->dest.addr, addr_size );

	if (result < 0) {
		return netError ( "Connect error" );
	}	

	return 0;
}

// socket bind()
int NetworkSystem::netSocketBind ( int sock )
{
	NetSock* s = &mSockets[sock];
	int addr_size = sizeof ( s->src.addr );

	if (mbVerbose) dbgprintf("  bind: %s, port %i\n", (s->side==NET_CLI) ? "cli" : "srv", s->src.port);
	int result = bind ( s->socket, (sockaddr*) &s->src.addr, addr_size );

	if ( netIsError(result) )
		netError ( "Cannot bind to source.");

	return result;
}


// socket accept()
int NetworkSystem::netSocketAccept ( int sock, SOCKET& tcp_sock, netIP& cli_ip, netPort& cli_port )
{
	NetSock& s = mSockets[sock];
	struct sockaddr_in sin;
	int addr_size = sizeof (sin);

	// socket accept
	tcp_sock = accept ( s.socket, (sockaddr*) &sin, (socklen_t *) (&addr_size) );

#ifdef _WIN32
	if ( tcp_sock == INVALID_SOCKET ) {
#else
	if ( tcp_sock < 0) {
#endif
		netError ( "TCP Accept error" );
		return -1;
	}
	
	cli_ip = sin.sin_addr.s_addr;		// IP address of connecting client
	cli_port = sin.sin_port;			// accepting TCP does not know/care what the client port is

	return 1;
}

// socket listen()
int NetworkSystem::netSocketListen ( int sock )
{
	NetSock& s = mSockets[sock];

	if (mbVerbose) dbgprintf( "  listen: port %i\n", s.src.port);

	int result = listen ( s.socket, SOMAXCONN );

	#ifdef _WIN32
		if (result==SOCKET_ERROR) netError ( "TCP Listen error\n" );
	#else
		if (result<0) netError ( "TCP Listen error\n" );
	#endif
	return result;
}

// socket recv()
// return value: success=0, or an error in errno.h.
// on success recvlen is set to bytes recieved
int NetworkSystem::netSocketRecv ( int sock, char* buf, int buflen, int& recvlen )
{
	#ifdef _WIN32
		int addr_size;
	#else
		socklen_t addr_size;
	#endif
	int result;
	NetSock& s = mSockets[sock];
	if ( s.src.type != NET_CONNECT ) return 0;		// recv only on connection sockets

	addr_size = sizeof ( s.src.addr );
	if ( s.mode==NET_TCP ) {
		result = recv ( s.socket, buf, buflen, 0 );		// TCP/IP
	} else {
		result = recvfrom ( s.socket, buf, buflen, 0, (sockaddr*) &s.src.addr, &addr_size );	// UDP
	}
	if (result==0) {
		// peer has shutdown (orderly shutdown)
		netTerminateSocket ( sock );
		netError ( "Orderly shutdown");
		return ECONNREFUSED;
	}
	#ifdef _WIN32
		if ( result == SOCKET_ERROR ) 
			return netError ( "Recv error" );
	#else
		if ( result < 0 )
			return netError ( "Recv error" );
    #endif
	recvlen = result;
	return 0;
}

// API-specific error checking
int NetworkSystem::netError ( std::string msg )
{
	char buf[512];			
	char* error;

#ifdef _WIN32
	int errn = WSAGetLastError();		// windows get error
    error = strerror ( errn );	
#else	
	error = (char*) strerror_r ( errno, buf, 512 );   // linux get error
#endif
	buf[511] = '\0';	// safety

	if (mbVerbose) dbgprintf ( "NET ERROR: %s: %s (%d)\n", msg.c_str(), error, errno );
	return errno;
}

// API-specific check for error
bool NetworkSystem::netIsError ( int result )
{
	#ifdef _WIN32
		if ( result == SOCKET_ERROR ) return true;		// windows error
	#else
		if ( result < 0 ) return true;					// linux/other error
	#endif
	return false;
}

std::string NetworkSystem::getIPStr ( netIP ip )
{
	char ipname[1024];
	in_addr addr;
	#ifdef _MSC_VER
		addr.S_un.S_addr = ip;
	#else
		addr.s_addr = ip;
	#endif
	sprintf (ipname, "%s", inet_ntoa ( addr ) );
	return std::string ( ipname );
}

netIP NetworkSystem::getStrToIP ( std::string name )
{
	char ipname[1024];
	strcpy ( ipname, name.c_str() );
	return inet_addr ( ipname );
}

/*void NetworkSystem::setMaxPacketLen ( int sock )
{
	int optval;

	#ifdef _MSC_VER
		int optlen = sizeof(int);
		int result = getsockopt ( mSockList[sock]->GetSocket(), SOL_SOCKET, SO_MAX_MSG_SIZE, (char*) &optval, &optlen );
		mMaxPacketLen = optval;
	#else
		mMaxPacketLen = 65535;		// Upper limit for UDP. Not queryable with BSD sockets (?)
	#endif

	// dbgprintf ( "  net", "  Maximum UDP Packet Size: %d\n", mMaxPacketLen);
}*/
