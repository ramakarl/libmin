
#include "call_client.h"
#include <mutex>

#define ENABLE_SSL

int Client::NetEventCallback ( Event& e, void* this_pointer ) {
    Client* self = static_cast<Client*>( this_pointer );
    return self->Process ( e );
}

void Client::Start ( std::string srv_addr )
{
	m_srvAddr = srv_addr;
	m_hasConnected = false;	
	bool bDebug = false;
	bool bVerbose = true;
	int cli_port = 10000 + rand ( ) % 9000; 
 
	std::cout << netSetSecurityLevel ( NET_SECURITY_PLAIN_TCP | NET_SECURITY_OPENSSL ) << std::endl;	
  //std::cout << netSetSecurityLevel ( NET_SECURITY_PLAIN_TCP ) << std::endl;	
	std::cout << netSetReconnectLimit ( 10 ) << std::endl;
	std::cout << netSetReconnectInterval ( 500 ) << std::endl;
	std::cout << netSetPathToPublicKey ( "server_pubkey.pem" ) << std::endl;

	m_currtime.SetTimeNSec ( ); // Start timer
	m_lasttime = m_currtime;
	m_seq = 0;
	srand ( m_currtime.GetMSec ( ) );
	netInitialize ( ); 
	netShowVerbose ( bVerbose );
	netShowFlow( bVerbose) ;

	netClientStart ( cli_port, srv_addr );
	netSetUserCallback ( &NetEventCallback );
	m_sock = NET_NOT_CONNECTED; // Not yet connected (see Run func)
	
	dbgprintf ( "App. Client IP: %s\n", getIPStr ( getHostIP ( ) ).c_str ( ) );	
}

void Client::Reconnect ( )
{   
	dbgprintf ( "App. Connecting..\n" );	
	m_sock = netClientConnectToServer ( m_srvAddr, 16101, false ); // Reconnect to server	 
}

void Client::Close ( )
{
	netCloseConnection ( m_sock );
}

int Client::Process ( Event& e )
{
	std::string line;
	eventStr_t sys = e.getTarget ( );
	if ( sys == 'net ' && e.getName ( ) =='nerr' ) { // Check for net error events
		// Enable netShowVerbose ( true ) for detailed messages; handle specific net error codes here..		
		int code = e.getInt ( );
		return 0;
	}
	e.startRead ( ); // Process Network events
	switch ( e.getName ( ) ) {
		case 'sOkT': { // Connection complete. server accepted OK.
			int srv_sock = e.getInt ( ); 
			int cli_sock = e.getInt ( ); 
			dbgprintf( "App. Connected to server: %s, %d\n", getSock( cli_sock )->dest.name.c_str ( ), srv_sock );
			return 1;
		} break;	
	};

	switch ( e.getName ( ) ) {
		case 'sRst': { // Process Application events, send back the words
			std::string words = e.getStr ( );
			dbgprintf ( "App. Result from server: %s\n", words.c_str ( ) );
			return 1;
		} 
		case 'sFIN': { // Server shutdown unexpectedly
			dbgprintf ( "App. Server disconnected.\n" );
			return 1;
		} 
	};
	dbgprintf ( "App. Unhandled message: %s\n", e.getNameStr ( ).c_str ( ) );
	return 0;
}

int Client::Run ( ) 
{
	m_currtime.SetTimeNSec ( );	
	float elapsed_sec = m_currtime.GetElapsedSec ( m_lasttime );
	if ( elapsed_sec >= 0.5 ) { // Demo app - request the words for a random number every 2 secs
		m_lasttime = m_currtime;
		if ( netIsConnectComplete ( m_sock ) ) {	
			m_hasConnected = true;		
			int rnum = rand ( ) % 10000;
			RequestWords ( rnum );
			dbgprintf ( "  Requested words for: %d\n", rnum ); // If connected, make request
		} else if ( ! m_hasConnected ) {
			Reconnect ( ); // If disconnected, try and reconnect
			m_hasConnected = true;	
		}
	}
	return netProcessQueue ( ); // Process event queue
}

void Client::RequestWords ( int num )
{	
	// Demo application protocol:
	// cRqs - request the words for a number (c=msg from client)
	// sRst - here is the result containing the words (s=msg from server)
	int srv_sock = getServerSock ( m_sock ); // Create cRqs app event
	if ( srv_sock >= 0 ) {
		Event e = new_event ( 120, 'app ', 'cRqs', 0, getNetPool ( ) );	
		e.attachInt ( srv_sock ); // Must always tell server which socket 
		e.attachInt ( m_seq++ ); 		
		e.attachInt ( num ); // Payload		
		netSend ( e ); // Send to server
	}
}

