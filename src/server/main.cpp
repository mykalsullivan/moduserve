#include "Server.h"
#include <iostream>
#include <unistd.h>

void printUsage() {
    std::cout << "Usage: program [-p port]" << std::endl;
    std::cout << "  -p port        Specify the port number" << std::endl;
    std::cout << "  -h             Display this help message" << std::endl;
}

int main(int argc, char *argv[])
{
    int port = 0;

    // Parse command-line arguments
    int opt;
    while ((opt = getopt(argc, argv, "p:h")) != -1)
    {
        switch (opt)
        {
            case 'p':
                port = std::stoi(optarg);
                break;
            case 'h':
                printUsage();
                return 0;
            case '?':
            default:
                printUsage();
                return -1;
        }
    }
    Server server(port, argc, argv);
}
