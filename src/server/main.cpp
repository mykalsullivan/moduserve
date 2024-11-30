#include "Server.h"

int main(int argc, char **argv)
{
    return Server::instance().run(argc, argv);
}
