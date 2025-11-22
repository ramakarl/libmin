
#include "bulk_client.h"

#define ENABLE_SSL

int Client::NetEventCallback ( Event& e, void* this_pointer ) 
{
    Client* self = static_cast<Client*>( this_pointer );
    return self->Process ( e );
}

int Client::InitBuf  ( char* buf, const int size, char main_pkt_char ) 
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

double Client::GetUpTime ( ) 
{
	TimeX current_time;
	current_time.SetTimeNSec ( );
	return current_time.GetElapsedSec ( m_startTime );
}

bool Client::TxActive ( )
{
	return m_txPkt.seq_nr < m_pktLimit;
}


void Client::Start ( std::string srv_addr,  int pkt_limit, int protocols, int error )
{
	m_srvAddr = srv_addr;
	m_startTime.SetTimeNSec ( );
	m_flowTrace = fopen ( "../tcp-app-tx-flow", "w" );
	m_hasConnected = false;
	m_pktSize = 0;
	m_pktLimit = pkt_limit;
	m_txPkt.seq_nr = 0;
 
	if ( protocols == PROTOCOL_TCP_ONLY ) {
		dbgprintf ( "Using TCP only \n" );
		m_srvPort = 16100; 
		netSetSecurityLevel ( NET_SECURITY_PLAIN_TCP );	
		netSetReconnectLimit ( 50 );
		netSetReconnectInterval ( 1000 );
	} else if ( protocols == PROTOCOL_SSL_ONLY ) {	
		dbgprintf ( "Using OpenSSL only \n" );
		m_srvPort = 16101; 
		netSetSecurityLevel ( NET_SECURITY_OPENSSL );	
		netSetReconnectLimit ( 50 );
		netSetReconnectInterval ( 1000 );
		if ( error != ERROR_MISSING_CLIENT_KEYS ) {
			netSetPathToPublicKey ( "server_pubkey.pem" );
		} 
	} else {
		dbgprintf ( "Using TCP and OpenSSL \n" );
		m_srvPort = 16101; 
		netSetSecurityLevel ( NET_SECURITY_PLAIN_TCP | NET_SECURITY_OPENSSL );	
		netSetReconnectLimit ( 10 );
		netSetReconnectInterval ( 500 );
		if ( error != ERROR_MISSING_CLIENT_KEYS ) {
			netSetPathToPublicKey ( "server_pubkey.pem" );
		} 
	}

	m_currtime.SetTimeNSec ( ); //  rt timer
	m_lasttime = m_currtime;
	m_seq = 0;
	srand ( m_currtime.GetMSec ( ) );
	netInitialize ( ); 
	netShowFlow( false );
	netShowVerbose( true );
	int cli_port = 10000 + rand ( ) % 9000; 
	netClientStart ( cli_port, srv_addr );
	netSetUserCallback ( &NetEventCallback );
	m_sock = NET_NOT_CONNECTED; // Not yet connected (see Run func)
	
	dbgprintf ( "App. Client IP: %s\n", getIPStr ( getHostIP ( ) ).c_str ( ) );	
}


void Client::Reconnect ( )
{   
	dbgprintf ( "App. Connecting..\n" );	
	m_sock = netClientConnectToServer ( m_srvAddr, m_srvPort, false ); // Reconnect to server	 
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
	e.startRead ( );  // Process Network events
	switch ( e.getName ( ) ) {
		case 'sOkT': { // Connection complete. server accepted OK.
			int srv_sock = e.getInt ( ); 
			int cli_sock = e.getInt ( ); 
			dbgprintf( "    App. CLI Connected to server: %s, %d\n", getSock( cli_sock )->dest.name.c_str ( ), srv_sock );
			return 1;
		} break;	
	};

	switch ( e.getName ( ) ) {
		case 'sRst': { // Process Application events, send back the words
			std::string words = e.getStr ( );
			dbgprintf ( "    App. CLI Result from server: %s\n", words.c_str ( ) );
			return 1;
		} 
		case 'sFIN': { // Server shutdown unexpectedly
			dbgprintf ( "    App. CLI found Server disconnected.\n" );
			return 1;
		} 
	};
	dbgprintf ( "    App. CLI Unhandled message: %s\n", e.getNameStr ( ).c_str ( ) );
	return 0;
}

void Client::SendPackets ( )
{	
	int srv_sock = getServerSock ( m_sock );
	if ( srv_sock == -1 ) {
		return;
	}
	if ( m_pktSize == 0 ) { 
		printf ( "*** Init state for server sock %d \n", srv_sock );
		char main_pkt_char = ( srv_sock % 2 == 0 ) ? '-' : '=';
		m_pktSize = InitBuf ( m_txPkt.buf, PKT_SIZE, main_pkt_char ) + sizeof(int);
		m_txPkt.seq_nr = 1;
	}
	bool outcome = true;
	while ( outcome && m_txPkt.seq_nr < m_pktLimit ) {
		Event e = new_event ( m_pktSize + sizeof(int) * 2, 'app ', 'cRqs', 0, getNetPool ( ) );	
		e.attachInt ( srv_sock ); // Must always tell server which socket
		e.attachInt ( m_pktSize );
		e.attachBuf ( (char*)&m_txPkt, m_pktSize );
		outcome = netSend ( e );
		if ( outcome ) {
			fprintf ( m_flowTrace, "%.3f:%u:%u\n", GetUpTime ( ), m_txPkt.seq_nr, m_pktSize );
			#ifdef FLOW_FLUSH
				fflush ( m_flowTrace );
			#endif	
			m_txPkt.seq_nr++;
		}
		printf ( "%d\n", m_txPkt.seq_nr );
	}
}
int Client::Run ( ) 
{
	// Transmit if bulk packets pending
	if ( TxActive () ) {
		m_currtime.SetTimeNSec ( );	
		float elapsed_sec = m_currtime.GetElapsedSec ( m_lasttime );
	 
		// Transmission rate
		if ( elapsed_sec >= 0.5 ) {		
			m_lasttime = m_currtime;
			if ( netIsConnectComplete ( m_sock ) ) {	
				m_hasConnected = true;		
				SendPackets ( );
			} else if ( ! m_hasConnected ) {
				Reconnect ( ); // If disconnected, try and reconnect
				m_hasConnected = true;	
			}
		}
	}

	// Process event queue 
	return netProcessQueue ( ); 
}

