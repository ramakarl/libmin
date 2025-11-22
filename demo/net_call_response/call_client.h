
#ifndef NETDEMO_CLIENT
#define NETDEMO_CLIENT

#include "network_system.h"

class Client : public NetworkSystem {
public:		
	Client( const char* trace_file_name = NULL ) : NetworkSystem( trace_file_name ) { }

	// Networking functions
	void Start ( std::string srv_addr );
	void Reconnect ( );
	void Close ( );		
	int Run ( );				
	int Process ( Event& e );
	static int NetEventCallback ( Event& e, void* this_ptr );	

	// Demo app protocol
	void RequestWords ( int num );		

private:
	int	m_hasConnected;
	int	m_sock; // This is my socket (local) to access the server
	int	m_seq;
	std::string m_srvAddr;
	TimeX m_currtime;
	TimeX m_lasttime;
};

#endif
