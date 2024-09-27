
#ifndef NETDEMO_SERVER
#define NETDEMO_SERVER

#include "network_system.h"

class Server : public NetworkSystem {
public:		
	Server( const char* trace_file_name = NULL ) : NetworkSystem( trace_file_name ) { }

	// Networking functions
	void Start ( );
	int Run ( );		
	void Close ( );
	int Process (Event& e);
	static int NetEventCallback ( Event& e, void* this_ptr );	

	// Demo app protocol
	void InitWords ();
	std::string ConvertToWords ( int num );
	void SendWordsToClient ( std::string msg, int sock );

private:
	std::vector<std::string> wordlist;
};

#endif
