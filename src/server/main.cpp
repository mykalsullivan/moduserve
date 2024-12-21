#include "Server.h"
#include "optional_modules/bf/BFModule.h"

int main(int argc, char **argv)
{
    Server server;
    server.addModule<BFModule>();
    return server.run(argc, argv);
}
