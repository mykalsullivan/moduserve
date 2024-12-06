#include "Server.h"
#include "optional_modules/BFModule.h"

int main(int argc, char **argv)
{
    Server server;
    server.addModule<BFModule>();
    return server.run(argc, argv);
}
