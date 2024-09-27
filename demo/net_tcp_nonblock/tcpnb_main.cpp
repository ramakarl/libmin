
#include <string>
#include <iostream>

#ifdef _WIN32
	#pragma comment(lib, "Ws2_32.lib")
	#include <conio.h>
	#include <winsock2.h>
	#include <ws2tcpip.h>
	#define CX_SOCKET					SOCKET
	#define CX_SOCK_ERROR			SOCKET_ERROR
	#define CX_WOULD_BLOCK		WSAEWOULDBLOCK
	#define CX_INVALID_SOCK		INVALID_SOCKET
	#define	sockLen						int
#elif __linux__
	#include <cstring>
	#include <stdio.h>
	#include <sys/socket.h>
	#include <net/if.h>
	#include <netinet/in.h>
	#include <netinet/tcp.h> 
	#include <errno.h>    
	#include <fcntl.h>
	#include <unistd.h>
	#define CX_SOCKET					int
	#define CX_SOCK_ERROR			0	
	#define CX_INVALID_SOCK		0
	#define CX_WOULD_BLOCK		EWOULDBLOCK	
	#define	sockLen						socklen_t
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
	#ifdef _WIN32 // get error on windows
		return WSAGetLastError(); // windows get last error
	#else
		return errno;
	#endif
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
	#ifdef _WIN32 // get error on windows
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

std::string errorf(const char* fmt_raw, ...)
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
	
	int serverPort = 10020;
	std::string serverIP = "127.0.0.1";

	// Server state
	#ifdef _WIN32
		SOCKET serverSock, srvCliSock;
	#else
		int serverSock, srvCliSock;
	#endif
	struct sockaddr_in serverAddr, srvCliAddr;
	sockLen srvCliAddrSize = sizeof(srvCliAddr);
	bool srvConnected = false;
	bool srvSent = false;

	// Client state
	#ifdef _WIN32
		SOCKET clientSock;
	#else	
		int clientSock;
	#endif
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
		if ((serverSock = socket(AF_INET, SOCK_STREAM, 0)) == CX_INVALID_SOCK) {
			errorf ( "Socket creation failed. " );
			cleanup( serverSock );
			return 1;
		}	else { std::cerr << "Server created socket." << std::endl; }
		// Setup address
		serverAddr.sin_family = AF_INET;
		serverAddr.sin_addr.s_addr = INADDR_ANY;
		serverAddr.sin_port = htons(serverPort);

		// Set socket options
		char opt = 1;
		if (setsockopt(serverSock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
			errorf( "Server setsockopt failed.");
			return 1;
		}

		// Bind socket to address
		if (bind(serverSock, (struct sockaddr*) &serverAddr, sizeof(serverAddr)) <= CX_SOCK_ERROR) {
			errorf( "Server bind failed.");			
			cleanup( serverSock );
			return 1;
		}	else { std::cerr << "Server bind to address ok." << std::endl; }

		// Listen for connections
		if (listen(serverSock, 3) <= CX_SOCK_ERROR) {
			errorf("Server listen failed.");			
			cleanup(serverSock);
			return 1;
		}	else { 
			printf ( "Server listening on %s:%d\n", serverIP.c_str(), serverPort );
		}

		// Set server socket to non-blocking mode
		if (ioctlsocket(serverSock, FIONBIO, &mode) <= CX_SOCK_ERROR) {
			errorf("Server ioctlsocket failed.");			
			cleanup(serverSock);
			return 1;
		}
		std::cout << "Server waiting for connections..." << std::endl;
     
	}	

	// Client TCP/IP Non-blocking 
	if (bClient) {
		// Create socket
		if ((clientSock = socket(AF_INET, SOCK_STREAM, 0)) == CX_INVALID_SOCK) {
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
		printf ( "Client contacting %s:%d\n", ipstr, serverPort );

		// Set client socket to non-blocking mode
		if (ioctlsocket(clientSock, FIONBIO, &mode) <= CX_SOCK_ERROR) {
			errorf( "Client ioctlsocket failed.");						
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
				srvCliSock = accept(serverSock, (struct sockaddr*)& srvCliAddr, &srvCliAddrSize);
				if (srvCliSock == CX_INVALID_SOCK) {
					if (getLastError() == CX_WOULD_BLOCK) {
						// std::cout << "Server waiting for client." << std::endl;
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
				}
				setLastError(0);
			}				
		}

		// run client
		if (bClient) {
			// Attempt to connect to the server
			if (!cliConnected) {
				
				// client not connected				
				if (++cliTimer > 10000) {

					cliTimer = 0;
					// check for complete connection using select
					fd_set writefds;
					FD_ZERO(&writefds);
					FD_SET(clientSock, &writefds);
					timeval timeout;
					timeout.tv_sec = 0;
					timeout.tv_usec = 200;
					ret = select(0, NULL, &writefds, NULL, &timeout);
					if (ret > 0 && FD_ISSET(clientSock, &writefds)) {
						std::cout << "Client connected ok!" << std::endl;
						cliConnected = true;
					} else {
						// try connect again
						ret = connect( clientSock, (struct sockaddr*)&cliSrvAddr, sizeof(cliSrvAddr) );
						if (ret == CX_SOCK_ERROR) {
							if (getLastError() == CX_WOULD_BLOCK) {
								// connection in progress. wait for it.
								std::cout << "Client connecting to server..." << std::endl;
								setLastError(0);						
							}	else {
								errorf( "Client connect failed.");
								cleanup(clientSock);
								return 1;
							}
						}
					}
				}
			}	else {
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

