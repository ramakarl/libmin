
#include "bulk_server.h"

#define FLOW_FLUSH

int Server::NetEventCallback ( Event& e, void* this_pointer ) 
{
    Server* self = static_cast<Server*>( this_pointer );
    return self->Process ( e );
}

int Server::InitBuf ( char* buf, const int size, char main_pkt_char ) 
{
  for ( int i = 0, c = 65; i < size; i++ ) {
    if ( i == size - 1 ) {
      memset ( buf + i, '*', 1 );
      memset ( buf + i + 1, '\0', 1 );
    }
    else if ( i % 50 == 49 ) {
      memset ( buf + i, '\n', 1 );
      c++;
    }
    else if ( i % 10 == 9 ) {
      memset ( buf + i, c, 1 );
    }
    else {
      memset ( buf + i, main_pkt_char, 1 );
    }
  }
  netPrintf ( PRINT_VERBOSE, "*** Packet content:\n\n%s\n*** Size is %luB \n", buf, strlen ( buf ) );
  return (int)strlen ( buf );
}

double Server::GetUpTime ( ) 
{
	TimeX current_time;
	current_time.SetTimeNSec ( );
	return current_time.GetElapsedSec ( m_startTime );
}

void Server::Start ( int protocols, int error )
{
	m_startTime.SetTimeNSec ( );
	m_flowTrace = fopen ( "../tcp-app-rx-flow", "w" );
	pkt_struct client_data;
	int srv_port = 16101;

	if ( protocols == PROTOCOL_TCP_ONLY ) {
		dbgprintf ( "Using TCP only \n" );
		srv_port--;
		netSetSecurityLevel ( NET_SECURITY_PLAIN_TCP );	
		netSetReconnectLimit ( 10 );
	} else if ( protocols == PROTOCOL_SSL_ONLY ) {	
		dbgprintf ( "Using OpenSSL only \n" );
		netSetSecurityLevel ( NET_SECURITY_OPENSSL );	
		netSetReconnectLimit ( 10 );
		if ( error == ERROR_MISSING_SERVER_KEYS ) {
			netSetPathToPublicKey ( "server_pubkey.pem" );
		} else {
			netSetPathToPublicKey ( "server_pubkey.pem" );
			netSetPathToPrivateKey ( "server_private.pem" );
		}
	} else {
		dbgprintf ( "Using TCP and OpenSSL \n" );
		netSetSecurityLevel ( NET_SECURITY_PLAIN_TCP | NET_SECURITY_OPENSSL );	
		netSetReconnectLimit ( 10 );
		if ( error == ERROR_MISSING_SERVER_KEYS ) {
			netSetPathToPublicKey ( "server_pubkey.pem" );
		} else {
			netSetPathToPublicKey ( "server_pubkey.pem" );
			netSetPathToPrivateKey ( "server_private.pem" );
		}
	}


	netInitialize ( ); // Start networking
	netShowFlow( false );
	netShowVerbose( true );
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

void Server::ReceiveBulkPkt ( Event& e )
{
	int sock = e.getInt ( ); // Which client 
	int pktSize = e.getInt ( );

	if ( m_clientData.find( sock ) == m_clientData.end ( ) ) {
		printf ( "*** Init state for server sock %d \n", sock );
		char main_pkt_char = ( sock % 2 == 0 ) ? '-' : '=';
		pkt_struct client_data;
		m_pktSize = InitBuf ( client_data.buf, PKT_SIZE, main_pkt_char ) + sizeof ( int );
		client_data.seq_nr = 1;
		m_clientData[ sock ] = client_data;
	}
	e.getBuf ( (char*) &m_rxPkt, pktSize );
	if ( m_rxPkt.seq_nr == 1 ) {
		m_clientData[ sock ].seq_nr = 1;
	}
	int outcome = memcmp ( &m_clientData[ sock ], &m_rxPkt, pktSize - 4 );
	m_clientData[ sock ].seq_nr++;
	fprintf ( m_flowTrace, "%.3f:%u:%u:o:%d\n", GetUpTime ( ), m_rxPkt.seq_nr, pktSize, outcome );
	printf ( "%d:%d:%d\n", m_rxPkt.seq_nr, pktSize, sock );

	#ifdef FLOW_FLUSH
		fflush ( m_flowTrace );
	#endif	

	if ( outcome != 0 ) {
		std::cout << "\n=========================================== " << m_rxPkt.seq_nr << " \n" << std::endl;
		std::cout.write( m_rxPkt.buf, pktSize - sizeof ( int ) );
		std::cout << "\n=========================================== " << m_clientData[ sock ].seq_nr << " \n" << std::endl;
		std::cout.write( m_clientData[ sock ].buf, pktSize - sizeof ( int ) );
		std::cout << "\n=========================================== " << outcome << " \n" << std::endl;
		std::cin.get();
	}
	netPrintf ( PRINT_FLOW, "Received event: %d, SEQ-%d", e.getSerializedLength(), m_rxPkt.seq_nr );
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
			dbgprintf ( "    App. SRV Connection to client closed unexpectedly.\n" );
		}		
		return 0;
	}
	e.startRead ( );
	switch ( e.getName ( ) ) { // Process Network events
		case 'sOkT': // Connection to client complete. (telling myself)
			sock = e.getInt ( ); // Server sock		
			dbgprintf ( "    App. SRV Connected to client: #%d\n", sock );
			return 1;
		case 'cFIN': // Client closed connection
			sock = e.getInt();
			dbgprintf ( "    App. SRV Disconnected client: #%d\n", sock );
			return 1;	
	};
	switch ( e.getName ( ) ) { // Process Application events
		case 'cRqs': // Client requested words for num
			ReceiveBulkPkt ( e );
			return 1;
	};
	dbgprintf ( "    App. SRV Unhandled message: %s\n", e.getNameStr ( ).c_str ( ) );
	return 0;
}
