#include <iostream>
#include <fstream>

#include "Client.h"
#include "protocol/ProtocolType.h"

int main(int argc, char *argv[]) {

    Client client(Protocol::MVFST_QUIC, "127.0.0.1", 12345);

    //std::ifstream fileStream ("/home/raigo/repos/snow_problem.bag",std::fstream::binary);
    std::ifstream fileStream("/home/raigo/repos/masters-thesis/solution/protocol_testbed/src/protocol/OpenSSLProtocolDTLS.cpp",std::fstream::binary);

    client.send(fileStream, 1024, 5);

    client.printStatistics();

    return 0;
}
