#include "Client.h"

int main(int argc, char *argv[])
{
    return Client::instance().run(argc, argv);
}
