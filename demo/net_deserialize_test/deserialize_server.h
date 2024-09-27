
#ifndef NETDEMO_SERVER
	#define NETDEMO_SERVER

	#include "network_system.h"

	#define TEST_TCP_PACKET_SIZE		1500	

	class Server : public NetworkSystem {
	public:		
		Server( const char* trace_file_name = NULL ) : NetworkSystem( trace_file_name ) {
			m_testlen = 0;
			m_testbuf = 0;
		}
	
		// networking funcs
		void Start ( int inject );
		int Run ();		
		void Close ();
		int Process (Event& e);
		static int NetEventCallback ( Event& e, void* this_ptr );	

		// test buffer
		int			BuildTestBuffer( int test_id );


	private:

		int			m_inject;

		char*		m_testbuf;
		char*		m_testptr;
		int			m_testlen;

		std::vector<int>	m_packet_list;
	};

#endif
