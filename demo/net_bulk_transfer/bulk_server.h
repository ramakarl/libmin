
#ifndef NETDEMO_SERVER
#define NETDEMO_SERVER

#include <unordered_map>
#include "network_system.h"
#include "bulk_main.h"

class Server : public NetworkSystem {
public:		
	Server( const char* trace_file_name = NULL ) : NetworkSystem( trace_file_name ) { }

	// Networking functions
	void Start ( int protocols, int error );
	int Run ( );		
	void Close ( );
	int Process ( Event& e );
	static int NetEventCallback ( Event& e, void* this_ptr );	

	// Demo app protocol
	void ReceiveBulkPkt ( Event& e );
	int InitBuf ( char* buf, const int size, char main_pkt_char );
	double GetUpTime ( );

private:
	std::unordered_map<int, pkt_struct> m_clientData;
	TimeX m_startTime;
	int m_pktSize;
	pkt_struct m_rxPkt;
	FILE* m_flowTrace;
};

#endif 
