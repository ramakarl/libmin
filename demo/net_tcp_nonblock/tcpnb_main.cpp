
#include <string>
#include <iostream>

// TCP/IP Non-Blocking demo (tcpnb)
//
// * This demo shows how to write TCP/IP non-blocking behavior 
//   for both Linux & Windows.
// * Pure demo. No dependency on Libmin. 
// * Functions provided here to handle cross-platform correctly.

// Server IP & Port. 
// Put external IP, LAN/NAT IP, or "localhost" here, in quotes.
// Remember to set up port forwarding if the server is behind a router.
#define SERVER_IP			"192.168.1.109"
#define SERVER_PORT		10020

#ifdef _WIN32
	#pragma comment(lib, "Ws2_32.lib")
	#include <conio.h>
	#include <winsock2.h>
	#include <ws2tcpip.h>
	#define CX_SOCKET					SOCKET
	#define CX_SOCK_ERROR			(SOCKET_ERROR+1)		// to allow: result < SOCK_ERROR	
	#define CX_OPT						char
	#define CX_SOCKLEN				int
	#define CX_INVALID_SOCK		INVALID_SOCKET
	#define SLEEP(x)					Sleep(x)
#elif __linux__
	#include <cstring>
	#include <stdarg.h>				// for va_start/va_end
	#include <stdio.h>
	#include <sys/socket.h>
	#include <net/if.h>
	#include <netinet/in.h>
	#include <netinet/tcp.h> 
	#include <arpa/inet.h>
	#include <netdb.h>				// for addrinfo
	#include <errno.h>    
	#include <fcntl.h>
	#include <unistd.h>
	#define CX_SOCKET					int
	#define CX_SOCK_ERROR			0										// check: result < SOCK_ERROR
	#define CX_OPT						int
	#define CX_SOCKLEN				socklen_t
	#define CX_INVALID_SOCK		-1
	#define SLEEP(x)					sleep(x)
#endif   

bool get_arg(int argc, char** argv, const char* chk_arg, std::string& val)
{
  std::string value = "";
  for (int i = 1; i < argc; ++i) {
    if (strcmp(argv[i], chk_arg) == 0) {
      if (i + 1 < argc) val = argv[i + 1];
      return true;
    }
  }
  return false;
}

void setLastError(int i) 
{
	#ifdef _WIN32
		WSASetLastError(i);
	#else
		errno = 0;
	#endif
}

int getLastError()
{
	#ifdef _WIN32
		return WSAGetLastError(); // windows get last error
	#else
		return errno;
	#endif

}
int setSockBlockMode (CX_SOCKET sock, bool block)
{
	#ifdef _WIN32
	  u_long mode = (block) ? 0 : 1;
	  return ioctlsocket(sock, FIONBIO, &mode);
	#else
	  int mode = fcntl(sock, F_GETFL);
	  if (block) mode &= ~O_NONBLOCK; else mode |= O_NONBLOCK;
  	  return fcntl(sock, F_SETFL, mode);
	#endif
}
bool isSockValid (CX_SOCKET sock)
{
	// SOCKET is unsigned on windows, signed on linux.
  // Windows checks for INVALID_SOCKET, a very large unsigned int.
  // Linux checks for -1 (signed). See Windows & Linux accept function, return value.
	return (sock != CX_INVALID_SOCK);
}

void cleanup (CX_SOCKET s)
{
	#ifdef _WIN32
		if (s != 0) {	closesocket(s);	} else { WSACleanup(); }
 	#else
		if (s != 0) { close(s);	}
	#endif
}

std::string getErrorMsg(int& error_id)
{	
	#ifdef _WIN32 
		// get error on windows
		if (error_id == 0) {
			error_id = WSAGetLastError(); // windows get last error
		}
		LPTSTR errorText = NULL;
		DWORD flags = FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS;
		DWORD lang_id = MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT);
		FormatMessage(flags, NULL, error_id, lang_id, (LPSTR)&errorText, 0, NULL);
		std::string error_str = std::string(errorText);
		LocalFree(errorText);
	#else // get error on linux/android
		if (error_id == 0) {
			error_id = errno; // linux error code
		}
		char buf[2048];
		char* error_buf = (char*) strerror_r(error_id, buf, 2048);
		std::string error_str = std::string(error_buf);
	#endif		
	return error_str;
}

