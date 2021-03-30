#include <iostream>
#include <fstream>

#include "Client.h"
#include "appProto/ProtocolType.h"

int main(int argc, char *argv[]) {

    Client client(Protocol::OpenSSL_TLS, "127.0.0.1", 12345);

    //client.send("/home/raigo/repos/snow_problem.bag", 1024, 5);
    client.send("/home/raigo/repos/masters-thesis/solution/protocol_testbed/src/protocol/OpenSSLProtocolDTLS.cpp", 1024, 5);

    client.printStatistics();

    return 0;
}
