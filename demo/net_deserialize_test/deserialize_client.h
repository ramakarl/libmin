
#ifndef NETDEMO_CLIENT
	#define NETDEMO_CLIENT

	#include "network_system.h"

	#define TEST_TCP_PACKET_SIZE		1500

	class Client : public NetworkSystem {
	public:		
	    Client( const char* trace_file_name = NULL ) : NetworkSystem( trace_file_name ) {
			mHasConnected = false;
		}
	
		// networking 
		void Start ( std::string srv_addr );
		void Reconnect ();
		void Close ();		
		int Run ();				
		int Process (Event& e);
		static int NetEventCallback ( Event& e, void* this_ptr );	

		// test buffer
		void		TransmitTestBuffer();

		int			mHasConnected;

	private:
		int			mSock;					// this is my socket (local) to access the server
		int			mSeq;
		std::string mSrvAddr;
		
		TimeX		m_currtime;
		TimeX		m_lasttime;

		char*		m_testbuf;
		int			m_testlen;


	};

#endif