bool sockOkWouldBlock ()
{
	#ifdef _WIN32
		int err = getLastError();
		return (err==WSAEWOULDBLOCK || err==WSAEINPROGRESS || err==WSAEALREADY );
	#else
		// Ubuntu notes: EWOULDBLOCK=11, EAGAIN=11, EINPROGRESS=115, EALREADY=114, EISCONN=106		
		return (errno==EWOULDBLOCK || errno==EAGAIN || errno==EINPROGRESS || errno==EALREADY || errno==EISCONN);
	#endif
}

std::string errorf (const char* fmt_raw, ...)
{
	// formatted user msg string
	char buffer[2048];
	va_list args;
	va_start(args, fmt_raw);
	vsnprintf(buffer, sizeof(buffer), fmt_raw, args);
	va_end(args);	
  // complete error message
	int errcode = getLastError();
	std::string msg = "=== ERROR: " + std::string(buffer) + "\nCode " +std::to_string(errcode)+ ": " + getErrorMsg(errcode) + "\n";
	printf (msg.c_str() );	
	return msg;
}



int main ( int argc, char* argv [] )
{	
	int ret;
	
	int serverPort = SERVER_PORT;
	std::string serverIP = SERVER_IP;

	// Server state
	CX_SOCKET serverSock, srvCliSock;	
	struct sockaddr_in serverAddr, srvCliAddr;
	CX_SOCKLEN srvCliAddrSize = sizeof(srvCliAddr);
	bool srvConnected = false;
	bool srvSent = false;

	// Client state
	CX_SOCKET clientSock;	
	struct sockaddr_in cliSrvAddr;
	bool cliConnected = false;
	int cliTimer = 0;
	int cliNumRead = 0;

	u_long mode = 1;		// Non-blocking mode
  
	std::string val	;
	bool bClient = get_arg ( argc, argv, "-c", val );
	bool bServer = get_arg ( argc, argv, "-s", val );

	#ifdef _WIN32
		// Initialize winsock
		WSADATA wsaData;		
		if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
			errorf( "WSAStartup failed.");
			return 1;
		}
	#endif

	if (!bServer && !bClient) {
		errorf ( "Must run server -s or client -c");
		return 1;
	}

	// Server TCP/IP Non-blocking 
	if (bServer) {      
		// Create server listening socket
		serverSock = socket(AF_INET, SOCK_STREAM, 0);
		if ( !isSockValid(serverSock) ) {
			errorf ( "Socket creation failed. " );
			cleanup( serverSock );
			return 1;
		}	else { std::cerr << "Server created socket." << std::endl; }
		// Setup address
		serverAddr.sin_family = AF_INET;
		serverAddr.sin_addr.s_addr = INADDR_ANY;
		serverAddr.sin_port = htons(serverPort);

		// Set socket options
		CX_OPT opt = 1;		
		if ( setsockopt(serverSock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) ) {
			errorf( "Server setsockopt failed.");
			return 1;
		}

		// Bind socket to address
		if (bind(serverSock, (struct sockaddr*) &serverAddr, sizeof(serverAddr)) < CX_SOCK_ERROR) {
			errorf( "Server bind failed.");			
			cleanup( serverSock );
			return 1;
		}	else { std::cerr << "Server bind to address ok." << std::endl; }

		// Listen for connections
		if (listen(serverSock, 3) < CX_SOCK_ERROR) {
			errorf("Server listen failed.");		
			cleanup(serverSock);
			return 1;
		}	else { 
			printf ( "Server listening on %s:%d\n", serverIP.c_str(), serverPort );
		}

		// Set server socket to non-blocking mode
		if (setSockBlockMode (serverSock, false) < CX_SOCK_ERROR) {
			errorf("Server set block mode failed.");			
			cleanup(serverSock);
			return 1;
		}
		std::cout << "Server waiting for connections..." << std::endl;
     
	}	

	// Client TCP/IP Non-blocking 
	if (bClient) {
		// Create socket
		clientSock = socket(AF_INET, SOCK_STREAM, 0);
		if ( !isSockValid(clientSock) ) {		
			errorf( "Client socket create failed. ");			
			cleanup(serverSock);
			return 1;
		}
		// Client set server address
		serverAddr.sin_family = AF_INET;
		serverAddr.sin_port = htons(serverPort);
		addrinfo* pAddrInfo;
		char portname[64]; sprintf( portname, "%d", serverPort );
		int result = getaddrinfo ( serverIP.c_str(), portname, 0, &pAddrInfo );		
		char ipstr[INET_ADDRSTRLEN];
		for (addrinfo* p = pAddrInfo; p != NULL; p = p->ai_next) { // Translate addrinfo to IP string
			struct in_addr* addr;
			if (p->ai_family == AF_INET) {
				struct sockaddr_in* ipv = (struct sockaddr_in*) p->ai_addr;
				addr = &(ipv->sin_addr);
			}			
			inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);
		}
		cliSrvAddr.sin_family = AF_INET;				
		cliSrvAddr.sin_port = htons(serverPort);
		cliSrvAddr.sin_addr.s_addr = inet_addr(ipstr);		// resolved server IP address

		// Set client socket to non-blocking mode
		if (setSockBlockMode(clientSock, false) < CX_SOCK_ERROR) {
			errorf( "Client set block mode failed.");						
			cleanup(clientSock);
			return 1;
		}		 
	}

	// Run server and/or client
	bool done = false;
	while ( !done ) {

		// run server 
		if (bServer) {
			if (!srvConnected) {
				// server not yet connected
				srvCliSock = accept(serverSock, (struct sockaddr*) &srvCliAddr, &srvCliAddrSize);			
				if ( !isSockValid(srvCliSock) ) {					
					if ( sockOkWouldBlock() ) {
						// server waiting for client. no problem.
						setLastError(0);
					} else {
						errorf("Server accept failed.");
						return 1;
					}						
				} else {
					std::cout << "Server connected to client!" << std::endl;
					srvConnected = true;
					setLastError(0);
				}	
			} else {
				// server is connected				
				if (!srvSent) {
					// send the list of numbers to the client
					char numbers[] = {76, 42, 13, 88, 24};
					int num_size = 5;
					send (srvCliSock, (char*) numbers, num_size, 0);
					std::cout << "Server connection ok. Data sent." << std::endl;
					for (int n = 0; n < num_size; n++) {
						printf( "%d ", int(numbers[n]));
					}
					printf ("\n");
					srvSent = true;		
					if (!bClient) done = true;		// no client, we're done.			
				}
				setLastError(0);
			}				
		}

		// run client
		if (bClient) {
			

			if (!cliConnected) {				
				// client not connected	yet
				
				// try connect again
				if (++cliTimer > 1000) {
					cliTimer = 0;					
					printf ( "Client contacting %s:%d\n", serverIP.c_str(), serverPort );
					ret = connect( clientSock, (struct sockaddr*) &cliSrvAddr, sizeof(cliSrvAddr) );					
					if (ret < CX_SOCK_ERROR) {
						if ( sockOkWouldBlock() ) {
							// connection in progress. wait for it.
							// check for complete connection using select
							fd_set writefds;
							FD_ZERO(&writefds);
							FD_SET(clientSock, &writefds);
							timeval timeout;
							timeout.tv_sec = 0;
							timeout.tv_usec = 500;
							// *note*: there must be a delay before selecting after connect
							SLEEP(500);
							ret = select ( clientSock+1, NULL, &writefds, NULL, &timeout);
							if (ret > 0 && FD_ISSET(clientSock, &writefds)) {
								std::cout << "Client connected ok!" << std::endl;
								cliConnected = true;
							}
							setLastError(0);
						} else {
							errorf( "Client connect failed.");
							cleanup(clientSock);
							return 1;
						}
					}
					
				}
			 } else {
				// client connected
				// receive data
				char cli_numbers[10];				
				int num_read = recv (clientSock, (char*) cli_numbers, sizeof(cli_numbers), 0);
				if (num_read > 0) {										
					// received partial data
					printf ( "Client received data.\n ");
					for (int n = 0; n < num_read; n++) {
						printf ( "%d ", int(cli_numbers[n]) );
					}			
					printf("\n");
					cliNumRead += num_read;
				}
				// check if we have all data
				if (cliNumRead == 5) {
					printf("Client done.\n ");
					done = true;
				}
				setLastError(0);
			}
		}

		if (getLastError() != 0) {
			errorf( "Network error.");
			break;
		}
	}

	cleanup(0);	

	return 1;
}

