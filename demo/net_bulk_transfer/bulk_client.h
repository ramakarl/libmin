
#ifndef NETDEMO_CLIENT
#define NETDEMO_CLIENT

#include "network_system.h"
#include "bulk_main.h"

class Client : public NetworkSystem {
public:		
	Client( const char* trace_file_name = NULL ) : NetworkSystem( trace_file_name ) { }

	// Networking functions
	void Start ( std::string srv_addr, int pkt_limit, int protocols, int error );
	void Reconnect ( );
	void Close ( );		
	int Run ( );				
	int Process ( Event& e );
	static int NetEventCallback ( Event& e, void* this_ptr );

	// Demo app protocol	
	int InitBuf ( char* buf, const int size, char main_pkt_char );
	void SendPackets ( );
	double GetUpTime ( );
	bool TxActive ( );
	
private:
	std::string m_srvAddr;
	bool m_hasConnected;
	int m_srvPort;
	int m_sock; // This is my socket (local) to access the server
	int m_seq;
	int m_pktSize;
	int m_pktLimit;
	pkt_struct m_txPkt;
	TimeX m_startTime;
	TimeX m_currtime;
	TimeX m_lasttime;
	FILE* m_flowTrace;
};

#endif
