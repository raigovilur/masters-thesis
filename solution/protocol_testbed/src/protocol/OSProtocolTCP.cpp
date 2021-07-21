
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <cstring>
#include <iostream>
#include <arpa/inet.h>
#include "OSProtocolTCP.h"
#include <unistd.h>

bool Protocol::OSProtocolTCP::openProtocol(std::string address, uint port, Protocol::Options) {

    _socket = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in saiServerAddress{};
    memset(&saiServerAddress, 0, sizeof(saiServerAddress));
    saiServerAddress.sin_family = AF_INET;
    saiServerAddress.sin_addr.s_addr = inet_addr(address.c_str());
    saiServerAddress.sin_port = htons(port);
    int ret = connect(_socket, (struct sockaddr*) &saiServerAddress, sizeof(saiServerAddress));
    if (ret < 0) {
        std::cout << "Error connecting to server: " << strerror(errno) << std::endl;
        return false;
    }

    return true;
}

bool Protocol::OSProtocolTCP::openProtocolServer(uint port) {
    return false;
}

bool Protocol::OSProtocolTCP::send(const char *buffer, size_t bufferSize, bool eof) {

    if (write(_socket, buffer, bufferSize) < 0) {
        std::cout << "Error writing to socket" << std::endl;
        return false;
    }

    return true;
}

bool Protocol::OSProtocolTCP::closeProtocol() {
    sleep(1);
    close(_socket);
    return false;
}

Protocol::OSProtocolTCP::~OSProtocolTCP() {

}
