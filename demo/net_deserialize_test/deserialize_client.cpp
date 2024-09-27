
#include "deserialize_client.h"

//#include <openssl/err.h>
//#include <openssl/md5.h>
//#include <openssl/ssl.h>
//#include <openssl/x509v3.h>
#include <mutex>

#define ENABLE_SSL

int Client::NetEventCallback (Event& e, void* this_pointer) {
    Client* self = static_cast<Client*>(this_pointer);
    return self->Process ( e );
}


void Client::Start (std::string srv_addr)
{
	mSrvAddr = srv_addr;
	
	std::cout << netSetSecurityLevel( NET_SECURITY_PLAIN_TCP | NET_SECURITY_OPENSSL) << std::endl;	
	std::cout << netSetReconnectLimit ( 10 ) << std::endl;
	std::cout << netSetReconnectInterval ( 500 ) << std::endl;
	std::cout << netSetPathToPublicKey ( "server_pubkey.pem" ) << std::endl;

	m_currtime.SetTimeNSec ( );	// Start timer
	m_lasttime = m_currtime;
	mSeq = 0;
	srand ( m_currtime.GetMSec ( ) );

	netInitialize ( ); // Start networking
	netShowFlow ( true );
	netShowVerbose ( true );
	int cli_port = 10000 + rand ( ) % 9000;
	netClientStart ( cli_port, srv_addr ); // Start client on random port
	netSetUserCallback ( &NetEventCallback );
	
	dbgprintf ( "App. Client IP: %s\n", getIPStr ( getHostIP ( ) ).c_str ( ) );	

	// not yet connected (see Run func)
	mSock = NET_NOT_CONNECTED; 
}

void Client::Reconnect ( )
{   
	int serverPort = 16101;
	dbgprintf ( "App. Connecting..\n" );	
	mSock = netClientConnectToServer ( mSrvAddr, 16101, false );	
}

void Client::Close ()
{
	netCloseConnection ( mSock );
}


int Client::Process ( Event& e )
{
	std::string line;
	eventStr_t sys = e.getTarget ( );

	// Check for net error events
	if ( sys == 'net ' && e.getName ( ) == 'nerr' ) {
		// enable netVerbose(true) for detailed messages.
		// application can gracefully handle specific net error codes here..		
		int code = e.getInt ();

		return 0;
	}
	// Process Network events
	e.startRead ( );
	switch ( e.getName ( ) ) {
	case 'sOkT': {
		// Connection complete. server accepted OK.
		int srv_sock = e.getInt();		// server sock
		int cli_sock = e.getInt();		// local socket
		dbgprintf( "App. Connected to server: %s, %d\n", getSock( cli_sock )->dest.name.c_str ( ), srv_sock );

		return 1;
	  //case 'sOkT': {
	} break;	
	};

	// Process Application events
	switch (e.getName()) {	
	case 'sFIN': {
		// server shutdown unexpectedly
		dbgprintf ("App. Server disconnected.\n" );
		return 1;
	  } break;
	};
	dbgprintf ( "App. Unhandled message: %s\n", e.getNameStr().c_str() );
	return 0;
}

int Client::Run ()
{
	m_currtime.SetTimeNSec();	

	// demo app - request the words for a random number every 2 secs
	//
	float elapsed_sec = m_currtime.GetElapsedSec ( m_lasttime );
	
	if ( elapsed_sec >= 2.0 ) {
		m_lasttime = m_currtime;
		if ( netIsConnectComplete ( mSock ) ) {	
			
			// transmit multiple events found in test buffer
			TransmitTestBuffer ();

		} else if ( ! mHasConnected ) {

			Reconnect ( ); // If disconnected, try and reconnect
			mHasConnected = true;	
		}
	}

	// process event queue
	return netProcessQueue ();
}

void Client::TransmitTestBuffer ()
{	
	// Test buffer
	// - create multiple events with lengths that vary close to the TCP_PACKET_SIZE
	
	int test_start_sz, test_inc_sz, test_cnt;
	
	// hard test #1
	test_start_sz = TEST_TCP_PACKET_SIZE - 4;
	test_inc_sz = 8;
	test_cnt = 20;
	
	std::string str;
	char buf[65535];
	int header_sz = Event::staticSerializedHeaderSize();	
	int payload_sz;
	int event_sz = test_start_sz;

	for (int i = 0; i < test_cnt; i++) {
				
		// make event
		Event e = new_event( event_sz - header_sz, 'app ', 'cTst', 0, getNetPool());	
		payload_sz = event_sz - header_sz - 2*sizeof(int);		// event = header + sock int + payload + verify_int		
		str = std::string(payload_sz, i + 65);					// make test str of identical chars
		memcpy(buf, str.c_str(), payload_sz);
		e.attachInt ( getServerSock (mSock) );
		e.attachBuf( buf, payload_sz );
		e.attachInt( event_sz );									// put verification length at end of event
		printf("  Client sent: #%d, %d bytes, %s, %.5s\n", i, event_sz, e.getNameStr().c_str(), str.c_str());

		netSend(e);

		event_sz += test_inc_sz;								// continually increase event_sz		
	}

}

