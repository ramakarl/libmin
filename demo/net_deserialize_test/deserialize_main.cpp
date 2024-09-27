
#ifdef _WIN32
  #include <conio.h>
#endif

#ifdef __linux__
  #include <stdio.h>
  #include <sys/ioctl.h>
  #include <termios.h>

  int _kbhit() {
    static const int STDIN = 0;
    static bool kbinit = false;
    if (!kbinit) {
      termios term;
      tcgetattr(STDIN, &term);
      term.c_lflag &= ~ICANON;
      tcsetattr(STDIN, TCSANOW, &term);
      setbuf(stdin, NULL);
      kbinit=true;
    }
    int bytes;
    ioctl(STDIN, FIONREAD, &bytes);
    return bytes;
  }
#endif   

#include "deserialize_client.h"
#include "deserialize_server.h"

std::string get_addr(int argc, char** argv)
{
    std::string addr = "127.0.0.1";
    for (int i = 1; i < argc - 1; ++i) {
        std::string arg = argv[i];
        if (arg == "--addr" || arg == "-a") {
            addr = argv[++i];
        }
    }
    return addr;
}

int main (int argc, char* argv[])
{
    bool server = false;
    int inject = -1;

	addSearchPath ( ASSET_PATH );

    // launch server with -s arg
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-s") == 0) {
            dbgprintf("STARTING SERVER.\n");
            server = true;
        }    
        if (strcmp(argv[i], "-i") == 0) {
            dbgprintf("INJECTION TEST.\n");
            inject = atoi(argv[i+1]);
        }
        if (strcmp(argv[i], "-f") == 0) {
            dbgprintf("INJECTION FROM FILES (packet_stream.raw, packet_sizes.txt).\n"); 
            inject = 255;
        }
    }

    if (server) {

        // Run server
        Server srv("../trace-func-call-server");
        srv.Start ( inject );

        while (!_kbhit()) {
            srv.Run();
        }

        srv.Close();
    }
    else {
        // Run client
        Client cli("../trace-func-call-client");

        cli.Start( get_addr(argc, argv) );        

        while (!_kbhit()) {
            cli.Run();
        }

        cli.Close();
    }

	
	return 1;
}
