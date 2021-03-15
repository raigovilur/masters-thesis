#include <iostream>
#include <fstream>

#include "Client.h"
#include "protocol/ProtocolType.h"

int main(int argc, char *argv[]) {

    Client client(Protocol::OpenSSL_TLS, "127.0.0.1", 12345);

    std::ifstream fileStream ("/home/raigo/repos/snow_problem.bag",std::fstream::binary);

    client.send(fileStream, 1024, 5);

    client.printStatistics();

    return 0;
}
