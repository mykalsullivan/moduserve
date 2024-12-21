#include "Server.h"
#include "optional_modules/bf/BFModule.h"

int main(int argc, char **argv)
{
    Server::addModule<BFModule>();
    return Server::run(argc, argv);
}
