

#include "call_server.h"

#include "network_system.h"

int Server::NetEventCallback ( Event& e, void* this_pointer ) {
    Server* self = static_cast<Server*>( this_pointer );
    return self->Process ( e );
}

void Server::Start ( )
{
	bool bDebug = true;
	bool bVerbose = true;

	std::cout << netSetSecurityLevel ( NET_SECURITY_PLAIN_TCP | NET_SECURITY_OPENSSL ) << std::endl;	
  //std::cout << netSetSecurityLevel ( NET_SECURITY_PLAIN_TCP ) << std::endl;	
	std::cout << netSetReconnectLimit ( 10 ) << std::endl;
	std::cout << netSetPathToPublicKey ( "server_pubkey.pem" ) << std::endl;
	std::cout << netSetPathToPrivateKey ( "server_private.pem" ) << std::endl;
	//std::cout << netSetPathToCertDir ( "/etc/ssl/certs" ) << std::endl;
	//std::cout << netSetPathToCertFile ( "/etc/ssl/certs/ca-certificates.crt" ) << std::endl;

	netInitialize ( ); // Start networking
	netShowVerbose( bVerbose );
  netShowFlow( bVerbose );

	int srv_port = 16101;
	netServerStart ( srv_port ); // Start server listening
	netSetUserCallback ( &NetEventCallback ); 
	
	dbgprintf ( "Server IP: %s\n", getIPStr ( getHostIP() ).c_str() );	
	dbgprintf ( "Listening on %d..\n", srv_port );
}

void Server::Close ( )
{
}

int Server::Run ( )
{
	return netProcessQueue ( ); // Process event queue
}

void Server::InitWords ( )
{
	for ( int n = 0; n < 10; n++ ) {
		wordlist.push_back ( "" );
	}
	wordlist[0] = "zero";
	wordlist[1] = "one";
	wordlist[2] = "two";
	wordlist[3] = "three";
	wordlist[4] = "four";
	wordlist[5] = "five";
	wordlist[6] = "six";
	wordlist[7] = "seven";
	wordlist[8] = "eight";
	wordlist[9] = "nine";	
}

std::string Server::ConvertToWords ( int num ) 
{
	std::string words = "==========";
	int n = num;
	int v;
	while ( n != 0 ) {
		v = n % 10;
		words = wordlist[v] + " " + words;
		n /= 10;
	}
    words = "========== " + words;
	return words; // Demo - this is the main task of the server
}

void Server::SendWordsToClient ( std::string msg, int sock )
{
	// Demo app protocol:
	// Message: cRqs: request the words for a number (c=msg from client)
	// Message: sRst: here is the result containing the words (s=msg from server)
	Event e = new_event ( 120, 'app ', 'sRst', 0, getNetPool ( ) );	// Create sRst app event
	e.attachStr ( msg );
	netSend ( e, sock ); // Send to specific client
}

int Server::Process ( Event& e )
{
	int sock;
	std::string line;
	eventStr_t sys = e.getTarget ( );
	if ( sys == 'net ' && e.getName ( ) == 'nerr' ) { // Check for net error events
		// Enable netShowVerbose ( true ) for detailed messages; handle specific net error codes here..			
		int code = e.getInt ( );		
		if ( code == NET_DISCONNECTED ) {
			dbgprintf ( "  Connection to client closed unexpectedly.\n" );
		}		
		return 0;
	}
	e.startRead ( );
	switch ( e.getName ( ) ) { // Process Network events
		case 'sOkT': // Connection to client complete. (telling myself)
			sock = e.getInt ( ); // Server sock		
			dbgprintf ( "  Connected to client: #%d\n", sock );
			return 1;
		case 'cFIN': // Client closed connection
			sock = e.getInt();
			dbgprintf ( "  Disconnected client: #%d\n", sock );
			return 1;	
	};
	switch ( e.getName ( ) ) { // Process Application events
		case 'cRqs': // Client requested words for num
			int sock = e.getInt ( ); // Which client 
			int seq = e.getInt ( ); 		
			int num = e.getInt ( );	
			std::string words = ConvertToWords ( num ); // Convert the num to words 
			SendWordsToClient ( words, sock ); // Send words back to client
			dbgprintf ( "  Sent words to #%d: SEQ-%d: %d, %s\n", sock, seq, num, words.c_str ( ) );
			return 1;
	};
	dbgprintf ( "   Unhandled message: %s\n", e.getNameStr ( ).c_str ( ) );
	return 0;
}
