#include "Client.h"
#include "common/PCH.h"

void printUsage() {
    std::cout << "Usage: myprogram [-h hostname] [-p port]" << std::endl;
    std::cout << "  -i ip          Specify the server's ip address" << std::endl;
    std::cout << "  -p port        Specify the port number" << std::endl;
    std::cout << "  -h             Display this help message" << std::endl;
}

int main(int argc, char *argv[]) {
    std::string serverIP;
    int serverPort = -1;
    std::string username;

    int opt;
    while ((opt = getopt(argc, argv, "i:p:h")) != -1)
    {
        switch (opt)
        {
            case 'i':
                serverIP = optarg;
            break;
            case 'p':
                serverPort = std::stoi(optarg);
            break;
            case 'h':
                printUsage();
            return 0;
            case '?':
                default:
                    printUsage();
            return 1;
        }
    }

    Client client(argc, argv);
}
